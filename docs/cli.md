# CLI reference — MultiPLY v1.0

[README](../README.md) · [UCI](uci.md) · [File formats](file-formats.md)

Start `chess_game.exe` without arguments. This reference describes the parser and executor, including behavior that the built-in `help` text may not explain.

## Syntax and session state

- Commands, flags, and enumerated values are case-sensitive. Enter one command per line.
- In the tables, `<value>` denotes a required argument; brackets denote optional syntax. Do not type angle brackets or optional-syntax brackets.
- Double quotes group spaces into one argument: `config set player-1 "Elia Damiani"`. Quotes are stripped; there is no escape syntax for embedding a literal double quote. Single quotes are not grouping characters in the program's parser.
- CLI integer arguments contain at most six decimal digits, with no sign. Every CLI depth argument must be **1–16**, inclusive. `--time-ms` accepts **0–999999 milliseconds**.
- A time control is `<initial-seconds>+<increment-seconds>`; both fields are nonnegative integers of at most six digits. `300+2` is five minutes with a two-second increment. Even zero increment needs `+0`.
- Managed names contain letters, digits, `_`, or `-`; omit extensions. Paths with spaces need double quotes. Relative paths use the process working directory.

The session holds three independent items: a working position, the last game returned by play/analysis, and the last diagnostic report. The working position starts at standard chess's initial position. A game or analysis session does not replace it. Saving a FEN uses the working position; saving a PGN uses the last game; saving a report uses the last report.

## General commands

| Command | Behavior |
| --- | --- |
| `help` | Print built-in help. |
| `quit` | Exit from the top-level prompt. EOF also exits. |
| `uci` | Permanently switch this process into UCI mode. Restart to return to CLI. |

At the top level, `quit` and `uci` must be exact whole lines, without surrounding whitespace. These are handled before the CLI parser. Inside an interactive game, replay, or analysis view, that view handles input instead.

## Working position

| Command | Behavior |
| --- | --- |
| `position show` | Show the board in the configured orientation and print its FEN. |
| `position --startpos` | Reset the working position. |
| `position --fen "<six-field-FEN>"` | Validate and load a FEN; quote the whole FEN. |
| `position --saved-fen <name>` | Load the one-line managed FEN named `<name>`. |
| `move <move>` | Resolve a legal move in the configured notation and apply it to the working position. |

Examples:

```text
position --startpos
move e2e4
config set move-notation san
move e5
position show
position --fen "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1"
```

UCI notation uses origin and target squares (`e2e4`), with a lowercase promotion suffix (`e7e8q`). SAN uses forms such as `e4`, `Nf3`, `exd5`, `O-O`, and `e8=Q`. Move resolution checks legality in the current position. Changing notation affects CLI input and relevant move output; PGN always uses SAN, and the UCI frontend always uses coordinates.

## Play, replay, and analysis

| Command | Behavior |
| --- | --- |
| `play local --untimed` | Start a standard-position game with both sides entered locally. |
| `play local --time-control <seconds+increment>` | Start a local game with clocks. |
| `play engine --player-color <white\|black> --untimed` | Play against MultiPLY; engine turns use a three-second search budget. |
| `play engine --player-color <white\|black> --time-control <seconds+increment>` | Play against MultiPLY with clock-based time management. |
| `replay <name>` | Reconstruct a managed PGN and open its history viewer. |
| `analyze` | Start an untimed analysis game from the working position. |

`play` always starts from the standard position, regardless of the working position. Local games use player 1 as White and player 2 as Black. Engine games use player 1 for the human's name and `Engine` for the opponent. Metadata is copied from the configuration when the game starts.

### Commands inside interactive views

| Input | Play | Analysis | Replay |
| --- | --- | --- | --- |
| `flip` | Reverse board orientation | Same | Same |
| `next`, `previous` | Move the viewing cursor one snapshot | Same | Same |
| `first`, `last` | Jump to initial/latest snapshot | Same | Same |
| A legal move | Play only at the latest snapshot | Branch from the viewed snapshot | Unavailable |
| `resign` | Side to move resigns | Finish with resignation | Unavailable |
| `draw` | Finish by agreement | Finish by agreement | Unavailable |
| `fen` | Unavailable | Show the **live/latest** position's FEN | Unavailable |
| `bestmove` | Unavailable | Search the **live/latest** position for three seconds | Unavailable |
| `eval` | Unavailable | Search the **live/latest** position for three seconds | Unavailable |
| `analyze` | Unavailable | Unavailable | Open temporary analysis from the viewed snapshot |
| `exit` | Unavailable | Return to CLI, recording `*` if unfinished | Return to CLI |

In analysis, entering a move after stepping backward discards later snapshots and continues from the selected point. The `fen`, `bestmove`, and `eval` shortcuts still inspect the live/latest state before branching. Analysis `eval` prints raw evaluation units; top-level `engine dynamic-eval` prints normalized values.

Play returns to the CLI when a result is reached. It has no `exit`/`quit` command; end the game with a result or close input. EOF aborts play/analysis without replacing the session's last game. Clock consumption is charged when input returns or engine search completes; the screen does not continuously tick. Time spent on invalid inputs or history navigation during play also counts.

Completing top-level play or analysis makes its game available to `pgn save`. Replay and the temporary analysis opened inside replay do not replace that saved-game candidate. Board flips persist in the configuration when the top-level command returns.

## Engine commands

These inspect a copy of the working position and do not play the returned move.

| Command | Behavior |
| --- | --- |
| `engine static-eval` | Print the static score without search. |
| `engine dynamic-eval --depth <N>` | Print the score from a single fixed-depth search. |
| `engine dynamic-eval --time-ms <milliseconds>` | Print the score from timed iterative deepening. |
| `engine bestmove --depth <N>` | Print the selected move at fixed depth, or `No moves available.` |
| `engine bestmove --time-ms <milliseconds>` | Print the selected move within an approximate time budget. |
| `engine rank --depth <N>` | Search every legal root move with an independent full window and print a sorted ranking. |

Scores favor White when positive and Black when negative. Ranking lists the side to move's preferred moves first. Mate scores are numeric rather than formatted as `mate N`. See [Engine](engine.md) for units, horizons, and search limits.

## Diagnostics

Each command below prints a report and replaces the last report in memory. Saving remains explicit.

| Command | Behavior |
| --- | --- |
| `perft test --depth <N>` | Print per-root-move leaf counts and total for the working position. |
| `perft test --preset <fast\|moderate\|extended>` | Compare suite counts against stored expectations. |
| `perft benchmark --depth <N>` | Time the working position's perft traversal. |
| `perft benchmark --preset <fast\|moderate\|extended>` | Time a suite and print aggregate throughput. |
| `debug --depth <N>` | Compare against `resources/stockfish.exe`, descending into a mismatching branch. |
| `engine benchmark --depth <N>` | Benchmark fixed-depth search on the working position. |
| `engine benchmark --preset <fast\|moderate\|extended>` | Benchmark fixed-depth searches from a suite. |
| `engine profile pruning --depth <N>` | Collect move-list length versus moves searched on the working position. |
| `engine profile pruning --preset <fast\|moderate\|extended>` | Collect the same histogram across a suite. |

Presets supply their own positions and depths. They do not use the working position. Benchmark warmups can take time before the requested measurement begins. See [Diagnostics](diagnostics.md) for interpretation and Picker Lab.

## Managed files

| Command | Behavior |
| --- | --- |
| `pgn save <name>` | Save the last completed/returned game as `data/pgn/<name>.pgn`. |
| `pgn show <name>` | Print a managed PGN. |
| `pgn list` | List managed PGN names in sorted order. |
| `pgn delete <name>` | Delete a managed PGN. |
| `pgn import "<path>"` | Validate/reconstruct an external PGN, then copy its text under the input filename stem. |
| `pgn export <name> "<directory>"` | Copy a managed PGN into an existing directory. |
| `fen save <name>` | Save the working position as `data/fen/<name>.fen`. |
| `fen show <name>` | Print a managed FEN. |
| `fen list` | List managed FEN names in sorted order. |
| `fen delete <name>` | Delete a managed FEN. |
| `fen import "<path>"` | Validate a one-line external FEN and copy it under the input filename stem. |
| `fen export <name> "<directory>"` | Copy a managed FEN into an existing directory. |
| `report save <name>` | Save the last report as `.txt`, plus `.csv` if it has structured data. |
| `report show <name>` | Print a managed report's text. |
| `report list` | List managed `.txt` report names in sorted order. |
| `report delete <name>` | Delete its `.txt` and attempt to delete its matching `.csv`. |

Save/import/export refuse to overwrite existing files. Import does not activate a position or set the last game: use `position --saved-fen` or `replay` afterward. Export takes a directory, not a full output filename, and does not create missing directories. There is no report import/export command; copy report files with your file manager.

## Configuration

`config show` prints current settings. `config set <field> <value>` supports exactly these fields:

| Field | Values | Default | Purpose |
| --- | --- | --- | --- |
| `player-1` | Quoted text if it contains spaces | `White` | White in local play; human in engine play. |
| `player-2` | Text | `Black` | Black in local play. |
| `event` | Text | `?` | Game/PGN event. |
| `site` | Text | `Elia.chess` | Game/PGN site. |
| `export-clocks` | `true`, `false` | `true` | Include available PGN clock annotations. |
| `move-notation` | `uci`, `san` | `uci` | CLI move input and relevant output notation. |
| `board-orientation` | `white`, `black` | `white` | Side displayed at the bottom. |
| `track-benchmark-stats` | `true`, `false` | `true` | Enable node/leaf instrumentation for engine benchmarks. |

Configuration is rewritten to `data/config.cfg` after every successful top-level CLI command. Setting fields is not available inside interactive views. File keys differ from CLI field names; see [File formats](file-formats.md).

## Example: save and replay an analysis

```text
position --startpos
config set move-notation san
analyze
e4
e5
Nf3
exit
pgn save opening-study
replay opening-study
next
exit
```

The stored PGN ends with `*` because analysis was exited without a result. The CLI working position remains the starting position.
