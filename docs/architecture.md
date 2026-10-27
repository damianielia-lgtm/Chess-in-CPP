# Architecture — MultiPLY v1.1

[README](../README.md) · [Engine](engine.md) · [Move generation](movegen.md) · [File formats](file-formats.md)

MultiPLY shares one chess core between a terminal program, a UCI frontend, and diagnostic workloads. The boundaries follow behavior: represent positions, produce legal moves, maintain games, interpret notation, choose moves, and store user artifacts.

## Module map

| Area | Responsibility |
| --- | --- |
| `src/core/` | Squares, pieces, moves, castling rights, board state, and reversible position changes. |
| `src/movegen/` | Precomputed movement tables, attack detection, legal move generation, and fixed-capacity move buffers. |
| `src/game/` | Mutable live game state, historical snapshots, captures, clocks, outcomes, and interactive game/replay/analysis loops. |
| `src/notation/` | FEN validation/serialization, legal SAN/UCI move resolution, PGN parsing/construction, and game reconstruction. |
| `src/interface/` | Typed CLI/UCI commands, parsing/execution, independent frontend sessions, and terminal display. |
| `src/engine/` | Static evaluation, alpha-beta search, timed iterative deepening, ranking, and optional search observers. |
| `src/storage/` | Configuration persistence, managed filenames, file I/O, and diagnostic suite loading. |
| `src/diagnostics/` | Perft, Stockfish comparison, benchmarking, and pruning-profile reports. |
| `src/picker_lab/` | A separate synthetic picker benchmark and Python report plots. |
| `src/main.cpp` | Startup, mode selection, line input, dispatch, and top-level error handling. |

## Position and move flow

A position contains a 64-square array plus side to move, castling rights, en passant target, halfmove/fullmove counters, and cached king locations. Square indexing begins at `a1`; FEN conversion accounts for the opposite rank order used by its board text.

Move generation emits legal moves into reusable fixed-capacity lists. `MovesList` preserves generation order for general callers and perft; `ScoredMoveList` orders search moves using capture, promotion, and piece-square scores supplied by the evaluation module. Movement/attack tables supply board geometry; attack checks enforce king safety and castling restrictions. Special move encodings carry promotion, castling, and en passant behavior. Text input resolves to one of the legal moves before execution. Low-level position mutation assumes the caller supplies an appropriate move; it is not a second user-input validator.

The [move-generation guide](movegen.md) explains the precomputed tables, target-centered attack queries, candidate filtering, and special-move handling with diagrams.

Search and perft use apply/revert rather than copying every child position. Applying returns an undo record for metadata and capture restoration; reverting restores the parent state. Each recursion level owns a separate reusable move list, so child generation cannot overwrite its parent's pending moves.

## Game state and history

A game stores mutable live state alongside immutable historical snapshots. A snapshot captures the position, last move, captures, and available clocks. This supports replay and stepping through history without reversing each move mutation. Removing the latest state rebuilds live state from the remaining snapshot; analysis uses this to branch from an earlier position.

Metadata includes names, event, site, starting position, and optional time control. A played move updates live state, adds the mover's increment when timed, and appends a snapshot. The game layer checks outcomes separately. Interactive loops charge elapsed time when user input or engine thinking returns, then update the display.

Game history is distinct from a frontend's working position. CLI `position`/`move` manipulate the latter. `play` creates a fresh standard game; `analyze` creates one from the working position; replay reconstructs one from PGN. Returning a game updates the CLI's last-game slot without replacing its working position.

## CLI data flow

```text
line -> quoted tokenization -> typed command -> executor
  -> working position / interactive game / engine / diagnostics / storage
  -> terminal output, optional last game or report
  -> persist configuration after successful command
```

The parser validates argument shape, permitted modes, and numeric ranges. The executor resolves semantic inputs such as legal moves or saved files and calls the relevant subsystem. Interactive game views read their own input until they return; top-level commands are not available inside them.

The CLI session retains a working position, an optional last game, and an optional last report. This makes `pgn save` and `report save` explicit operations and prevents a diagnostic run from saving files automatically.

## UCI data flow

```text
exact "uci" line -> identify engine and select UCI mode
position command -> independent UCI position
go command -> copy position -> synchronous search -> bestmove
```

The frontend uses its own position and always resolves coordinate moves. Searches do not play their selected move into the session. It has no game-history adjudication, background worker, options, or streaming `info` output. `ucinewgame` is presently a no-op. See the [UCI contract and FEN defect](uci.md).

## Search instrumentation and diagnostics

Search templates accept an observer with node, leaf, and move-loop hooks. Ordinary search uses a no-op observer. Benchmarks optionally count nodes/leaves; pruning profiles record how many legal moves a node had and how many children were actually searched before cutoff. This keeps diagnostic logic outside normal evaluation and search policy.

Diagnostics return report contents: text lines and, when available, CSV lines. The interface prints them and retains the latest report. Storage writes it only on request. Perft exercises move generation and reversible mutations without evaluation or game-result adjudication. The debugger adds an external Stockfish process as a reference.

Picker Lab is built as `picker_bench`, separately from `chess_game`. It measures candidate move-selection costs and joins those measurements with observed search workloads during plotting. It does not alter the engine's move order.

## Persistence, startup, and errors

All managed paths are relative to the process working directory. Startup creates `data/pgn`, `data/fen`, and `data/reports`, creates default configuration if missing, then loads it before accepting CLI or UCI input. Resource copying is a CMake post-build step; runtime does not locate resources relative to the executable automatically.

The error hierarchy separates user errors from operational failures. The main loop reports either and continues reading input. Unexpected standard exceptions terminate with status 1; startup directory/configuration failures terminate with status 2. Normal `quit`/EOF returns 0. Fatal paths pause about five seconds before exiting. Reports can contain failed perft cases while the process still exits normally; consumers must inspect report results rather than relying solely on exit status.
