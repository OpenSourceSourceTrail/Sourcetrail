"""Read routes, exercised against a real indexed project.

The fixture is a copy of the shipped tictactoe_cpp sample, whose .srctrldb is prebuilt -- so
these run without an indexer, and therefore in a build without BUILD_CXX_LANGUAGE_PACKAGE.
"""

import unittest
import urllib.parse

from engine_harness import API, LoadedProjectTestCase, as_id, query

# LocationType::LOCATION_TOKEN
LOCATION_TOKEN = 0
# TooltipOrigin::TOOLTIP_ORIGIN_CODE
TOOLTIP_ORIGIN_CODE = 1


class ProjectDataTestCase(LoadedProjectTestCase):
    """Adds lookups the individual suites build their requests from."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()

        stats = cls.client.get(f"{API}/stats").json()
        # getAutocompletionMatches takes a NodeTypeSet; a zero mask is the empty set and matches
        # nothing, so every search here goes through the mask the engine itself reports.
        cls.node_mask = stats["available_node_types"]
        cls.edge_mask = stats["available_edge_types"]

        graph = cls.client.get(f"{API}/graph" + query(mode="all")).json()
        cls.nodes = graph["graph"]["nodes"]
        cls.file_paths = [node["file_path"] for node in cls.nodes if node.get("file_path")]

        # mode=all is the overview graph: top-level nodes only, no edges and no expandable
        # children. Edges and children come from the graph around a real symbol instead.
        matches = cls.client.get(
            f"{API}/search" + query(q="Player", types=cls.node_mask)).json()["matches"]
        cls.symbol_token = as_id(matches[0]["token_ids"][0]) if matches else None

        cls.edges = []
        cls.parent_nodes = []
        if cls.symbol_token is not None:
            around = cls.client.get(
                f"{API}/graph" + query(mode="active", tokens=[cls.symbol_token])).json()["graph"]
            cls.edges = around.get("edges", [])
            cls.parent_nodes = [node for node in around["nodes"] if as_id(node["child_count"]) > 0]

    @classmethod
    def encoded(cls, path):
        return urllib.parse.quote(path, safe="")

    def a_file(self):
        self.assertTrue(self.file_paths, "fixture has no indexed files")
        return self.file_paths[0]

    def a_symbol_token(self):
        """A token id of a real class in the fixture."""
        self.assertIsNotNone(self.symbol_token, "fixture has no symbol named like 'Player'")
        return self.symbol_token


class StatsTest(ProjectDataTestCase):
    def test_no_include_returns_everything(self):
        body = self.client.get(f"{API}/stats").json()
        self.assertGreater(as_id(body["stats"]["node_count"]), 0)
        self.assertGreater(as_id(body["stats"]["file_count"]), 0)
        self.assertIn("error_total", body)
        self.assertGreater(body["available_node_types"], 0)

    def test_counts_are_uint64_strings(self):
        stats = self.client.get(f"{API}/stats" + query(include="counts")).json()["stats"]
        for key in ["node_count", "edge_count", "file_count",
                    "completed_file_count", "file_loc_count"]:
            with self.subTest(key=key):
                self.assertIsInstance(stats[key], str)

    def test_include_narrows_to_counts(self):
        # `include` exists so the status bar, which wants only error counts, does not make the
        # engine run all four queries.
        body = self.client.get(f"{API}/stats" + query(include="counts")).json()
        self.assertGreater(as_id(body["stats"]["node_count"]), 0)
        self.assertEqual(body["available_node_types"], 0)

    def test_include_narrows_to_errors(self):
        # always_print_primitive_fields spells out primitives, but an unset *submessage* is
        # omitted entirely -- so a narrowed response has no `stats` key at all.
        body = self.client.get(f"{API}/stats" + query(include="errors")).json()
        self.assertNotIn("stats", body)
        self.assertEqual(body["available_node_types"], 0)
        self.assertIn("error_total", body)

    def test_include_accepts_several_parts(self):
        body = self.client.get(f"{API}/stats" + query(include=["errors", "types"])).json()
        self.assertGreater(body["available_node_types"], 0)
        self.assertNotIn("stats", body)


class GraphTest(ProjectDataTestCase):
    def test_mode_all(self):
        body = self.client.get(f"{API}/graph" + query(mode="all")).json()
        self.assertTrue(body["graph"]["nodes"])

    def test_mode_defaults_to_all(self):
        body = self.client.get(f"{API}/graph").json()
        self.assertTrue(body["graph"]["nodes"])

    def test_ids_are_uint64_strings(self):
        for node in self.nodes[:5]:
            with self.subTest(node=node["name_hierarchy_serialized"]):
                self.assertIsInstance(node["id"], str)
                self.assertGreater(as_id(node["id"]), 0)

    def test_mode_types(self):
        body = self.client.get(f"{API}/graph" + query(mode="types", mask=self.node_mask)).json()
        self.assertTrue(body["graph"]["nodes"])

    def test_mode_types_with_an_empty_mask_returns_nothing(self):
        body = self.client.get(f"{API}/graph" + query(mode="types", mask=0)).json()
        self.assertFalse(body.get("graph", {}).get("nodes"))

    def test_mode_children(self):
        self.assertTrue(self.parent_nodes, "fixture has no node with children")
        body = self.client.get(
            f"{API}/graph" + query(mode="children", id=as_id(self.parent_nodes[0]["id"]))).json()
        self.assertTrue(body["graph"]["nodes"])

    def test_mode_active(self):
        token = self.a_symbol_token()
        body = self.client.get(f"{API}/graph" + query(mode="active", tokens=[token])).json()
        self.assertTrue(body["graph"]["nodes"])
        self.assertIn("is_active_namespace", body)

    def test_mode_active_with_expanded_nodes(self):
        token = self.a_symbol_token()
        body = self.client.get(
            f"{API}/graph" + query(mode="active", tokens=[token], expanded=[token])).json()
        self.assertTrue(body["graph"]["nodes"])

    def test_mode_trail(self):
        self.assertTrue(self.edges, "fixture has no edges")
        edge = self.edges[0]
        body = self.client.get(f"{API}/graph" + query(
            mode="trail",
            origin=as_id(edge["from_node_id"]),
            target=as_id(edge["to_node_id"]),
            nodeMask=self.node_mask,
            edgeMask=self.edge_mask,
            depth=2,
            directed=True)).json()
        self.assertIn("graph", body)

    def test_parent_files_are_folded_in(self):
        # Folded into this response so graph layout needs no second round trip.
        body = self.client.get(f"{API}/graph" + query(mode="all", parentFiles=True)).json()
        self.assertTrue(body["parent_files"])
        entry = body["parent_files"][0]
        self.assertIsInstance(entry["node_id"], str)
        self.assertTrue(entry["name_hierarchy_serialized"])

    def test_parent_files_are_absent_by_default(self):
        body = self.client.get(f"{API}/graph" + query(mode="all")).json()
        self.assertFalse(body.get("parent_files"))

    def test_unknown_mode_is_rejected(self):
        response = self.client.get(f"{API}/graph" + query(mode="sideways"))
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Unknown graph mode 'sideways'")


class SymbolResolveTest(ProjectDataTestCase):
    def resolve(self, body):
        response = self.client.post(f"{API}/symbols/resolve", body)
        self.assertEqual(response.status, 200)
        return response.json()

    def test_node_ids_to_name_hierarchies(self):
        node_ids = [as_id(node["id"]) for node in self.nodes[:3]]
        body = self.resolve({"node_ids": node_ids})
        self.assertEqual(len(body["nodes"]), len(node_ids))
        for resolved in body["nodes"]:
            self.assertTrue(resolved["name_hierarchy_serialized"])

    def test_node_kinds_only_when_asked(self):
        node_id = as_id(self.nodes[0]["id"])

        without = self.resolve({"node_ids": [node_id]})["nodes"][0]
        self.assertEqual(without["node_kind"], 0)

        with_kinds = self.resolve(
            {"node_ids": [node_id], "include_node_kinds": True})["nodes"][0]
        self.assertNotEqual(with_kinds["node_kind"], 0)

    def test_name_hierarchy_round_trips_back_to_the_same_id(self):
        node = self.nodes[0]
        serialized = self.resolve(
            {"node_ids": [as_id(node["id"])]})["nodes"][0]["name_hierarchy_serialized"]
        body = self.resolve({"name_hierarchies": [serialized]})
        self.assertEqual(as_id(body["name_hierarchy_node_ids"][0]), as_id(node["id"]))

    def test_edge_ids(self):
        self.assertTrue(self.edges, "fixture has no edges")
        edge_id = as_id(self.edges[0]["id"])
        body = self.resolve({"edge_ids": [edge_id]})
        self.assertEqual(as_id(body["edges"][0]["id"]), edge_id)
        self.assertGreater(as_id(body["edges"][0]["source_node_id"]), 0)

    def test_file_paths_to_node_ids(self):
        body = self.resolve({"file_paths": [self.a_file()]})
        self.assertGreater(as_id(body["file_path_node_ids"][0]), 0)

    def test_unknown_file_path_resolves_to_zero(self):
        body = self.resolve({"file_paths": ["/nonexistent/nope.cpp"]})
        self.assertEqual(as_id(body["file_path_node_ids"][0]), 0)

    def test_location_ids_to_node_ids(self):
        token = self.a_symbol_token()
        collection = self.client.get(f"{API}/locations" + query(tokens=[token])).json()
        location_ids = [as_id(location["location_id"])
                        for file in collection["files"] for location in file["locations"]]
        if not location_ids:
            self.skipTest("fixture symbol has no source locations")
        body = self.resolve({"location_ids": location_ids[:3]})
        self.assertTrue(body["location_node_ids"])

    def test_search_match_token_ids(self):
        body = self.resolve({"search_match_token_ids": [self.a_symbol_token()]})
        self.assertTrue(body["search_matches"])
        self.assertTrue(body["search_matches"][0]["name"])

    def test_parent_file_node_ids(self):
        node_ids = [as_id(node["id"]) for node in self.nodes[:10]]
        body = self.resolve({"parent_file_node_ids": node_ids})
        for entry in body["parent_files"]:
            self.assertIsInstance(entry["parent_file_id"], str)

    def test_an_empty_request_resolves_nothing(self):
        # Repeated fields are always spelled out, so this is empty lists rather than {}.
        body = self.resolve({})
        self.assertFalse(any(body.values()))

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/symbols/resolve", "not json at all")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed resolve request")


class ActiveTokensTest(ProjectDataTestCase):
    def test_returns_ids_and_a_declaration(self):
        token = self.a_symbol_token()
        body = self.client.get(f"{API}/tokens/active" + query(id=token)).json()
        self.assertIn(token, [as_id(value) for value in body["ids"]])
        self.assertEqual(as_id(body["declaration_id"]), token)

    def test_unknown_id_answers_empty(self):
        body = self.client.get(f"{API}/tokens/active" + query(id=999999999)).json()
        self.assertFalse(body["ids"])


class SearchTest(ProjectDataTestCase):
    def test_finds_a_known_symbol(self):
        body = self.client.get(
            f"{API}/search" + query(q="Player", types=self.node_mask)).json()
        names = [match["name"] for match in body["matches"]]
        self.assertIn("Player", names)

    def test_match_fields_are_populated(self):
        match = self.client.get(
            f"{API}/search" + query(q="Player", types=self.node_mask)).json()["matches"][0]
        self.assertTrue(match["token_ids"])
        self.assertIsInstance(match["token_ids"][0], str)
        self.assertTrue(match["token_names_serialized"])
        self.assertTrue(match["type_name"])

    def test_an_empty_type_mask_matches_nothing(self):
        # The mask is a NodeTypeSet; zero is the empty set, so callers must pass a real mask.
        body = self.client.get(f"{API}/search" + query(q="Player", types=0)).json()
        self.assertFalse(body["matches"])

    def test_unmatchable_query_is_empty_not_an_error(self):
        response = self.client.get(
            f"{API}/search" + query(q="zzzznosuchsymbolzzzz", types=self.node_mask))
        self.assertEqual(response.status, 200)
        self.assertFalse(response.json()["matches"])

    def test_empty_query_matches_everything(self):
        # Regression: an empty query matches every entry while matching no character within
        # it, so scoreText got an empty index list and read indices[0] off the end -- which
        # segfaulted the whole engine process. The GUI reaches this just by clearing the
        # search box, so it took the engine down with it.
        for label, target in [("empty q", query(q="", types=self.node_mask)),
                              ("absent q", query(types=self.node_mask)),
                              ("empty q with commands",
                               query(q="", types=self.node_mask, commands=True))]:
            with self.subTest(case=label):
                response = self.client.get(f"{API}/search" + target)
                self.assertEqual(response.status, 200)
                self.assertTrue(response.json()["matches"])

    def test_engine_survives_an_empty_query(self):
        self.client.get(f"{API}/search" + query(q="", types=self.node_mask))
        self.assertIsNone(self.engine.process.poll(), "engine died serving an empty query")
        self.assertEqual(self.client.get(f"{API}/capabilities").status, 200)

    def test_commands_are_included_when_asked(self):
        without = self.client.get(
            f"{API}/search" + query(q="", types=self.node_mask)).json()["matches"]
        with_commands = self.client.get(
            f"{API}/search" + query(q="", types=self.node_mask, commands=True)).json()["matches"]
        self.assertGreater(len(with_commands), len(without))


class FullTextSearchTest(ProjectDataTestCase):
    def test_finds_text_in_the_sources(self):
        body = self.client.get(f"{API}/search/fulltext" + query(q="Player")).json()
        self.assertTrue(body["files"])

    def test_case_sensitive_variant(self):
        response = self.client.get(f"{API}/search/fulltext" + query(q="Player", case=True))
        self.assertEqual(response.status, 200)

    def test_unmatchable_query_is_empty(self):
        body = self.client.get(
            f"{API}/search/fulltext" + query(q="zzzznosuchtextzzzz")).json()
        self.assertFalse(body.get("files"))


class FileTest(ProjectDataTestCase):
    def test_returns_content_locations_info_and_errors(self):
        body = self.client.get(f"{API}/files/" + self.encoded(self.a_file())).json()
        self.assertTrue(body["content"])
        self.assertTrue(body["locations"]["locations"])
        self.assertTrue(body["info"]["file_path"])
        self.assertIn("errors", body)

    def test_include_narrows_to_content(self):
        # Opening a file used to cost four round trips; `include` is what lets a caller that
        # wants one part avoid paying for the rest.
        body = self.client.get(
            f"{API}/files/" + self.encoded(self.a_file()) + query(include="content")).json()
        self.assertTrue(body["content"])
        # Unset submessages are omitted outright, so the narrowed parts are absent, not empty.
        self.assertNotIn("locations", body)
        self.assertNotIn("info", body)

    def test_include_narrows_to_locations(self):
        body = self.client.get(
            f"{API}/files/" + self.encoded(self.a_file()) + query(include="locations")).json()
        self.assertEqual(body["content"], "")
        self.assertTrue(body["locations"]["locations"])

    def test_line_range_narrows_the_locations(self):
        path = self.encoded(self.a_file())
        whole = self.client.get(f"{API}/files/" + path + query(include="locations")).json()
        ranged = self.client.get(
            f"{API}/files/" + path + query(include="locations", lines="1,3")).json()
        self.assertLessEqual(len(ranged["locations"]["locations"]),
                             len(whole["locations"]["locations"]))
        self.assertFalse(ranged["locations"]["is_whole"])

    def test_single_line_range(self):
        response = self.client.get(
            f"{API}/files/" + self.encoded(self.a_file()) + query(include="locations", lines="5"))
        self.assertEqual(response.status, 200)

    def test_malformed_line_range_is_rejected(self):
        # Guarded explicitly: an unguarded stoull here would escape the handler as a 500, where
        # every other malformed parameter on this route answers 400.
        for lines in ["abc", "1,abc", "-"]:
            with self.subTest(lines=lines):
                response = self.client.get(
                    f"{API}/files/" + self.encoded(self.a_file()) + query(lines=lines))
                self.assertEqual(response.status, 400)
                self.assertEqual(response.error, "Malformed line range")

    def test_location_type_narrows_the_locations(self):
        response = self.client.get(
            f"{API}/files/" + self.encoded(self.a_file())
            + query(include="locations", locationType=LOCATION_TOKEN))
        self.assertEqual(response.status, 200)

    def test_unindexed_file_still_reports_its_path(self):
        # The code view titles itself from this path, so an unindexed file must still answer.
        body = self.client.get(
            f"{API}/files/%2Ftmp%2Fnot%20indexed.cpp" + query(include="locations")).json()
        self.assertEqual(body["locations"]["file_path"], "/tmp/not indexed.cpp")
        self.assertFalse(body["locations"]["is_indexed"])

    def test_percent_encoding_is_decoded(self):
        body = self.client.get(
            f"{API}/files/%2Ftmp%2Fa%20b.cpp" + query(include="locations")).json()
        self.assertEqual(body["locations"]["file_path"], "/tmp/a b.cpp")

    def test_plus_decodes_to_a_space(self):
        body = self.client.get(
            f"{API}/files/%2Ftmp%2Fa+b.cpp" + query(include="locations")).json()
        self.assertEqual(body["locations"]["file_path"], "/tmp/a b.cpp")

    def test_missing_path_is_rejected(self):
        response = self.client.get(f"{API}/files/")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing file path")


class FileInfoTest(ProjectDataTestCase):
    def test_batch_lookup(self):
        response = self.client.post(f"{API}/files/info", {"file_paths": [self.a_file()]})
        self.assertEqual(response.status, 200)
        self.assertTrue(response.json()["file_infos"])

    def test_unknown_paths_are_skipped(self):
        body = self.client.post(
            f"{API}/files/info",
            {"file_paths": [self.a_file(), "/nonexistent/nope.cpp"]}).json()
        self.assertEqual(len(body["file_infos"]), 1)

    def test_empty_list(self):
        response = self.client.post(f"{API}/files/info", {"file_paths": []})
        self.assertEqual(response.status, 200)
        self.assertFalse(response.json().get("file_infos"))

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/files/info", "[[[")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed file info request")


class LocationsTest(ProjectDataTestCase):
    def test_by_token_ids(self):
        body = self.client.get(f"{API}/locations" + query(tokens=[self.a_symbol_token()])).json()
        self.assertTrue(body["files"])
        location = body["files"][0]["locations"][0]
        self.assertIsInstance(location["location_id"], str)

    def test_by_location_ids(self):
        collection = self.client.get(
            f"{API}/locations" + query(tokens=[self.a_symbol_token()])).json()
        location_ids = [as_id(location["location_id"])
                        for file in collection["files"] for location in file["locations"]]
        body = self.client.get(f"{API}/locations" + query(locations=location_ids[:3])).json()
        self.assertTrue(body["files"])

    def test_malformed_ids_are_dropped_and_the_rest_still_answer(self):
        # idList() drops a bad entry rather than failing the request: the caller asked about a
        # set, and the rest of the set is still answerable.
        token = self.a_symbol_token()
        body = self.client.get(f"{API}/locations?tokens=bogus,{token}").json()
        self.assertTrue(body["files"])

    def test_neither_selector_is_rejected(self):
        response = self.client.get(f"{API}/locations")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Provide either tokens or locations")

    def test_unknown_ids_answer_empty(self):
        body = self.client.get(f"{API}/locations" + query(tokens=[999999999])).json()
        self.assertFalse(body.get("files"))


class TooltipTest(ProjectDataTestCase):
    def test_by_token_ids(self):
        body = self.client.get(f"{API}/tooltip" + query(
            tokens=[self.a_symbol_token()], origin=TOOLTIP_ORIGIN_CODE)).json()
        self.assertTrue(body["info"]["title"])

    def test_by_location_ids(self):
        collection = self.client.get(
            f"{API}/locations" + query(tokens=[self.a_symbol_token()])).json()
        location_ids = [as_id(location["location_id"])
                        for file in collection["files"] for location in file["locations"]]
        response = self.client.get(
            f"{API}/tooltip" + query(locations=location_ids[:1], locals=[]))
        self.assertEqual(response.status, 200)

    def test_neither_selector_is_rejected(self):
        response = self.client.get(f"{API}/tooltip")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Provide either tokens or locations")


class ErrorsTest(ProjectDataTestCase):
    def test_unfiltered(self):
        response = self.client.get(f"{API}/errors")
        self.assertEqual(response.status, 200)
        self.assertIn("errors", response.json())

    def test_each_filter_flag_is_accepted(self):
        for flag in ["error", "fatal", "unindexedError", "unindexedFatal"]:
            with self.subTest(flag=flag):
                response = self.client.get(f"{API}/errors" + query(**{flag: False}))
                self.assertEqual(response.status, 200)

    def test_limit_caps_the_result(self):
        body = self.client.get(f"{API}/errors" + query(limit=1)).json()
        self.assertLessEqual(len(body.get("errors", [])), 1)

    def test_scoped_to_one_file(self):
        response = self.client.get(f"{API}/errors" + query(file=self.a_file()))
        self.assertEqual(response.status, 200)

    def test_all_filters_off(self):
        response = self.client.get(f"{API}/errors" + query(
            error=False, fatal=False, unindexedError=False, unindexedFatal=False))
        self.assertEqual(response.status, 200)
        self.assertFalse(response.json().get("errors"))


class ErrorLocationsTest(ProjectDataTestCase):
    def test_synthesised_entries(self):
        # This request carries only the four fields the location synthesis reads, not whole
        # ErrorInfo rows.
        response = self.client.post(f"{API}/errors/locations", {
            "errors": [{"id": "1", "file_path": self.a_file(),
                        "line_number": "1", "column_number": "1"}]})
        self.assertEqual(response.status, 200)

    def test_empty_list(self):
        response = self.client.post(f"{API}/errors/locations", {"errors": []})
        self.assertEqual(response.status, 200)

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/errors/locations", "}{")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed error locations request")


if __name__ == "__main__":
    unittest.main()
