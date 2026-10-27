# Diagnostics and Picker Lab — MultiPLY v1.0

[README](../README.md) · [CLI syntax](cli.md) · [File formats](file-formats.md)

Diagnostics use the same position and legal move implementation as play and search. They answer different questions: perft checks legal-tree counts, the debugger localizes disagreements, benchmarks measure traversal/search, and profiles describe how much of each move list search consumes.

## Perft correctness

Perft counts leaf positions after exactly N plies. It includes all legal moves and does not stop for repetition, the fifty-move rule, or insufficient material. A position without moves contributes no leaves at a remaining positive depth; depth zero contributes one.

```text
position --startpos
perft test --depth 3
```

The single-position test prints a divide: one subtree count per root move, sorted by the rendered move string, then the total. Start-position reference counts are 20, 400, 8,902, 197,281, and 4,865,609 at depths 1–5. Single-position output is a count, not an automatic comparison against a database.

```text
perft test --preset fast
report save perft-fast
```

Preset tests read `resources/suites/fast-suite.epd`, `moderate-suite.epd`, or `extended-suite.epd`. They print `[PASS]` or `[FAIL]` for each selected position/depth pair. Failed cases include the expected count and FEN. One EPD position can supply multiple searches. Only positive-depth records with expected counts from **5,000 through 20,000,000**, inclusive, are selected; the preset name selects a file, not a depth multiplier. See the [suite format](file-formats.md#epd-suites).

Check the output for failures: a failed comparison does not set a failing process exit code. A larger suite is broader validation, not a proof of every rule or every possible position.

## Stockfish comparison debugger

Place an independently supplied compatible Windows Stockfish binary at `resources/stockfish.exe` relative to the working directory. It must support `go perft N` and the diagnostic `d` command. The binary is optional for everything except this debugger.

```text
position --startpos
debug --depth 3
report save debug-start
```

The debugger compares UCI-coordinate root divides against Stockfish, identifies extra/missing moves, and descends through a mismatching branch. It also compares resulting FEN and checks that apply/revert restores state. Output identifies a move path and the first discovered disagreement, or reports no bugs found on that position. It does not enumerate all independent defects in one run.

The Windows bridge launches a hidden process with redirected pipes, performs the UCI readiness handshake, and requests quit afterward. It reads until expected output markers; there is no general response timeout. A binary with incompatible output can leave the operation waiting.

## Performance benchmarks

```text
perft benchmark --depth 4
perft benchmark --preset fast
engine benchmark --depth 4
engine benchmark --preset fast
```

Perft benchmarks warm up with three start-position depth-5 traversals, then report nodes, elapsed time, and leaf nodes/second for the measured workload. Engine benchmarks warm up with three start-position depth-6 searches. Warmups are outside reported measurement time; total wall time is longer.

Engine benchmark measurements use a single fixed-depth alpha-beta search, not timed iterative deepening. Text includes chosen move, raw evaluation (currently labeled `cp`), and elapsed time. `config set track-benchmark-stats true` selects node/leaf observers; `false` uses the no-op observer and omits those counters.

With tracking enabled, suite reports distinguish:

| Metric | Meaning |
| --- | --- |
| Nodes | Search calls visited, including roots and leaves. |
| Leaves | Depth-zero evaluations and terminal nodes visited. |
| Raw NPS | Actual visited search nodes divided by measured time. |
| Pruning ratio | `100 * (1 - visited_leaves / suite_expected_perft_leaves)`. |
| Effective full-tree rate | Expected full-tree perft leaves divided by measured search time. |

The last two use expected perft counts as a comparison baseline, not actual search-node counts. Terminal root cases are skipped in engine suite timing/counters, while preset summary counts still describe the input suite. A single terminal position reports `No legal moves.`

Use a Release build, a consistent machine and power state, the same suite, and the same stats setting for comparisons. Stats instrumentation affects cost. These are performance measurements, not chess-strength tests. There is no CTest test target defined in the current CMake file.

## Pruning profiles

```text
engine profile pruning --depth 4
report save pruning-start
engine profile pruning --preset fast
report save pruning-fast
```

The profiler records `(N, k)` for completed non-leaf move loops: `N` is legal moves available and `k` is children searched before returning/cutting off. Root loops are included and consume all their moves; depth-zero leaves and no-legal-move nodes have no move-loop observation. Fixed-depth profiling has no timeout.

Text reports the number of observed loops, average legal moves, and average moves searched. Saving writes a companion CSV with header `N,k,count` when observations exist. Preset mode accumulates the histogram across selected searches. `track-benchmark-stats` does not disable pruning profiling.

The histogram describes the present engine's generator order. Adding move ordering would change `k` and therefore change the workload used to evaluate picker strategies.

## Picker Lab

Picker Lab investigates the cost of preparing an ordered move list when a search may consume only part of it. Its synthetic scored moves are separate from actual search moves. Two strategies are compared:

- **Presort:** sort the list up front, then retrieve moves.
- **LazySelect:** find/remove the next best move on demand.

`picker_bench` takes no arguments and writes **`picker_bench.csv` in its working directory**, replacing any existing file with that name. It measures list lengths 1–100, including zero consumption and full consumption, and reports median per-invocation costs in nanoseconds. It attempts to raise process/thread priorities and pin the benchmark thread to CPU index 2. These Windows calls are part of the harness; success is not checked. Expect a substantial benchmark run.

From the repository root after building:

```powershell
.\build\max\Release\picker_bench.exe
```

### Create a fresh plot report

First produce a pruning profile and save it through the CLI:

```text
engine profile pruning --preset fast
report save picker-workload
quit
```

After running `picker_bench`, use PowerShell from the same repository root:

```powershell
New-Item -ItemType Directory -Force .\src\picker_lab\picker_report | Out-Null
Copy-Item .\picker_bench.csv .\src\picker_lab\picker_report\cost.csv
Copy-Item .\data\reports\picker-workload.csv .\src\picker_lab\picker_report\freq.csv
python -m pip install numpy matplotlib
python .\src\picker_lab\plot_picker.py
```

These copies and plot generation replace prior report files at the destination. Keep a separate directory if retaining an earlier experiment. Python 3.10+ is required by the script's syntax, along with NumPy and Matplotlib. Python is not required to build or run the chess engine.

The script takes no arguments. Its actual input location is `src/picker_lab/picker_report/`, resolved relative to the script, regardless of the shell's working directory. Its introductory comment about files being beside the script is outdated. It does not require the historical `picker_analysis.cpp` mentioned in that comment; analysis is implemented in the Python file.

The plotter combines cost and frequency by `(N,k)` and writes:

| Artifact | Content |
| --- | --- |
| `01_summary.png` | Workload and aggregate strategy comparison. |
| `02_fixed_n.pdf` | One page per list size, including sizes absent from the workload. |
| `03_frequency_heatmap.png` | Observed move-loop frequency. |
| `04_winner_heatmap.png` | Faster strategy by list size and consumption. |
| `05_crossover_frontier.png` | Cost crossover across supported sizes. |
| `06_expected_cost_by_n.png` | Frequency-weighted expected cost by list size. |
| `07_workload_impact.png` | Contributions to aggregate workload cost. |

The plotter supports N=1–100, while engine profile CSV can contain larger legal lists. Such rows must be handled explicitly before plotting; the script rejects values outside its supported domain. It requires positive workload observations and sufficient cost coverage. Frontier/expected-cost views use a 250-observation-per-N cutoff. Projected picker savings concern picker overhead only; they do not establish end-to-end engine speedups. No measured picker policy is installed into v1 search.

## Reports and a small validation routine

The most recent diagnostic result remains in memory until another replaces it. `report save <name>` creates `.txt` and optional `.csv`; an existing destination is refused. `report show`, `report list`, and `report delete` manage saved results. Reports do not automatically record compiler, CPU, commit, or environment, so record those separately for reproducibility.

A short check after a build is:

```text
position --startpos
perft test --depth 3
perft test --preset fast
engine bestmove --depth 3
engine profile pruning --depth 3
report save smoke-profile
```

Confirm 8,902 nodes, inspect every preset comparison, check that the returned move is legal, and verify both saved profile files. The README contains a separate UCI handshake example. Use the debugger when investigating a move-generation discrepancy.
