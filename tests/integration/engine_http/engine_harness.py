"""Black-box harness for the Sourcetrail engine HTTP API.

Spawns the real ``sourcetrail_engine`` binary, reads its ``ENGINE_PORT <port> <token>``
handshake line, and hands the tests a small HTTP client bound to that port and token.

Standard library only, on purpose: this suite is an independent client of the engine, so it
shares no code with the implementation it is testing and needs nothing installed to run.
"""

import http.client
import json
import os
import queue
import shutil
import socket
import subprocess
import tempfile
import threading
import time
import unittest
import urllib.parse

# The engine flushes the handshake before it does anything else, but a cold start still has to
# construct the Application and discover plugins first.
HANDSHAKE_TIMEOUT = 30.0
# How long to keep polling /capabilities before deciding the listener never came up.
READY_TIMEOUT = 30.0
SHUTDOWN_TIMEOUT = 10.0

API = "/api/v1"


def engine_binary():
    """Path to the engine under test, or None when it was not pointed out to us."""
    path = os.environ.get("SOURCETRAIL_ENGINE")
    if path and os.path.isfile(path):
        return os.path.abspath(path)
    return None


def projects_dir():
    """Directory holding the shipped sample projects, or None."""
    path = os.environ.get("SOURCETRAIL_PROJECTS_DIR")
    if path and os.path.isdir(path):
        return os.path.abspath(path)
    return None


def require_engine():
    """Skips the calling test module unless the engine binary was handed to us."""
    if engine_binary() is None:
        raise unittest.SkipTest(
            "SOURCETRAIL_ENGINE is unset or does not point at a file; "
            "build Sourcetrail_engine and run through ctest, or set it by hand")


def require_projects():
    if projects_dir() is None:
        raise unittest.SkipTest(
            "SOURCETRAIL_PROJECTS_DIR is unset or does not point at a directory")


def free_port():
    """Asks the OS for a port, then gives it straight back."""
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def as_id(value):
    """Unwraps a protobuf uint64.

    ProtoJson leaves uint64 as a quoted JSON string, so every id, count and request id on the
    wire is a str. Going through this helper rather than int() keeps the tests honest about
    that contract instead of quietly accepting either shape.
    """
    if not isinstance(value, str):
        raise AssertionError(f"expected a uint64 as a JSON string, got {value!r}")
    return int(value)


def query(**params):
    """Builds a query string, dropping None values and joining lists with commas."""
    pairs = []
    for key, value in params.items():
        if value is None:
            continue
        if isinstance(value, bool):
            value = "true" if value else "false"
        elif isinstance(value, (list, tuple)):
            value = ",".join(str(item) for item in value)
        pairs.append((key, str(value)))
    return ("?" + urllib.parse.urlencode(pairs)) if pairs else ""


class Response:
    """One HTTP response: status, headers and the decoded JSON body."""

    def __init__(self, status, headers, body):
        self.status = status
        self.headers = headers
        self.body = body

    def header(self, name):
        return self.headers.get(name.lower())

    def json(self):
        if not self.body:
            return {}
        return json.loads(self.body)

    @property
    def error(self):
        """The `error` string the server puts in every non-2xx body."""
        try:
            return self.json().get("error")
        except json.JSONDecodeError:
            return None


class Client:
    """Thin HTTP client over http.client.

    Raw rather than urllib because the transport tests need to send a wrong bearer token, an
    arbitrary Origin, an unregistered method, and to reuse one keep-alive connection.
    """

    def __init__(self, port, token, timeout=30.0):
        self.port = port
        self.token = token
        self.timeout = timeout
        self._conn = None

    def _connection(self, reuse):
        if reuse and self._conn is not None:
            return self._conn
        conn = http.client.HTTPConnection("127.0.0.1", self.port, timeout=self.timeout)
        if reuse:
            self._conn = conn
        return conn

    def close(self):
        if self._conn is not None:
            self._conn.close()
            self._conn = None

    def request(self, method, path, body=None, headers=None, token=..., reuse=False):
        """Issues one request. Pass token=None to omit the Authorization header entirely."""
        sent = dict(headers or {})
        if token is ...:
            token = self.token
        if token is not None:
            sent["Authorization"] = f"Bearer {token}"

        payload = None
        if body is not None:
            payload = body if isinstance(body, (bytes, str)) else json.dumps(body)
            sent.setdefault("Content-Type", "application/json")

        conn = self._connection(reuse)
        try:
            conn.request(method, path, body=payload, headers=sent)
            raw = conn.getresponse()
            data = raw.read().decode("utf-8", "replace")
            headers_out = {key.lower(): value for key, value in raw.getheaders()}
            response = Response(raw.status, headers_out, data)
        finally:
            if not reuse:
                conn.close()
        return response

    def get(self, path, **kwargs):
        return self.request("GET", path, **kwargs)

    def post(self, path, body=None, **kwargs):
        return self.request("POST", path, body=body, **kwargs)

    def put(self, path, body=None, **kwargs):
        return self.request("PUT", path, body=body, **kwargs)

    def patch(self, path, body=None, **kwargs):
        return self.request("PATCH", path, body=body, **kwargs)

    def delete(self, path, **kwargs):
        return self.request("DELETE", path, **kwargs)


class EngineProcess:
    """A running sourcetrail_engine, addressable over HTTP."""

    def __init__(self, port=0, binary=None):
        self.binary = binary or engine_binary()
        if self.binary is None:
            raise unittest.SkipTest("SOURCETRAIL_ENGINE is not set")

        # cwd matters: platform_paths::setupPaths() and IndexerPluginRegistry::discover()
        # resolve data/, user/ and plugins/ relative to the app directory the binary sits in.
        self.app_dir = os.path.dirname(self.binary)
        self.stderr_lines = []

        self.process = subprocess.Popen(
            [self.binary, "--port", str(port)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.app_dir,
            text=True,
            bufsize=1)

        self._drain_stderr()
        self.port, self.token = self._handshake()
        self.client = Client(self.port, self.token)
        self._wait_until_ready()

    # -- startup -------------------------------------------------------------

    def _drain_stderr(self):
        """Keeps stderr flowing so the engine never blocks on a full pipe."""

        def pump():
            for line in self.process.stderr:
                self.stderr_lines.append(line.rstrip("\n"))

        thread = threading.Thread(target=pump, daemon=True)
        thread.start()

    def _handshake(self):
        """Reads and parses the first stdout line: `ENGINE_PORT <port> <token>`."""
        first = queue.Queue(maxsize=1)

        def read_line():
            line = self.process.stdout.readline()
            first.put(line)

        thread = threading.Thread(target=read_line, daemon=True)
        thread.start()

        try:
            line = first.get(timeout=HANDSHAKE_TIMEOUT)
        except queue.Empty:
            self.kill()
            raise AssertionError(
                f"engine printed no handshake line within {HANDSHAKE_TIMEOUT}s; "
                f"stderr:\n{self.stderr()}")

        parts = line.strip().split()
        if len(parts) < 3 or parts[0] != "ENGINE_PORT":
            self.kill()
            raise AssertionError(
                f"expected 'ENGINE_PORT <port> <token>', got {line!r}; stderr:\n{self.stderr()}")
        return int(parts[1]), parts[2]

    def _wait_until_ready(self):
        """Polls until the listener answers, so tests never race the accept thread."""
        deadline = time.monotonic() + READY_TIMEOUT
        last = None
        while time.monotonic() < deadline:
            try:
                response = self.client.get(f"{API}/capabilities")
                if response.status == 200:
                    return
                last = f"status {response.status}"
            except OSError as error:
                last = str(error)
            time.sleep(0.05)
        self.kill()
        raise AssertionError(f"engine never became ready ({last}); stderr:\n{self.stderr()}")

    # -- teardown ------------------------------------------------------------

    def stop(self):
        """Shutdown route first, then terminate, then kill -- QtEngineSupervisor's sequence."""
        try:
            if self.process.poll() is not None:
                return self.process.returncode

            try:
                self.client.post(f"{API}/shutdown")
            except OSError:
                pass

            try:
                return self.process.wait(timeout=SHUTDOWN_TIMEOUT)
            except subprocess.TimeoutExpired:
                pass

            self.process.terminate()
            try:
                return self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                try:
                    return self.process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    return None
        finally:
            self.client.close()
            self._close_pipes()

    def kill(self):
        try:
            self.process.kill()
            try:
                return self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                return None
        finally:
            self.client.close()
            self._close_pipes()

    def _close_pipes(self):
        """The stderr pump thread holds the read end open; drop both so no fd leaks."""
        for pipe in (self.process.stdout, self.process.stderr):
            try:
                if pipe is not None:
                    pipe.close()
            except OSError:
                pass

    def stderr(self):
        return "\n".join(self.stderr_lines)

    # -- convenience ---------------------------------------------------------

    def is_listening(self):
        try:
            with socket.create_connection(("127.0.0.1", self.port), timeout=1):
                return True
        except OSError:
            return False

    def open_event_stream(self, token=..., origin=None):
        """Opens /api/v1/events and returns an EventStream positioned after the head."""
        return EventStream(self.port, self.token if token is ... else token, origin)

    def load_project(self, project_file):
        return self.client.put(f"{API}/project", {"project_file_path": str(project_file)})


class EventStream:
    """Reader for the server-sent-event stream at /api/v1/events.

    Hand-rolled rather than reusing http.client's response object, because the tests assert on
    the raw frame grammar (`event: <name>\\ndata: <json>\\n\\n`) and on the `:keep-alive`
    comments, both of which a higher-level reader would hide.
    """

    def __init__(self, port, token, origin=None, timeout=30.0):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=timeout)
        request = [
            f"GET {API}/events HTTP/1.1",
            "Host: 127.0.0.1",
            "Accept: text/event-stream",
        ]
        if token is not None:
            request.append(f"Authorization: Bearer {token}")
        if origin is not None:
            request.append(f"Origin: {origin}")
        self.socket.sendall(("\r\n".join(request) + "\r\n\r\n").encode())

        self._buffer = b""
        self.status, self.headers = self._read_head()

    def _read_head(self):
        while b"\r\n\r\n" not in self._buffer:
            chunk = self.socket.recv(4096)
            if not chunk:
                raise AssertionError("connection closed before the response head arrived")
            self._buffer += chunk

        head, self._buffer = self._buffer.split(b"\r\n\r\n", 1)
        lines = head.decode("utf-8", "replace").split("\r\n")
        status = int(lines[0].split()[1])
        headers = {}
        for line in lines[1:]:
            if ":" in line:
                key, value = line.split(":", 1)
                headers[key.strip().lower()] = value.strip()
        return status, headers

    def read_body(self, limit=4096):
        """Reads whatever body bytes are already buffered or immediately available."""
        if not self._buffer:
            try:
                self._buffer += self.socket.recv(limit)
            except (socket.timeout, TimeoutError, OSError):
                pass
        body, self._buffer = self._buffer, b""
        return body.decode("utf-8", "replace")

    def read_frame(self, timeout=30.0):
        """Returns the next raw SSE frame (text up to and excluding the blank-line separator)."""
        deadline = time.monotonic() + timeout
        while b"\n\n" not in self._buffer:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            self.socket.settimeout(remaining)
            try:
                chunk = self.socket.recv(4096)
            except (socket.timeout, TimeoutError):
                return None
            if not chunk:
                return None
            self._buffer += chunk

        frame, self._buffer = self._buffer.split(b"\n\n", 1)
        return frame.decode("utf-8", "replace")

    def read_event(self, timeout=30.0, skip_comments=True):
        """Returns the next (name, parsed data) pair, skipping `:keep-alive` comments."""
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            frame = self.read_frame(timeout=remaining)
            if frame is None:
                return None
            if skip_comments and frame.startswith(":"):
                continue

            name = None
            data = []
            for line in frame.split("\n"):
                if line.startswith(":"):
                    continue
                if line.startswith("event: "):
                    name = line[len("event: "):]
                elif line.startswith("data: "):
                    data.append(line[len("data: "):])
            if name is None:
                continue
            payload = "\n".join(data)
            return name, (json.loads(payload) if payload else None)

    def close(self):
        try:
            self.socket.close()
        except OSError:
            pass


def copy_project(name):
    """Copies a shipped sample project into a temp dir and returns (tempdir, .srctrlprj path).

    Loading a project mutates its .srctrldb/.srctrlbm in place, so the tests always work on a
    copy and never touch the assets tracked in bin/app/user/projects.
    """
    source = os.path.join(projects_dir(), name)
    if not os.path.isdir(source):
        raise unittest.SkipTest(f"sample project {name!r} is not present at {source}")

    tempdir = tempfile.TemporaryDirectory(prefix=f"sourcetrail_{name}_")
    destination = os.path.join(tempdir.name, name)
    shutil.copytree(source, destination)
    return tempdir, os.path.join(destination, f"{name}.srctrlprj")


class EngineTestCase(unittest.TestCase):
    """Base for tests wanting one engine for the whole class, and no project loaded."""

    engine = None

    @classmethod
    def setUpClass(cls):
        require_engine()
        cls.engine = EngineProcess()
        cls.addClassCleanup(cls.engine.stop)
        cls.client = cls.engine.client


class LoadedProjectTestCase(EngineTestCase):
    """Base for tests wanting one engine with a sample project already loaded."""

    project_name = "tictactoe_cpp"

    @classmethod
    def setUpClass(cls):
        require_engine()
        require_projects()
        cls.tempdir, cls.project_file = copy_project(cls.project_name)
        cls.addClassCleanup(cls.tempdir.cleanup)

        cls.engine = EngineProcess()
        cls.addClassCleanup(cls.engine.stop)
        cls.client = cls.engine.client

        response = cls.engine.load_project(cls.project_file)
        if response.status != 200 or not response.json().get("loaded"):
            raise AssertionError(
                f"could not load {cls.project_file}: status {response.status}, body {response.body}")
