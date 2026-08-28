"""Bookmark routes -- the only genuinely mutating storage the HTTP API exposes.

Everything here runs against a temp-dir copy of a shipped sample project, so the .srctrlbm
tracked in the repo is never written to.
"""

import unittest

from engine_harness import API, LoadedProjectTestCase, as_id, query


class BookmarkTestCase(LoadedProjectTestCase):
    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        graph = cls.client.get(f"{API}/graph" + query(mode="all")).json()
        cls.node_ids = [as_id(node["id"]) for node in graph["graph"]["nodes"][:2]]

        # An edge bookmark stores the serialized names of the edge's endpoints, so it needs a
        # real edge -- and mode=all is the overview graph, which carries none.
        node_mask = cls.client.get(f"{API}/stats").json()["available_node_types"]
        matches = cls.client.get(
            f"{API}/search" + query(q="Player", types=node_mask)).json()["matches"]
        around = cls.client.get(f"{API}/graph" + query(
            mode="active", tokens=[as_id(matches[0]["token_ids"][0])])).json()["graph"]
        cls.edge_id = as_id(around["edges"][0]["id"])

    def all_bookmarks(self):
        response = self.client.get(f"{API}/bookmarks")
        self.assertEqual(response.status, 200)
        return response.json()

    def add_node_bookmark(self, name, comment="", category=None):
        base = {"name": name, "comment": comment}
        if category is not None:
            base["category"] = {"name": category}
        response = self.client.post(f"{API}/bookmarks", {
            "base": base,
            "node_ids": [str(node_id) for node_id in self.node_ids[:1]]})
        self.assertEqual(response.status, 200, response.body)
        bookmark_id = as_id(response.json()["id"])
        self.assertNotEqual(bookmark_id, 0, "the bookmark was not stored")
        return bookmark_id

    def add_edge_bookmark(self, name, edge_id=None, category=None):
        edge_id = self.edge_id if edge_id is None else edge_id
        base = {"name": name, "comment": ""}
        if category is not None:
            base["category"] = {"name": category}
        response = self.client.post(f"{API}/bookmarks", {
            "base": base,
            "edge_ids": [str(edge_id)],
            "active_node_id": str(self.node_ids[0])})
        self.assertEqual(response.status, 200, response.body)
        bookmark_id = as_id(response.json()["id"])
        self.assertNotEqual(bookmark_id, 0, "the bookmark was not stored")
        return bookmark_id

    def find(self, bookmarks, bookmark_id):
        for bookmark in bookmarks:
            if as_id(bookmark["base"]["id"]) == bookmark_id:
                return bookmark
        return None


class ListBookmarksTest(BookmarkTestCase):
    def test_lists_all_three_collections(self):
        body = self.all_bookmarks()
        for key in ["node_bookmarks", "edge_bookmarks", "categories"]:
            self.assertIn(key, body)
            self.assertIsInstance(body[key], list)


class AddBookmarkTest(BookmarkTestCase):
    def test_node_bookmark_round_trips(self):
        bookmark_id = self.add_node_bookmark("a node bookmark", comment="why it matters")
        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertIsNotNone(stored)
        self.assertEqual(stored["base"]["name"], "a node bookmark")
        self.assertEqual(stored["base"]["comment"], "why it matters")
        self.assertEqual([as_id(value) for value in stored["node_ids"]], self.node_ids[:1])

    def test_edge_bookmark_round_trips(self):
        # The node/edge branch is chosen purely by edge_ids being non-empty.
        bookmark_id = self.add_edge_bookmark("an edge bookmark")
        body = self.all_bookmarks()
        self.assertIsNotNone(self.find(body["edge_bookmarks"], bookmark_id))
        self.assertIsNone(self.find(body["node_bookmarks"], bookmark_id))

    def test_node_bookmark_does_not_land_in_the_edge_list(self):
        bookmark_id = self.add_node_bookmark("strictly a node bookmark")
        body = self.all_bookmarks()
        self.assertIsNotNone(self.find(body["node_bookmarks"], bookmark_id))
        self.assertIsNone(self.find(body["edge_bookmarks"], bookmark_id))

    def test_category_is_created_with_the_bookmark(self):
        self.add_node_bookmark("categorised", category="A Category")
        names = [category["name"] for category in self.all_bookmarks()["categories"]]
        self.assertIn("A Category", names)

    def test_omitted_category_falls_back_to_default(self):
        # A bookmark row's category_id is a foreign key. Without this substitution the insert
        # violates it, is rolled back, and the caller is handed an id for a bookmark that was
        # never stored -- a silent write loss behind a 200.
        bookmark_id = self.add_node_bookmark("no category given")
        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertIsNotNone(stored, "a bookmark with no category was not stored")
        self.assertEqual(stored["base"]["category"]["name"], "default")

    def test_empty_category_name_falls_back_to_default(self):
        bookmark_id = self.add_node_bookmark("empty category given", category="")
        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertIsNotNone(stored)
        self.assertEqual(stored["base"]["category"]["name"], "default")

    def test_omitted_timestamp_is_stamped_by_the_engine(self):
        # Without this the row stores the literal "not-a-date-time", which then throws on every
        # later read -- one such bookmark makes GET /bookmarks 500 for good.
        bookmark_id = self.add_node_bookmark("no timestamp given")
        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertIsNotNone(stored)
        self.assertRegex(stored["base"]["timestamp"], r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}")

    def test_supplied_timestamp_is_kept(self):
        response = self.client.post(f"{API}/bookmarks", {
            "base": {"name": "stamped", "comment": "", "timestamp": "2020-01-02 03:04:05",
                     "category": {"name": "Stamped"}},
            "node_ids": [str(self.node_ids[0])]})
        bookmark_id = as_id(response.json()["id"])
        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertEqual(stored["base"]["timestamp"], "2020-01-02 03:04:05")

    def test_listing_survives_a_bookmark_created_with_no_optional_fields(self):
        self.add_node_bookmark("bare")
        for _ in range(2):
            self.assertEqual(self.client.get(f"{API}/bookmarks").status, 200)

    def test_ids_are_uint64_strings(self):
        response = self.client.post(f"{API}/bookmarks", {
            "base": {"name": "id shape", "comment": ""},
            "node_ids": [str(self.node_ids[0])]})
        self.assertIsInstance(response.json()["id"], str)

    def test_malformed_body_is_rejected(self):
        response = self.client.post(f"{API}/bookmarks", "not a bookmark")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed bookmark")


class UpdateBookmarkTest(BookmarkTestCase):
    def test_updates_via_the_path_id(self):
        bookmark_id = self.add_node_bookmark("before")
        response = self.client.patch(f"{API}/bookmarks/{bookmark_id}", {
            "name": "after", "comment": "edited"})
        self.assertEqual(response.status, 200)
        self.assertEqual(response.json(), {})

        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertEqual(stored["base"]["name"], "after")
        self.assertEqual(stored["base"]["comment"], "edited")

    def test_updates_via_the_body_id(self):
        # The id may come from the path or from bookmark_id in the body.
        bookmark_id = self.add_node_bookmark("before body update")
        response = self.client.patch(f"{API}/bookmarks/", {
            "bookmark_id": str(bookmark_id), "name": "after body update",
            "comment": ""})
        self.assertEqual(response.status, 200)

        stored = self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id)
        self.assertEqual(stored["base"]["name"], "after body update")

    def test_moves_a_bookmark_into_a_category(self):
        bookmark_id = self.add_node_bookmark("to be categorised")
        response = self.client.patch(f"{API}/bookmarks/{bookmark_id}", {
            "name": "to be categorised", "comment": "", "category_name": "Moved Here"})
        self.assertEqual(response.status, 200)

        names = [category["name"] for category in self.all_bookmarks()["categories"]]
        self.assertIn("Moved Here", names)

    def test_missing_id_is_rejected(self):
        response = self.client.patch(f"{API}/bookmarks/not-a-number", {"name": "x", "comment": ""})
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing bookmark id")

    def test_malformed_body_is_rejected(self):
        response = self.client.patch(f"{API}/bookmarks/1", "><")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Malformed bookmark update")


class DeleteBookmarkTest(BookmarkTestCase):
    def test_deletes_a_node_bookmark(self):
        bookmark_id = self.add_node_bookmark("doomed")
        self.assertIsNotNone(self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id))

        response = self.client.delete(f"{API}/bookmarks/{bookmark_id}")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.json(), {})

        self.assertIsNone(self.find(self.all_bookmarks()["node_bookmarks"], bookmark_id))

    def test_deletes_an_edge_bookmark(self):
        bookmark_id = self.add_edge_bookmark("doomed edge")
        response = self.client.delete(f"{API}/bookmarks/{bookmark_id}")
        self.assertEqual(response.status, 200)
        self.assertIsNone(self.find(self.all_bookmarks()["edge_bookmarks"], bookmark_id))

    def test_deletes_a_category(self):
        self.add_node_bookmark("in a doomed category", category="Doomed Category")
        categories = self.all_bookmarks()["categories"]
        category = next(item for item in categories if item["name"] == "Doomed Category")

        response = self.client.delete(
            f"{API}/bookmarks/categories/{as_id(category['id'])}")
        self.assertEqual(response.status, 200)

        names = [item["name"] for item in self.all_bookmarks()["categories"]]
        self.assertNotIn("Doomed Category", names)

    def test_deleting_a_category_leaves_other_categories_alone(self):
        # Deleting a category cascades to the bookmarks inside it -- but only to those.
        doomed = self.add_node_bookmark("inside the doomed one", category="Cascade Doomed")
        survivor = self.add_node_bookmark("inside the safe one", category="Cascade Safe")

        categories = self.all_bookmarks()["categories"]
        target = next(item for item in categories if item["name"] == "Cascade Doomed")
        self.assertEqual(
            self.client.delete(f"{API}/bookmarks/categories/{as_id(target['id'])}").status, 200)

        remaining = self.all_bookmarks()["node_bookmarks"]
        self.assertIsNone(self.find(remaining, doomed))
        self.assertIsNotNone(self.find(remaining, survivor))

    def test_missing_bookmark_id_is_rejected(self):
        response = self.client.delete(f"{API}/bookmarks/not-a-number")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing bookmark id")

    def test_missing_category_id_is_rejected(self):
        response = self.client.delete(f"{API}/bookmarks/categories/not-a-number")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing bookmark category id")

    def test_deleting_an_unknown_id_is_accepted(self):
        response = self.client.delete(f"{API}/bookmarks/999999999")
        self.assertEqual(response.status, 200)


if __name__ == "__main__":
    unittest.main()
