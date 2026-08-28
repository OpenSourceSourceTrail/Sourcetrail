# Sourcetrail MCP server

Exposes the Sourcetrail code index to an LLM over [MCP](https://modelcontextprotocol.io).

Sourcetrail's Clang index knows things `grep` cannot: exact symbol resolution, and the real
reference graph. Until now only the Qt GUI consumed it. This server puts it behind four read-only
tools, so a coding agent can ask *"how is this field created?"* and get call sites instead of a
regex guess.

It is a thin translation layer over `sourcetrail_engine`'s existing HTTP+JSON API — it spawns an
engine, loads a project into it, and maps tool calls onto routes that already exist. No C++ changes.

## Setup

```
pip install -r mcp/requirements.txt
```

Needs a built `sourcetrail_engine` and a project that has already been indexed (a `.srctrlprj` with
its `.srctrldb` next to it).

## Register it

`.mcp.json` in your repo root, or the equivalent in your MCP client's config:

```json
{
  "mcpServers": {
    "sourcetrail": {
      "command": "python3",
      "args": [
        "mcp/sourcetrail_mcp.py",
        "--engine", "build/app/sourcetrail_engine",
        "--project", "path/to/your_project.srctrlprj"
      ]
    }
  }
}
```

`$SOURCETRAIL_ENGINE` and `$SOURCETRAIL_PROJECT` work as fallbacks for the two flags.

The engine starts on the first tool call, not at startup, so listing tools stays fast.

## Tools

| Tool | Answers |
| --- | --- |
| `search_symbols(query, limit=20)` | "what symbols are called something like this?" — fuzzy, returns ids |
| `describe_symbol(symbol_id)` | "what is this and where is it defined?" — kind, signature, definition source |
| `find_references(symbol_id, kinds=None, limit=50)` | "who uses this?" — every use site with its source line |
| `read_source(path, start_line=1, end_line=None)` | the source at coordinates the other tools return |

`find_references` splits results into `incoming` (what points at the symbol: callers, writers,
subclasses) and `outgoing` (what the symbol points at: what it calls, types it uses, its members).
Filter with `kinds`: `CALL`, `USAGE`, `TYPE_USAGE`, `INHERITANCE`, `OVERRIDE`, `MEMBER`,
`INCLUDE`, `MACRO_USAGE`, and the rest of `Edge::EdgeType`.

Worked example — *"how is `Player::name_` created?"*:

```
search_symbols("name_")                  -> id 1041, kind FIELD
find_references("1041", kinds=["USAGE"]) -> incoming:
    Player::Player   player.cpp:5   ", name_( name ) {"      <- created here
    Player::getName  player.cpp:18  "return name_;"
    HumanPlayer::Turn human_player.cpp:14 "io::stringOut(name_);"
```

Symbol ids are strings, not numbers: the engine's JSON quotes `uint64` (protobuf's canonical
mapping) and that contract is kept end to end rather than risking precision loss.

## Test

```
ctest --test-dir build -R integration.mcp
```

Or directly, from this directory:

```
SOURCETRAIL_ENGINE=../build/app/sourcetrail_engine \
SOURCETRAIL_PROJECTS_DIR=../bin/app/user/projects \
python3 -m unittest -v test_mcp
```

It runs against the shipped `tictactoe_cpp` sample, whose index is prebuilt — so no indexer and no
`BUILD_CXX_LANGUAGE_PACKAGE` needed.

## Notes

- Read-only by design. The agent edits files with its own tools.
- `EngineProcess` and the HTTP client are imported from
  `tests/integration/engine_http/engine_harness.py` rather than duplicated. If that coupling ever
  hurts, move them here and have the tests import them from this directory instead.
- Keep the engine on loopback. Its `/api/v1/files/{path}` route reads any path the engine process
  can reach, and the bearer token is the only thing in front of it — the same posture the GUI runs
  with.
- Deliberately not built yet: full-text search (the agent has `grep`), reindex/project-lifecycle
  tools, and write tools.
