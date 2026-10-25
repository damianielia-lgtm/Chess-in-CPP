# Files, configuration, and formats — MultiPLY v1.0

[README](../README.md) · [CLI](cli.md) · [Diagnostics](diagnostics.md)

## Runtime directories

All engine paths below are relative to the **process working directory**, not automatically to the executable or repository.

```text
data/
    config.cfg
    pgn/<name>.pgn
    fen/<name>.fen
    reports/<name>.txt
    reports/<name>.csv
resources/
    fast-suite.epd
    moderate-suite.epd
    extended-suite.epd
    stockfish.exe          # optional, independently supplied
```

Startup creates the three data subdirectories and a default config if absent. It does this even when the process will be used through UCI. It does not recreate missing suites or download Stockfish. CMake copies the resource directory beside `chess_game.exe` after a build; running from that directory makes those relative resource paths available.

Different working directories produce separate data/configuration sets. Choose a writable directory for GUI launches. Startup cannot continue with an unreadable/invalid configuration or failed directory initialization.

## Managed names and I/O behavior

Managed commands accept a logical name without a suffix: `fen save opening-1` creates `data/fen/opening-1.fen`. Names must be nonempty and contain letters/digits, underscores, or dashes; spaces, dots, and path separators are rejected. Use ASCII names for predictable behavior across locales.

PGN/FEN import uses the source filename stem as the managed name, so `My game.pgn` fails the name rules even when its full path is quoted correctly. Import validates before saving. Export appends the managed filename to an existing destination directory.

Managed saves, imports, and exports refuse existing destinations. They write lines with trailing newlines; copying through these commands preserves line content rather than guaranteeing byte-identical files. Writes are not transactional or backed up. Report saving checks both intended destinations before writing, but a later I/O failure can still leave one file. Configuration is the exception to no-overwrite behavior: it is rewritten after successful CLI commands.

Listing returns sorted stems matching the relevant extension. Reports are listed by `.txt`; a CSV alone will not appear. Report deletion first deletes the text file, then attempts the companion CSV. A missing CSV is ignored, but filesystem I/O failures can still be reported.

## Configuration file

Default `data/config.cfg`:

```ini
player_1=White
player_2=Black
event=?
site=Elia.chess
export_clocks=true
move_notation=uci
board_orientation=white
benchmark_track_stats=true
```

Each nonempty line is `key=value`, split at the first `=`. There is no whitespace trimming, quoting, section syntax, or comment syntax. Empty lines are skipped; whitespace-only lines are invalid. String values are stored literally. Unknown keys, duplicate keys, and invalid enumerated values fail loading. Missing known keys retain defaults.

| File key | CLI field | Accepted values |
| --- | --- | --- |
| `player_1` | `player-1` | Text |
| `player_2` | `player-2` | Text |
| `event` | `event` | Text |
| `site` | `site` | Text |
| `export_clocks` | `export-clocks` | `true`, `false` |
| `move_notation` | `move-notation` | `uci`, `san` |
| `board_orientation` | `board-orientation` | `white`, `black` |
| `benchmark_track_stats` | `track-benchmark-stats` | `true`, `false` |

Prefer `config set` while running. To edit by hand, close the process first; a subsequent successful CLI command rewrites the file from memory. If it is invalid, correct it or move it aside while the program is closed; the next launch regenerates defaults when the file is absent.

## FEN positions

A managed `.fen` file has exactly one line containing all six FEN fields:

```text
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

Fields are piece placement, side to move, castling rights, en passant target, halfmove clock, and fullmove number. Use exactly one ASCII space between fields: the parser expects five spaces in the FEN string. It checks board shape, king counts, pawn ranks, side to move, castling consistency, en passant data, counters, and king-state constraints. Validation is not a proof that every accepted position is reachable from a legal game.

The internal position constructor also accepts `startpos` as a shorthand. Saving a FEN always serializes the full six-field form. An extra blank line in a managed/imported FEN fails the one-line requirement. Loading a FEN sets a position, not move history or clock data. UCI's separate `position fen` path has a known field-joining bug; see [UCI](uci.md).

## PGN games

PGN import/replay targets **one game**, not a multi-game database. The parser requires `Event`, `Site`, `White`, `Black`, and `Result` tags, plus movetext ending in the matching result (`1-0`, `0-1`, `1/2-1/2`, or `*`). All tag values must be nonempty, including ignored tags. Unknown/unneeded tags are ignored for reconstruction.

Minimal untimed example:

```pgn
[Event "Study"]
[Site "Local"]
[White "White"]
[Black "Black"]
[Result "*"]

1. e4 e5 2. Nf3 *
```

Tag values support escaped `\"` and `\\` on import. Export currently writes metadata directly into quoted tags without escaping; avoid quotes, backslashes, and newlines in names/event/site when planning to save PGN.

Movetext uses SAN and valid move numbering. The parser strips trailing `!`/`?`, skips `$` numeric-annotation tokens, normalizes zeroes in castling notation to `O`, skips semicolon comments, and ignores parenthesized variations, including nested ones. Brace comments are inspected for clock annotations; other comment text is ignored during reconstruction. Unmatched comments or variations are errors. This is not a lossless annotation/variation editor.

`SetUp "1"` requires a matching `FEN` tag. A FEN without the setup indication, or a setup indication without FEN, is rejected. `SetUp "0"` corresponds to no FEN tag. Starting move numbers and side to move come from the starting position.

`TimeControl` accepts `-`/`?` for untimed/unknown, an integer number of initial seconds, or `initial+increment` in seconds. Multi-stage controls are unsupported. Clock comments use forms such as `{[%clk 0:04:59.5]}`: hours, two-digit minutes/seconds, and an optional 1–3 digit fraction. Place each clock after its move. Replay retains clocks only if a timed control exists and **every** ply has a clock value; otherwise reconstruction is untimed.

Import validates by reconstructing the legal move sequence, then stores the original input lines. Replay reconstructs positions and snapshots, including clocks when possible, and records the PGN's declared outcome. Saving a returned game generates new PGN with Event, Site, Date, Round, White, Black, Result, and TimeControl tags, optional SetUp/FEN, and optional clocks. Date is the save date. General comments, variations, and extra original tags are not reconstructed into newly generated PGN.

## EPD suites

The resources use an EPD-like perft format with a **six-field FEN** before the first semicolon:

```text
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1; id "Initial position"; D1 20; D2 400; D3 8902; D4 197281;
```

This differs from conventional four-field EPD. Each nonempty line is a position followed by semicolon-separated fields. An optional `id "..."` is recognized when it is the second nonempty field; otherwise an automatic position name is used. `D<depth> <expected-leaf-count>` supplies a perft reference. Other fields are ignored unless they begin with `D`, in which case they are parsed as a depth record.

Only records with positive depth and expected counts between 5,000 and 20,000,000 inclusive are selected. Thus the example selects D3/D4 but not D1/D2. Multiple selected depths create multiple searches. A position counts in the summary only if at least one record is selected. Blank lines are skipped; comment lines and whitespace-only lines are not a supported comment mechanism.

Keep custom suite depths within **1–16**, valid for the engine's move-list stack. The EPD loader permits depth text beyond the normal CLI range and does not enforce the stack limit. Ensure a suite contains at least one selected record; empty selections are unsuitable for progress displays and aggregate rate calculations. There is no CLI switch for an arbitrary suite filename: edit the appropriate `resources/*-suite.epd` file or load a single FEN and use a depth command. Rebuild to refresh the copy beside the executable.

## Text reports and CSV data

`report save <name>` stores human-readable output at `data/reports/<name>.txt`. Only reports carrying structured data create a matching CSV; currently pruning profiles do. The last report is an in-memory slot, not a file until saved. Reports have no versioned schema or full environment metadata.

Pruning-frequency CSV:

```csv
N,k,count
20,1,12
20,20,1
```

`N` is legal-list size, `k` is children searched, and `count` is the number of completed move loops with that pair. Only nonzero counts are emitted, with `1 <= k <= N`; leaves are excluded. It is a workload histogram, not a list of best moves or perft results.

Picker microbenchmark CSV:

```csv
N,k,presort,lazyselect
1,0,13.281250,12.500000
```

Costs are median nanoseconds per invocation; N runs 1–100 and k runs 0–N. The separate benchmark writes `picker_bench.csv` in its working directory, overwriting it. For plotting, copy it to `src/picker_lab/picker_report/cost.csv` and copy a saved pruning CSV to `freq.csv` in that same report directory. The plotter joins their rows by N/k, supports only N<=100, and writes its figures there. See [Picker Lab](diagnostics.md#picker-lab) for the full workflow and interpretation.

## Release assets and local artifacts

The build inputs are the CMake files, source tree, and resource suites. Generated `build/`, user `data/`, benchmark output, and plot reports are local artifacts. `versions/` is not read by the program. Stockfish is independently supplied for diagnostics; do not assume a clone contains its executable. The root ignore rules explicitly allow documentation, the CMake files, and resource EPD suites while leaving generated/user files ignored.
