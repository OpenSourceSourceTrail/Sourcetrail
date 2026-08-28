#!/usr/bin/env python3
"""MCP server exposing the Sourcetrail code index to an LLM.

Sourcetrail's Clang index knows things grep cannot: exact symbol resolution, and the real
reference graph (who calls this, who writes this field, what inherits from this). Until now only
the Qt GUI consumed it. This server puts it behind four read-only MCP tools.

It is a translation layer, nothing more. `sourcetrail_engine` is already a standalone HTTP+JSON
daemon with the whole read surface exposed, so this spawns one, loads a project into it, and maps
MCP tool calls onto routes that already exist. No C++ changes.

Run it via an MCP client config (see README.md), or by hand:

    python3 mcp/sourcetrail_mcp.py --engine build/app/sourcetrail_engine \
                                   --project path/to/project.srctrlprj
"""

import argparse
import atexit
import os
import sys
import threading
import urllib.parse

from mcp.server.mcpserver import MCPServer

# The engine harness in the integration tests already spawns the engine, parses the
# `ENGINE_PORT <port> <token>` handshake, waits for the listener and shuts it down cleanly -- and
# it is stdlib-only, so importing it costs nothing. If this coupling ever hurts, move Client and
# EngineProcess here and have the tests import them from this file instead.
_HARNESS = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "tests", "integration", "engine_http")
sys.path.insert(0, _HARNESS)

# EngineProcess raises unittest.SkipTest when handed binary=None; unreachable here because
# _config() resolves and validates the path before we construct one.
from engine_harness import API, EngineProcess, as_id, query    # noqa: E402

# --- enums, mirrored from the C++ -------------------------------------------------------------
# Ints on the wire, names for the LLM.

# src/lib/lib/data/graph/Edge.h
EDGE_KINDS = {
    1 << 0: "MEMBER",
    1 << 1: "TYPE_USAGE",
    1 << 2: "USAGE",
    1 << 3: "CALL",
    1 << 4: "INHERITANCE",
    1 << 5: "OVERRIDE",
    1 << 6: "TYPE_ARGUMENT",
    1 << 7: "TEMPLATE_SPECIALIZATION",
    1 << 8: "INCLUDE",
    1 << 9: "IMPORT",
    1 << 10: "BUNDLED_EDGES",
    1 << 11: "MACRO_USAGE",
    1 << 12: "ANNOTATION_USAGE",
}

# src/lib/lib/data/NodeKind.h -- 0 is deliberately unused there, so it can be a bitmask.
NODE_KINDS = {
    1 << 0: "SYMBOL",
    1 << 1: "TYPE",
    1 << 2: "BUILTIN_TYPE",
    1 << 3: "MODULE",
    1 << 4: "NAMESPACE",
    1 << 5: "PACKAGE",
    1 << 6: "STRUCT",
    1 << 7: "CLASS",
    1 << 8: "INTERFACE",
    1 << 9: "ANNOTATION",
    1 << 10: "GLOBAL_VARIABLE",
    1 << 11: "FIELD",
    1 << 12: "FUNCTION",
    1 << 13: "METHOD",
    1 << 14: "ENUM",
    1 << 15: "ENUM_CONSTANT",
    1 << 16: "TYPEDEF",
    1 << 17: "TYPE_PARAMETER",
    1 << 18: "FILE",
    1 << 19: "MACRO",
    1 << 20: "UNION",
}

# src/lib/lib/data/location/LocationType.h
LOCATION_TOKEN = 0
LOCATION_SCOPE = 1

# TooltipOrigin::TOOLTIP_ORIGIN_CODE
TOOLTIP_ORIGIN_CODE = 1


def edge_kind(value):
    return EDGE_KINDS.get(value, f"UNKNOWN({value})")


def node_kind(value):
    return NODE_KINDS.get(value, f"UNKNOWN({value})")


def parse_name(serialized):
    """Renders a serialized NameHierarchy as a readable qualified name.

    Mirrors NameHierarchy::deserialize (src/lib/lib/data/name/NameHierarchy.cpp): the string is
    `<delimiter>\\tm` followed by parts joined by `\\tn`, each part being
    `<name>\\ts<prefix>\\tp<postfix>`. Only the signature-less name is kept -- the prefix/postfix
    are return type and parameter list, which the tooltip renders better.
    """
    meta = serialized.find("\tm")
    if meta < 0:
        return serialized
    delimiter = serialized[:meta] or "::"
    names = []
    for part in serialized[meta + 2:].split("\tn"):
        name, _, _rest = part.partition("\ts")
        if name:
            names.append(name)
    return delimiter.join(names)


# --- engine ------------------------------------------------------------------------------------


class Engine:
    """One lazily started engine with a project loaded, plus a file-content cache."""

    def __init__(self, binary, project):
        self.binary = binary
        self.project = project
        self._lock = threading.Lock()
        self._process = None
        self._node_mask = None
        self._files = {}

    def client(self):
        """Starts the engine and loads the project on first use.

        Lazy on purpose: an MCP client that only lists tools should not pay a cold start, which
        includes constructing the Application and discovering indexer plugins.
        """
        with self._lock:
            if self._process is None:
                process = EngineProcess(port=0, binary=self.binary)
                response = process.load_project(self.project)
                body = response.json() if response.status == 200 else {}
                if not body.get("loaded"):
                    process.stop()
                    reason = body.get("error_message") or f"status {response.status}"
                    raise RuntimeError(f"could not load {self.project}: {reason}")
                atexit.register(process.stop)
                self._process = process
            return self._process.client

    def get(self, path, **params):
        response = self.client().get(path + query(**params))
        if response.status != 200:
            raise RuntimeError(f"GET {path} failed: {response.error or response.status}")
        return response.json()

    def post(self, path, body):
        response = self.client().post(path, body)
        if response.status != 200:
            raise RuntimeError(f"POST {path} failed: {response.error or response.status}")
        return response.json()

    def node_mask(self):
        """The set of node kinds this project actually contains.

        `types` on /search is a NodeTypeSet mask and a zero mask matches *nothing*, so the mask
        has to come from the engine rather than be assumed.
        """
        if self._node_mask is None:
            self._node_mask = self.get(f"{API}/stats", include="types")["available_node_types"]
        return self._node_mask

    def lines(self, path):
        """The file's lines, from the engine's copy. Memoized -- reference lists hit one file
        many times over.

        ponytail: unbounded cache, keyed by path. Bound it if someone points this at a project
        big enough for the whole corpus to land in it.
        """
        if path not in self._files:
            body = self.get(f"{API}/files/{urllib.parse.quote(path, safe='')}", include="content")
            self._files[path] = body.get("content", "").splitlines()
        return self._files[path]

    def line(self, path, number):
        """One 1-based source line, stripped. Empty when out of range."""
        lines = self.lines(path)
        return lines[number - 1].strip() if 1 <= number <= len(lines) else ""


ENGINE = None
mcp = MCPServer("sourcetrail")


# --- tools -------------------------------------------------------------------------------------


@mcp.tool()
def search_symbols(query: str, limit: int = 20) -> list[dict]:
    """Find indexed symbols by name (fuzzy). Start here: other tools take a symbol id.

    Returns the matches with their `id`, readable `name` and `kind` (CLASS, METHOD, FIELD, FILE,
    ...), best match first. Pass an id to describe_symbol for the signature and definition, or to
    find_references for the use sites.

    A fuzzy match can expand to several ids -- distinct symbols in different scopes that share the
    same indexed text, e.g. one `main` per translation unit -- so this can return more than `limit`
    entries; `limit` bounds distinct fuzzy matches, not result rows.
    """
    body = ENGINE.get(f"{API}/search", q=query,
                      types=ENGINE.node_mask(), commands=False)
    results = []
    for match in body.get("matches", [])[:limit]:
        token_ids = match.get("token_ids") or []
        if not token_ids:
            continue
        # `name` is the display name -- the full path for a FILE node, the qualified name
        # otherwise. The signature is not here; it comes from the tooltip in describe_symbol.
        # token_names_serialized is index-aligned with token_ids (ConvertQuery.cpp) and
        # disambiguates ids that collapsed under the same match text, e.g. same-named symbols
        # in different scopes/files; fall back to the match's own name if it's missing.
        token_names = match.get("token_names_serialized") or []
        kind = node_kind(match.get("node_kind", 0))
        score = match.get("score", 0)
        for i, token_id in enumerate(token_ids):
            name = parse_name(token_names[i]) if i < len(token_names) else match.get("name", "")
            results.append({"id": token_id, "name": name, "kind": kind, "score": score})
    return results


@mcp.tool()
def describe_symbol(symbol_id: str) -> dict:
    """What a symbol is and where it is defined, with its source.

    Takes an id from search_symbols or find_references. Returns the kind, the qualified name, the
    declaration signature, and the definition's file, line span and body text.
    """
    resolved = ENGINE.post(f"{API}/symbols/resolve", {
        "node_ids": [symbol_id],
        "include_node_kinds": True,
        "parent_file_node_ids": [symbol_id],
    })
    nodes = resolved.get("nodes") or []
    if not nodes:
        raise RuntimeError(f"no symbol with id {symbol_id}")
    node = nodes[0]

    info = {
        "id": symbol_id,
        "name": parse_name(node.get("name_hierarchy_serialized", "")),
        "kind": node_kind(node.get("node_kind", 0)),
        "signature": "",
        "definition": None,
    }

    parents = resolved.get("parent_files") or []
    if parents:
        info["declared_in"] = parse_name(parents[0].get("name_hierarchy_serialized", ""))

    tooltip = ENGINE.get(f"{API}/tooltip", tokens=[symbol_id], origin=TOOLTIP_ORIGIN_CODE)
    snippets = (tooltip.get("info") or {}).get("snippets") or []
    if snippets:
        info["signature"] = snippets[0].get("code", "").strip()

    # LOCATION_SCOPE is the definition body span; LOCATION_TOKEN is just the name. Prefer the
    # scope so the caller gets a whole function, falling back to the name's line.
    locations = ENGINE.get(f"{API}/locations", tokens=[symbol_id])
    best = None
    for source_file in locations.get("files") or []:
        for location in source_file.get("locations") or []:
            if location.get("type") == LOCATION_SCOPE:
                best = (source_file["file_path"], location)
                break
            if best is None and location.get("type") == LOCATION_TOKEN:
                best = (source_file["file_path"], location)
        if best is not None and best[1].get("type") == LOCATION_SCOPE:
            break

    if best is not None:
        path, location = best
        start = as_id(location["start_line"])
        end = as_id(location["end_line"])
        info["definition"] = {
            "file": path,
            "start_line": start,
            "end_line": end,
            "code": "\n".join(ENGINE.lines(path)[start - 1:end]),
        }
    return info


@mcp.tool()
def find_references(symbol_id: str, kinds: list[str] | None = None,
                    limit: int = 50) -> dict:
    """Every place a symbol is used, with the source line at each site.

    This is how you answer "how is this variable created?" or "who calls this?": ask for the
    symbol's references and read the `incoming` USAGE and CALL sites.

    `incoming` is what points at the symbol (callers, writers, subclasses); `outgoing` is what the
    symbol points at (what it calls, the types it uses, its members). Filter with `kinds`, e.g.
    ["CALL"], ["USAGE", "TYPE_USAGE"], ["INHERITANCE", "OVERRIDE"]; omit it for everything.
    """
    graph = ENGINE.get(f"{API}/graph", mode="active", tokens=[symbol_id])["graph"]
    names = {node["id"]: parse_name(node.get("name_hierarchy_serialized", ""))
             for node in graph.get("nodes") or []}

    wanted = {kind.upper() for kind in kinds} if kinds else None
    edges = []
    for edge in graph.get("edges") or []:
        kind = edge_kind(edge.get("type", 0))
        if wanted is not None and kind not in wanted:
            continue
        incoming = edge.get("to_node_id") == symbol_id
        other = edge.get("from_node_id") if incoming else edge.get("to_node_id")
        edges.append((edge["id"], kind, incoming, names.get(other, "?")))
        if len(edges) >= limit:
            break

    if not edges:
        return {"incoming": [], "outgoing": []}

    # One batched call for every edge's location rather than one per edge.
    sites = {}
    locations = ENGINE.get(f"{API}/locations", tokens=[edge_id for edge_id, _, _, _ in edges])
    for source_file in locations.get("files") or []:
        for location in source_file.get("locations") or []:
            line = as_id(location["start_line"])
            for token_id in location.get("token_ids") or []:
                sites.setdefault(token_id, (source_file["file_path"], line))

    result = {"incoming": [], "outgoing": []}
    for edge_id, kind, incoming, other in edges:
        entry = {"kind": kind, "symbol": other}
        if edge_id in sites:
            path, line = sites[edge_id]
            entry.update(file=path, line=line, code=ENGINE.line(path, line))
        result["incoming" if incoming else "outgoing"].append(entry)
    return result


@mcp.tool()
def read_source(path: str, start_line: int = 1, end_line: int | None = None) -> str:
    """Read an indexed source file, or a 1-based inclusive line range of one.

    Reads through the engine, which serves the content it indexed -- so this still works when the
    file has since moved or changed on disk. Other tools hand back file+line coordinates; this is
    how you look at them.
    """
    lines = ENGINE.lines(path)
    if not lines:
        raise RuntimeError(f"no content for {path}")
    stop = len(lines) if end_line is None else end_line
    return "\n".join(lines[max(start_line - 1, 0):stop])


# --- entry point -------------------------------------------------------------------------------


def config(argv=None):
    """Resolves the engine binary and project, from flags or the environment."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", default=os.environ.get("SOURCETRAIL_ENGINE"),
                        help="path to the sourcetrail_engine binary "
                             "[$SOURCETRAIL_ENGINE]")
    parser.add_argument("--project", default=os.environ.get("SOURCETRAIL_PROJECT"),
                        help="path to the .srctrlprj to serve [$SOURCETRAIL_PROJECT]")
    args = parser.parse_args(argv)

    if not args.engine or not os.path.isfile(args.engine):
        parser.error(f"--engine must point at the sourcetrail_engine binary, got {args.engine!r}")
    if not args.project or not os.path.isfile(args.project):
        parser.error(f"--project must point at a .srctrlprj file, got {args.project!r}")
    return Engine(os.path.abspath(args.engine), os.path.abspath(args.project))


def main(argv=None):
    global ENGINE
    ENGINE = config(argv)
    mcp.run()


if __name__ == "__main__":
    main()
