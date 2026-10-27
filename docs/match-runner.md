# Opening books and engine matches

[README](../README.md) · [File formats](file-formats.md)

The build adds two standalone Windows tools: `book_compiler` converts opening PGNs into UCI move lists, and `match_runner` plays two UCI engines against each other. Run both from the repository root so that `resources/openings/` and `data/matches/` resolve correctly.

## Compile opening books

Place the following source PGNs in `resources/openings/`. The filenames and output names are currently fixed in `src/match_runner/book_compiler.cpp`.

| Input PGN | Compiled output |
| --- | --- |
| `UHO_XXL_+0.80_+1.09.pgn` | `very-low_var.txt` |
| `UHO_XXL_+0.90_+1.19.pgn` | `low_var.txt` |
| `UHO_XXL_2022_+100_+129.pgn` | `medium_var.txt` |
| `UHO_XXL_2022_+110_+139.pgn` | `high_var.txt` |
| `UHO_XXL_2022_+120_+149.pgn` | `very-high_var.txt` |

After building with the `max` preset:

```powershell
.\build\max\Release\book_compiler.exe
```

The compiler takes no arguments and replaces the compiled files in `resources/openings/`. Each accepted entry needs an `ECO` tag and legal SAN moves. Entries without ECO or with illegal moves are skipped; malformed PGN can stop compilation. Duplicate final positions are removed within each book, ignoring halfmove/fullmove counters through `Position::to_fen(true)`.

Each output starts with `name=`, `eval_var=`, and `opening_count=` lines, a blank line, and an `ID        ECO   UCI-MOVES` header. Data rows contain an opening ID, ECO code, and space-separated UCI moves; the three fields are separated by three spaces.

Use nonempty opening lines starting from the standard position for matches. The compiler expects a blank line between tags and movetext and a blank line after each game, including the last game.

## Run a match

Launch the runner after preparing a compiled book:

```powershell
.\build\max\Release\match_runner.exe
```

At `match-runner>`, configure the two engine executables and run:

```text
set engine1 C:/engines/MultiPLY.exe
set engine2 C:/engines/reference.exe
set time-control 2000+0
set game-number 100
set opening-eval-variation medium
set seed 1
show-config
run
quit
```

Time controls use **milliseconds**: `2000+0` gives each engine two seconds with no increment. Engine paths must contain no whitespace; the command parser does not support quoted paths. `help` lists the commands, and `quit` exits at the prompt.

Defaults are `2000+0`, 100 games, the `medium` book, and seed 1; both engine paths must be set. Game count must be even and between 2 and 1,000,000. Available opening variations are `very-low`, `low`, `medium`, `high`, and `very-high`.

The runner samples half as many openings as games, then plays each opening twice with engine colors swapped. Keep at least that many entries in the book to obtain the requested game count. A fixed seed controls opening selection. Engines receive the full move history and UCI clock commands; the game layer handles legal moves, clocks, and game endings.

Each match creates a directory under `data/matches/`, named from the engine names and timestamp. It contains one PGN per game (`00001_opening-<id>.pgn`, etc.) and `summary.txt` with engine 1's score and win/draw/loss counts overall and by color. PGNs include clocks and optional `Opening_ID` and `ECO` tags.

The runner waits synchronously for UCI responses. An unresponsive engine can stall the match; there is no response timeout or interactive cancellation command during `run`.
