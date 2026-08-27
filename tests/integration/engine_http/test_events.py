"""The server-sent-event stream at /api/v1/events, and the dialog round trip it carries."""

import json
import os
import re
import threading
import unittest

from engine_harness import (API, EngineProcess, EngineTestCase, as_id, copy_project,
                            require_engine, require_projects)

# EngineEvent's oneof arms, which the server uses as the SSE event name.
ENGINE_EVENT_NAMES = {
    "indexingStarted", "indexingProgress", "indexingFinished", "indexingError",
    "statusInfo", "unknownProgress", "progress", "clearDialogs", "errorCount",
}


def snake_case(name):
    """`indexingStarted` -> `indexing_started`, the spelling ProtoJson uses for the arm key."""
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).lower()


class EventStreamAuthTest(EngineTestCase):
    def test_missing_token_is_rejected(self):
        stream = self.engine.open_event_stream(token=None)
        self.addCleanup(stream.close)
        self.assertEqual(stream.status, 401)

    def test_wrong_token_is_rejected(self):
        stream = self.engine.open_event_stream(token="nope")
        self.addCleanup(stream.close)
        self.assertEqual(stream.status, 401)

    def test_unknown_origin_is_rejected(self):
        stream = self.engine.open_event_stream(origin="http://evil.example")
        self.addCleanup(stream.close)
        self.assertEqual(stream.status, 403)


class EventStreamHeadersTest(EngineTestCase):
    def test_stream_headers(self):
        stream = self.engine.open_event_stream()
        self.addCleanup(stream.close)

        self.assertEqual(stream.status, 200)
        self.assertEqual(stream.headers["content-type"], "text/event-stream")
        self.assertEqual(stream.headers["cache-control"], "no-cache")
        self.assertEqual(stream.headers["connection"], "keep-alive")
        # Tells a reverse proxy not to sit on the stream.
        self.assertEqual(stream.headers["x-accel-buffering"], "no")

    def test_no_cors_header_without_an_allowed_origin(self):
        stream = self.engine.open_event_stream()
        self.addCleanup(stream.close)
        self.assertNotIn("access-control-allow-origin", stream.headers)


class EventDeliveryTest(unittest.TestCase):
    """Needs its own engine per test: a project load is what makes events flow."""

    @classmethod
    def setUpClass(cls):
        require_engine()
        require_projects()
        cls.tempdir, cls.project_file = copy_project("tutorial")
        cls.addClassCleanup(cls.tempdir.cleanup)

    def start_engine(self):
        engine = EngineProcess()
        self.addCleanup(engine.stop)
        return engine

    def test_events_arrive_while_a_project_loads(self):
        engine = self.start_engine()
        stream = engine.open_event_stream()
        self.addCleanup(stream.close)

        engine.load_project(self.project_file)

        event = stream.read_event(timeout=30)
        self.assertIsNotNone(event, "no event arrived while loading a project")
        name, data = event
        self.assertIn(name, ENGINE_EVENT_NAMES | {"dialog"})
        self.assertIsInstance(data, dict)

    def test_event_name_matches_the_payload_arm(self):
        # The SSE name is the oneof arm, so a browser can addEventListener per event type
        # instead of switching on a discriminator inside the payload. `data` is the whole
        # EngineEvent wrapper, with the arm nested under its own key -- not the bare arm.
        #
        # Note the two spellings: the event *name* is camelCase (the C++ switch writes it by
        # hand), while the payload *key* is snake_case, because ProtoJson runs with
        # preserve_proto_field_names. A client matching one against the other must convert.
        engine = self.start_engine()
        stream = engine.open_event_stream()
        self.addCleanup(stream.close)

        engine.load_project(self.project_file)

        for _ in range(10):
            event = stream.read_event(timeout=30)
            if event is None:
                break
            name, data = event
            if name in ENGINE_EVENT_NAMES:
                self.assertEqual(len(data), 1, f"{name} payload carries more than the arm")
                self.assertEqual(next(iter(data)), snake_case(name),
                                 f"{name} payload is not the EngineEvent wrapper")
                self.assertIsInstance(data[snake_case(name)], dict)
                return
        self.skipTest("no EngineEvent arrived to inspect")

    def test_frame_grammar(self):
        engine = self.start_engine()
        stream = engine.open_event_stream()
        self.addCleanup(stream.close)

        engine.load_project(self.project_file)

        frame = stream.read_frame(timeout=30)
        self.assertIsNotNone(frame, "no frame arrived")
        if frame.startswith(":"):
            return    # a keep-alive comment is a valid frame too
        lines = frame.split("\n")
        self.assertTrue(lines[0].startswith("event: "), frame)
        self.assertTrue(any(line.startswith("data: ") for line in lines), frame)
        payload = "\n".join(line[len("data: "):] for line in lines if line.startswith("data: "))
        json.loads(payload)

    def test_two_readers_both_receive_events(self):
        engine = self.start_engine()
        first = engine.open_event_stream()
        second = engine.open_event_stream()
        self.addCleanup(first.close)
        self.addCleanup(second.close)

        engine.load_project(self.project_file)

        results = {}

        def read(label, stream):
            results[label] = stream.read_event(timeout=30)

        threads = [threading.Thread(target=read, args=("first", first)),
                   threading.Thread(target=read, args=("second", second))]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join(timeout=40)

        self.assertIsNotNone(results.get("first"), "first reader got nothing")
        self.assertIsNotNone(results.get("second"), "second reader got nothing")

    def test_stream_survives_ordinary_requests(self):
        engine = self.start_engine()
        stream = engine.open_event_stream()
        self.addCleanup(stream.close)

        # The stream hijacks its own connection; unrelated calls must be unaffected.
        for _ in range(3):
            self.assertEqual(engine.client.get(f"{API}/capabilities").status, 200)
        self.assertEqual(stream.status, 200)

    @unittest.skipUnless(os.environ.get("SOURCETRAIL_SLOW_TESTS"),
                         "keep-alive tick is 15s; set SOURCETRAIL_SLOW_TESTS=1 to include it")
    def test_idle_stream_emits_a_keep_alive_comment(self):
        engine = self.start_engine()
        stream = engine.open_event_stream()
        self.addCleanup(stream.close)

        frame = stream.read_frame(timeout=40)
        self.assertIsNotNone(frame, "idle stream produced nothing within 40s")
        self.assertTrue(frame.startswith(":"), frame)


class DialogResponseTest(EngineTestCase):
    """The dialog round trip, exercised from the answering side."""

    def test_answering_an_unknown_id_is_404(self):
        response = self.client.post(f"{API}/dialogs/4242", {"selected_option": 0})
        self.assertEqual(response.status, 404)
        self.assertEqual(response.error, "No dialog is waiting on that id")

    def test_answering_with_the_id_in_the_body_is_404_when_unknown(self):
        response = self.client.post(
            f"{API}/dialogs/", {"request_id": "4243", "selected_option": 0})
        self.assertEqual(response.status, 404)

    def test_missing_id_is_rejected(self):
        response = self.client.post(f"{API}/dialogs/not-a-number", {"selected_option": 0})
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing dialog request id")

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/dialogs/1", "not a dialog response")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed dialog response")

    def test_request_ids_are_uint64_strings(self):
        # DialogRequest.request_id crosses the wire as a quoted string like every other uint64,
        # so a client that echoes it back verbatim must be accepted.
        response = self.client.post(f"{API}/dialogs/", {"request_id": "99", "selected_option": 1})
        self.assertIn(response.status, (200, 404))


class DialogRoundTripTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_engine()
        require_projects()
        cls.tempdir, cls.project_file = copy_project("tutorial")
        cls.addClassCleanup(cls.tempdir.cleanup)

    def test_a_dialog_can_be_answered_over_http(self):
        engine = EngineProcess()
        self.addCleanup(engine.stop)
        stream = engine.open_event_stream()
        self.addCleanup(stream.close)

        engine.load_project(self.project_file)

        # A dialog only appears when the engine needs a decision; if none does, there is
        # nothing to answer and the other tests already cover the error branches.
        for _ in range(10):
            event = stream.read_event(timeout=20)
            if event is None:
                self.skipTest("engine asked no dialog during this run")
            name, data = event
            if name == "dialog":
                request_id = as_id(data["request_id"])
                response = engine.client.post(
                    f"{API}/dialogs/{request_id}", {"selected_option": 0})
                self.assertEqual(response.status, 200)
                self.assertEqual(response.json(), {})
                return
        self.skipTest("engine asked no dialog during this run")


if __name__ == "__main__":
    unittest.main()
