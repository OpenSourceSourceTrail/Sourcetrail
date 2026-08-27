"""Cross-cutting HTTP behaviour of the engine: auth, Origin, routing, status codes.

These mirror src/lib/core/http/tests/HttpServerTestSuite.cpp, but against the real engine
process rather than an http::Server built inside the test binary.
"""

import unittest

from engine_harness import API, EngineTestCase, query


class AuthTest(EngineTestCase):
    def test_valid_token_is_accepted(self):
        self.assertEqual(self.client.get(f"{API}/capabilities").status, 200)

    def test_missing_token_is_rejected(self):
        response = self.client.get(f"{API}/capabilities", token=None)
        self.assertEqual(response.status, 401)
        self.assertEqual(response.error, "Missing or invalid bearer token")

    def test_wrong_token_is_rejected(self):
        response = self.client.get(f"{API}/capabilities", token="not-the-token")
        self.assertEqual(response.status, 401)
        self.assertEqual(response.error, "Missing or invalid bearer token")

    def test_malformed_authorization_header_is_rejected(self):
        for header in ["", "Bearer", "Basic " + self.engine.token, self.engine.token]:
            with self.subTest(header=header):
                response = self.client.request(
                    "GET", f"{API}/capabilities", headers={"Authorization": header}, token=None)
                self.assertEqual(response.status, 401)

    def test_refusal_closes_the_connection(self):
        # The server sets keep_alive(false) on a refusal, so the socket must not be reusable.
        response = self.client.get(f"{API}/capabilities", token="wrong")
        self.assertEqual(response.status, 401)
        self.assertEqual(response.header("connection"), "close")

    def test_every_route_requires_the_token(self):
        for method, path in [("GET", f"{API}/stats"),
                             ("GET", f"{API}/project"),
                             ("GET", f"{API}/bookmarks"),
                             ("POST", f"{API}/project/refresh")]:
            with self.subTest(path=path):
                self.assertEqual(self.client.request(method, path, token=None).status, 401)


class OriginTest(EngineTestCase):
    def test_any_origin_is_refused(self):
        # Server::allowOrigin() is never called in production code, so today every request
        # carrying an Origin -- i.e. every browser-issued one -- is refused outright.
        for origin in ["http://evil.example", "http://localhost:3000", "null"]:
            with self.subTest(origin=origin):
                response = self.client.get(f"{API}/capabilities", headers={"Origin": origin})
                self.assertEqual(response.status, 403)
                self.assertEqual(response.error, "Origin not allowed")

    def test_origin_is_checked_before_the_token(self):
        response = self.client.get(
            f"{API}/capabilities", token=None, headers={"Origin": "http://evil.example"})
        self.assertEqual(response.status, 403)

    def test_no_cors_headers_are_emitted(self):
        response = self.client.get(f"{API}/capabilities")
        self.assertIsNone(response.header("access-control-allow-origin"))


class PreflightTest(EngineTestCase):
    def test_options_succeeds_without_a_token(self):
        # Preflight carries no Authorization header by design, so it is answered before auth.
        response = self.client.request("OPTIONS", f"{API}/capabilities", token=None)
        self.assertEqual(response.status, 204)
        self.assertEqual(response.body, "")

    def test_options_on_an_unknown_path_also_succeeds(self):
        response = self.client.request("OPTIONS", f"{API}/no/such/thing", token=None)
        self.assertEqual(response.status, 204)


class RoutingTest(EngineTestCase):
    def test_unknown_path_is_404(self):
        response = self.client.get(f"{API}/definitely-not-a-route")
        self.assertEqual(response.status, 404)
        self.assertEqual(response.error, "No such endpoint")

    def test_wrong_method_is_404_not_405(self):
        # findRoute keys on the (method, path) pair, so a wrong method simply fails to match.
        # A client author would reasonably guess 405; pin the real contract.
        for method, path in [("DELETE", f"{API}/capabilities"),
                             ("POST", f"{API}/capabilities"),
                             ("GET", f"{API}/shutdown"),
                             ("PUT", f"{API}/stats")]:
            with self.subTest(method=method, path=path):
                response = self.client.request(method, path)
                self.assertEqual(response.status, 404)
                self.assertEqual(response.error, "No such endpoint")

    def test_exact_route_beats_a_prefix_route(self):
        # /bookmarks is exact, /bookmarks/ is a prefix; the exact one must win.
        self.assertEqual(self.client.get(f"{API}/bookmarks").status, 200)

    def test_longest_prefix_wins(self):
        # /bookmarks/categories/ and /bookmarks/ are both DELETE prefixes. The longer one must
        # take the request, otherwise a category id would be deleted as a bookmark id.
        # Registration order in registerRoutes() is what makes this hold -- guard it.
        response = self.client.delete(f"{API}/bookmarks/categories/not-a-number")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing bookmark category id")

        response = self.client.delete(f"{API}/bookmarks/not-a-number")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing bookmark id")

    def test_prefix_route_captures_the_remainder(self):
        response = self.client.get(f"{API}/files/")
        self.assertEqual(response.status, 400)
        self.assertEqual(response.error, "Missing file path")


class QueryParsingTest(EngineTestCase):
    def test_percent_encoded_path_reaches_the_handler(self):
        # That the decoding is *correct* is asserted in test_queries, where a loaded project
        # makes the engine echo the decoded path back; here we only need it to route.
        response = self.client.get(f"{API}/files/%2Ftmp%2Fa%20b.cpp" + query(include="locations"))
        self.assertEqual(response.status, 200)

    def test_valueless_key_reads_as_true(self):
        # getBool treats a present-but-empty value as true, so `?commands` == `?commands=true`.
        self.assertEqual(self.client.get(f"{API}/search?q=x&commands").status, 200)

    def test_non_numeric_uint_falls_back_silently(self):
        # getUInt swallows the parse error and returns its fallback rather than failing.
        response = self.client.get(f"{API}/tokens/active?id=not-a-number")
        self.assertEqual(response.status, 200)

    def test_malformed_ids_in_a_list_are_dropped(self):
        # idList() deliberately drops a bad entry instead of failing the whole request: the
        # caller asked about a set, and the rest of the set is still answerable.
        response = self.client.get(f"{API}/locations?tokens=1,bogus,2")
        self.assertEqual(response.status, 200)

    def test_absent_query_parameter_uses_the_fallback(self):
        self.assertEqual(self.client.get(f"{API}/errors").status, 200)


class ResponseShapeTest(EngineTestCase):
    def test_content_type_is_json(self):
        for path in [f"{API}/capabilities", f"{API}/stats", f"{API}/project", f"{API}/bookmarks"]:
            with self.subTest(path=path):
                response = self.client.get(path)
                self.assertEqual(response.header("content-type"), "application/json")

    def test_error_bodies_are_json_with_an_error_key(self):
        response = self.client.get(f"{API}/nope")
        self.assertEqual(response.header("content-type"), "application/json")
        self.assertIn("error", response.json())

    def test_empty_responses_are_an_empty_json_object(self):
        response = self.client.post(f"{API}/project/indexing/cancel")
        self.assertEqual(response.status, 200)
        self.assertEqual(response.json(), {})

    def test_defaults_are_spelled_out(self):
        # always_print_primitive_fields=true, so a field at its default is still present.
        body = self.client.get(f"{API}/project").json()
        for key in ["loaded", "indexing", "reindexable", "description"]:
            self.assertIn(key, body)

    def test_field_names_are_snake_case(self):
        # preserve_proto_field_names=true -- a deliberate departure from canonical proto JSON,
        # which would render these lowerCamelCase.
        body = self.client.get(f"{API}/capabilities").json()
        self.assertIn("can_create_project", body)
        self.assertIn("engine_version", body)
        self.assertNotIn("canCreateProject", body)


class KeepAliveTest(EngineTestCase):
    def test_one_connection_serves_several_requests(self):
        client = type(self.client)(self.engine.port, self.engine.token)
        try:
            for _ in range(5):
                self.assertEqual(client.get(f"{API}/capabilities", reuse=True).status, 200)
        finally:
            client.close()

    def test_mixed_methods_over_one_connection(self):
        client = type(self.client)(self.engine.port, self.engine.token)
        try:
            self.assertEqual(client.get(f"{API}/stats", reuse=True).status, 200)
            self.assertEqual(client.post(f"{API}/project/refresh", {}, reuse=True).status, 200)
            self.assertEqual(client.get(f"{API}/bookmarks", reuse=True).status, 200)
            self.assertEqual(client.get(f"{API}/nope", reuse=True).status, 404)
            self.assertEqual(client.get(f"{API}/capabilities", reuse=True).status, 200)
        finally:
            client.close()


if __name__ == "__main__":
    unittest.main()
