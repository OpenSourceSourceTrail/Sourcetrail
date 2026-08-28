"""The one runnable check for the MCP server.

Drives the four tools against a real engine holding the shipped tictactoe_cpp sample. That
sample's .srctrldb is prebuilt, so this runs without an indexer and therefore in a build without
BUILD_CXX_LANGUAGE_PACKAGE.

    SOURCETRAIL_ENGINE=build/app/sourcetrail_engine \
    SOURCETRAIL_PROJECTS_DIR=bin/app/user/projects \
    python3 -m unittest -v mcp.test_mcp
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import sourcetrail_mcp    # noqa: E402
from engine_harness import copy_project, engine_binary, require_engine, require_projects    # noqa: E402


class _StubEngine:
    """Fakes just enough of Engine for a search_symbols call, no process spawned."""

    def __init__(self, search_response):
        self._search_response = search_response

    def node_mask(self):
        return 0

    def get(self, path, **params):
        assert path.endswith("/search"), path
        return self._search_response


class ToolsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        require_engine()
        require_projects()
        # Loading a project mutates its .srctrldb in place, so work on a copy.
        cls.tempdir, project = copy_project("tictactoe_cpp")
        cls.addClassCleanup(cls.tempdir.cleanup)

        sourcetrail_mcp.ENGINE = sourcetrail_mcp.Engine(engine_binary(), project)
        cls.addClassCleanup(lambda: sourcetrail_mcp.ENGINE._process
                            and sourcetrail_mcp.ENGINE._process.stop())

    def test_parse_name(self):
        # No engine needed: the serialized form is `<delimiter>\tm` then `<name>\ts<pre>\tp<post>`.
        self.assertEqual(sourcetrail_mcp.parse_name("::\tmPlayer\ts\tp"), "Player")
        self.assertEqual(
            sourcetrail_mcp.parse_name("::\tmGame\ts\tp\tnrun\tsvoid \tp()"), "Game::run")

    def test_search_expands_a_multi_id_match(self):
        # A single fuzzy match can carry several ids that share the same indexed text (e.g. one
        # `main` per translation unit) -- search_symbols must expand each into its own result
        # instead of keeping only token_ids[0]. No real engine needed: stub ENGINE.get.
        real_engine, sourcetrail_mcp.ENGINE = sourcetrail_mcp.ENGINE, _StubEngine({
            "matches": [{
                "name": "main",
                "node_kind": 1 << 12,    # FUNCTION
                "score": 50,
                "token_ids": ["1", "2"],
                "token_names_serialized": ["::\tmmain\ts\tp", "::\tmgui\ts\tp\tnmain\ts\tp"],
            }]
        })
        try:
            matches = sourcetrail_mcp.search_symbols("main")
        finally:
            sourcetrail_mcp.ENGINE = real_engine
        self.assertEqual([m["id"] for m in matches], ["1", "2"])
        self.assertEqual([m["name"] for m in matches], ["main", "gui::main"])
        self.assertEqual({m["kind"] for m in matches}, {"FUNCTION"})

    def test_search_finds_a_symbol(self):
        matches = sourcetrail_mcp.search_symbols("Player")
        self.assertTrue(matches, "fixture has no symbol named like 'Player'")
        first = matches[0]
        # Ids stay strings end to end: the wire quotes uint64 and we never widen that contract.
        self.assertIsInstance(first["id"], str)
        self.assertTrue(first["id"].isdigit())
        self.assertNotIn("UNKNOWN", first["kind"])

    def test_describe_returns_a_definition_with_source(self):
        info = sourcetrail_mcp.describe_symbol(self.a_symbol())
        self.assertNotIn("UNKNOWN", info["kind"])
        self.assertTrue(info["name"])
        self.assertIsNotNone(info["definition"], "symbol has no definition location")
        self.assertTrue(info["definition"]["code"].strip())
        self.assertLessEqual(info["definition"]["start_line"], info["definition"]["end_line"])

    def test_references_have_resolvable_kinds(self):
        references = sourcetrail_mcp.find_references(self.a_symbol())
        every = references["incoming"] + references["outgoing"]
        self.assertTrue(every, "a class in the fixture should have at least one reference")
        for reference in every:
            self.assertNotIn("UNKNOWN", reference["kind"])

    def test_references_filter_by_kind(self):
        references = sourcetrail_mcp.find_references(self.a_symbol(), kinds=["MEMBER"])
        every = references["incoming"] + references["outgoing"]
        self.assertTrue(every, "a class should own members")
        self.assertEqual({reference["kind"] for reference in every}, {"MEMBER"})

    def test_read_source_slices_by_line(self):
        definition = sourcetrail_mcp.describe_symbol(self.a_symbol())["definition"]
        text = sourcetrail_mcp.read_source(definition["file"], 1, 5)
        self.assertEqual(len(text.splitlines()), 5)

    def a_symbol(self):
        matches = sourcetrail_mcp.search_symbols("Player")
        self.assertTrue(matches, "fixture has no symbol named like 'Player'")
        return matches[0]["id"]


if __name__ == "__main__":
    unittest.main()
