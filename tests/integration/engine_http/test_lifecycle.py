"""Engine process lifecycle and project state routes."""

import json
import os
import socket
import subprocess
import tempfile
import unittest

from engine_harness import (API, EngineProcess, EngineTestCase, as_id, copy_project,
                            engine_binary, free_port, require_engine, require_projects)

# SourceGroupType::SOURCE_GROUP_CUSTOM_COMMAND -- the engine adds it unconditionally.
SOURCE_GROUP_CUSTOM_COMMAND = 5


class CapabilitiesTest(EngineTestCase):
    def test_reports_a_version_and_can_create_a_project(self):
        body = self.client.get(f"{API}/capabilities").json()
        self.assertRegex(body["engine_version"], r"^\d+\.\d+")
        self.assertTrue(body["can_create_project"])

    def test_custom_command_is_always_offered(self):
        # Custom Command needs no plugin -- the user supplies the command line -- so even a
        # build with no indexer at all can still create a project.
        body = self.client.get(f"{API}/capabilities").json()
        self.assertIn(SOURCE_GROUP_CUSTOM_COMMAND, body["supported_source_group_types"])

    def test_plugin_entries_are_well_formed(self):
        body = self.client.get(f"{API}/capabilities").json()
        for plugin in body["plugins"]:
            with self.subTest(plugin=plugin.get("id")):
                self.assertTrue(plugin["id"])
                self.assertTrue(plugin["name"])
                self.assertIsInstance(plugin["source_group_types"], list)
                # Every plugin's types must also show up in the aggregate the wizard reads.
                for group in plugin["source_group_types"]:
                    self.assertIn(group, body["supported_source_group_types"])


class NoProjectTest(EngineTestCase):
    def test_project_state_is_reported_not_errored(self):
        # No project loaded is a 200 carrying error_message, not an HTTP error.
        response = self.client.get(f"{API}/project")
        self.assertEqual(response.status, 200)
        body = response.json()
        self.assertEqual(body["error_message"], "No project is loaded.")
        self.assertFalse(body["loaded"])
        self.assertFalse(body["indexing"])

    def test_marking_outdated_without_a_project_is_409(self):
        # The only 409 in the API.
        response = self.client.post(f"{API}/project/state/outdated")
        self.assertEqual(response.status, 409)
        self.assertEqual(response.error, "No project is loaded")

    def test_cancelling_indexing_is_accepted_when_idle(self):
        response = self.client.post(f"{API}/project/indexing/cancel")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.json(), {})

    def test_stats_answer_with_zeroes(self):
        body = self.client.get(f"{API}/stats").json()
        self.assertEqual(as_id(body["stats"]["node_count"]), 0)


class LoadProjectTest(EngineTestCase):
    @classmethod
    def setUpClass(cls):
        require_projects()
        super().setUpClass()
        cls.tempdir, cls.project_file = copy_project("tutorial")
        cls.addClassCleanup(cls.tempdir.cleanup)

    def test_loads_a_project(self):
        response = self.client.put(f"{API}/project", {"project_file_path": self.project_file})
        self.assertEqual(response.status, 200)
        body = response.json()
        self.assertTrue(body["loaded"])
        self.assertFalse(body["indexing"])
        self.assertEqual(body["error_message"], "")
        self.assertTrue(body["description"])

    def test_state_persists_for_the_next_get(self):
        self.client.put(f"{API}/project", {"project_file_path": self.project_file})
        body = self.client.get(f"{API}/project").json()
        self.assertTrue(body["loaded"])
        self.assertEqual(body["error_message"], "")

    def test_marking_outdated_succeeds_once_loaded(self):
        self.client.put(f"{API}/project", {"project_file_path": self.project_file})
        response = self.client.post(f"{API}/project/state/outdated")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.json(), {})

    def test_missing_path_is_rejected(self):
        for body in [{}, {"project_file_path": ""}]:
            with self.subTest(body=body):
                response = self.client.put(f"{API}/project", body)
                self.assertEqual(response.status, 400)
                self.assertEqual(response.error, "Missing project_file_path")

    def test_malformed_body_is_rejected(self):
        response = self.client.put(f"{API}/project", "{not json")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing project_file_path")

    def test_unknown_fields_are_ignored(self):
        # JsonStringToMessage runs with ignore_unknown_fields, so version skew is tolerated.
        response = self.client.put(
            f"{API}/project",
            {"project_file_path": self.project_file, "a_field_from_the_future": 1})
        self.assertEqual(response.status, 200)
        self.assertTrue(response.json()["loaded"])

    def test_nonexistent_project_does_not_load(self):
        response = self.client.put(
            f"{API}/project", {"project_file_path": "/nonexistent/nope.srctrlprj"})
        self.assertEqual(response.status, 200)
        self.assertFalse(response.json()["loaded"])


class RefreshTest(EngineTestCase):
    def test_accepts_all_body_shapes(self):
        # ProtoJson::fromJson returns true on an empty body, so a bodyless POST is valid here.
        for label, body in [("all", {"all": True}),
                            ("not all", {"all": False}),
                            ("empty object", {}),
                            ("no body", None)]:
            with self.subTest(body=label):
                response = self.client.post(f"{API}/project/refresh", body)
                self.assertEqual(response.status, 200)
                self.assertEqual(response.json(), {})

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/project/refresh", "{{{")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed refresh request")


class CompilationDatabaseTest(EngineTestCase):
    def post_cdb(self, path):
        return self.client.post(f"{API}/compilation-database", {"cdb_path": path})

    def test_missing_file_reports_in_band(self):
        # A bad CDB is a 200 with valid=false, not an HTTP error -- easy to get wrong.
        response = self.post_cdb("/nonexistent/compile_commands.json")
        self.assertEqual(response.status, 200)
        body = response.json()
        self.assertFalse(body["valid"])
        self.assertEqual(body["error"], "The provided Compilation Database path does not exist.")

    def test_empty_path_reports_in_band(self):
        response = self.post_cdb("")
        self.assertEqual(response.status, 200)
        self.assertFalse(response.json()["valid"])

    def test_unparseable_file_reports_in_band(self):
        with tempfile.TemporaryDirectory() as tempdir:
            path = os.path.join(tempdir, "compile_commands.json")
            with open(path, "w", encoding="utf-8") as handle:
                handle.write("this is not a compilation database")
            response = self.post_cdb(path)
            self.assertEqual(response.status, 200)
            body = response.json()
            self.assertFalse(body["valid"])
            self.assertTrue(body["error"])

    def test_valid_database_lists_its_source_files(self):
        with tempfile.TemporaryDirectory() as tempdir:
            source = os.path.join(tempdir, "main.cpp")
            with open(source, "w", encoding="utf-8") as handle:
                handle.write("int main() { return 0; }\n")
            path = os.path.join(tempdir, "compile_commands.json")
            with open(path, "w", encoding="utf-8") as handle:
                json.dump([{"directory": tempdir,
                            "command": f"c++ -c {source}",
                            "file": source}], handle)

            response = self.post_cdb(path)
            self.assertEqual(response.status, 200)
            body = response.json()
            self.assertTrue(body["valid"], body)
            self.assertEqual(body["error"], "")
            self.assertIn(source, body["source_files"])
            self.assertFalse(body["contains_include_pch_flags"])

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/compilation-database", "nonsense")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed compilation database request")


class ProcessTest(unittest.TestCase):
    """Tests that need to control the engine process itself, so they spawn their own."""

    @classmethod
    def setUpClass(cls):
        require_engine()

    def test_ephemeral_port_handshake(self):
        engine = EngineProcess()
        self.addCleanup(engine.stop)
        self.assertGreater(engine.port, 0)
        self.assertTrue(engine.token)
        self.assertTrue(engine.is_listening())

    def test_explicit_port_is_honoured(self):
        port = free_port()
        engine = EngineProcess(port=port)
        self.addCleanup(engine.stop)
        self.assertEqual(engine.port, port)

    def test_invalid_port_exits_non_zero(self):
        process = subprocess.run(
            [engine_binary(), "--port", "not-a-number"],
            cwd=os.path.dirname(engine_binary()),
            capture_output=True, text=True, timeout=30, check=False)
        self.assertNotEqual(process.returncode, 0)
        self.assertIn("Invalid port:", process.stderr)

    def test_two_engines_are_independent(self):
        first = EngineProcess()
        self.addCleanup(first.stop)
        second = EngineProcess()
        self.addCleanup(second.stop)

        self.assertNotEqual(first.port, second.port)
        self.assertNotEqual(first.token, second.token)

        # A token is proof of being the client that engine was spawned for, so it must not
        # open the other one.
        response = second.client.get(f"{API}/capabilities", token=first.token)
        self.assertEqual(response.status, 401)

    def test_shutdown_route_stops_the_process(self):
        engine = EngineProcess()
        self.addCleanup(engine.kill)

        response = engine.client.post(f"{API}/shutdown")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.json(), {})

        self.assertEqual(engine.process.wait(timeout=15), 0)

        engine.client.close()
        with self.assertRaises(OSError):
            with socket.create_connection(("127.0.0.1", engine.port), timeout=2):
                pass

    def test_sigterm_stops_the_process(self):
        engine = EngineProcess()
        self.addCleanup(engine.kill)
        engine.process.terminate()
        self.assertEqual(engine.process.wait(timeout=15), 0)


if __name__ == "__main__":
    unittest.main()
