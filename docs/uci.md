# UCI interface — MultiPLY v1.0

[README](../README.md) · [Engine](engine.md)

MultiPLY exposes a limited UCI command set through standard input/output. It starts in CLI mode; send the exact line `uci` to switch. There is no command-line mode flag and no switch back to CLI. CLI and UCI positions are separate; the UCI position initially contains the standard starting position.

## Handshake and lifecycle

| Input | Output / behavior |
| --- | --- |
| `uci` | `id name MultiPLY v1.0`, `id author Elia Damiani`, then `uciok`. Repeating it repeats identification. |
| `isready` | `readyok`, once the command can be processed. |
| `ucinewgame` | Accepted with no response; currently a no-op. Send `position` to establish the next game position. |
| `quit` | Exit when read. EOF also closes the process. |

`uci` and `quit` are intercepted as exact whole lines. Other commands are whitespace-tokenized and case-sensitive. UCI numeric values contain at most nine decimal digits and must be nonnegative.

## Position commands

```text
position startpos
position startpos moves e2e4 e7e5 g1f3
position fen <board> <turn> <castling> <ep> <halfmove> <fullmove> [moves <move> ...]
```

`startpos` and its optional move list work. Every move uses UCI coordinates, with a lowercase promotion suffix when needed. Castling uses king origin/target coordinates such as `e1g1`. At least one move is required after `moves`. Moves are resolved against the evolving legal position.

A position command is not transactional across its move list: a failure after some legal moves leaves the successfully applied prefix in the session. Resend a complete valid `position` command to recover.

## Search commands

### Fixed depth

```text
go depth 4
```

Depth is 1–16 plies inclusive. This performs one search at the requested depth; it has no time limit or external interruption mechanism.

### Fixed move time

```text
go movetime 1000
```

Time is in milliseconds. Timed iterative deepening returns the last fully completed iteration, or a legal fallback if no iteration completes. Time checks are periodic, so this is an approximate budget. Zero is accepted and normally returns the fallback quickly.

### Game clocks

```text
go wtime 300000 btime 300000
go wtime 300000 btime 300000 winc 2000 binc 2000
go wtime 60000 btime 45000 winc 0 binc 0 movestogo 20
```

Both `wtime` and `btime` are required. Increments are optional, but if either is present, **both** `winc` and `binc` must be supplied. `movestogo` is optional; zero is accepted and treated as a horizon of one move. Clock fields can appear in any order, once each. Clock and increment units are milliseconds. The engine uses the clock/increment for the current side to move. See [time allocation](engine.md#time-management) for its budget policy.

These three search forms are separate. Do not combine depth, movetime, and clock fields in one `go`. Bare `go` is unsupported.

## Outputs

A completed search produces exactly one protocol result:

```text
bestmove b1c3
```

The move is in UCI notation. When there are no legal moves, the result is `bestmove 0000`. Search does not advance the stored position; the GUI must send the updated position before another turn.

MultiPLY does not emit `info depth`, `info score`, `info nodes`, `info time`, a principal variation, `ponder`, or option declarations. A sample session follows; the shown best move is illustrative:

```text
> uci
< id name MultiPLY v1.0
< id author Elia Damiani
< uciok
> isready
< readyok
> position startpos moves e2e4 e7e5
> go depth 2
< bestmove b1c3
> quit
```

The `>`/`<` markers indicate direction and are not part of the protocol. Protocol output goes to stdout. The initial CLI prompt, errors, and initialization diagnostics go to stderr, sometimes with ANSI color codes. GUIs should keep the streams separate.

## Compatibility limits

- Search runs on the same thread that reads commands. `isready` and `quit` sent during search are handled only after search returns.
- `stop`, `go infinite`, `ponderhit`, `go ponder`, `searchmoves`, node/mate limits, `setoption`, `register`, and `debug` are unsupported and produce an error when processed.
- There are no Hash, Threads, MultiPV, or strength options. Search is single-threaded and has no transposition table.
- Unknown commands are reported as errors rather than silently ignored.
- Search sees the position, not a game history. It does not adjudicate repetition or fifty-move draws; a GUI must manage game outcomes.

For GUI play, register the executable as a UCI engine with no arguments and use ordinary timed games or bounded searches based on the starting position. Verify the handshake and a short game in the particular GUI. Arbitrary-FEN setup, continuous analysis, early stop, and option configuration require future interface work.
