"""Render the picker analysis as six PNG charts and one PDF with a page per N.

Run this file without arguments with cost.csv and freq.csv beside it.
Analysis follows picker_analysis.cpp; charts are written to picker_report.
"""

import csv
from dataclasses import dataclass
from pathlib import Path

try:
    import numpy as np
    import matplotlib

    matplotlib.use('Agg')  # Render files without opening a desktop window.
    import matplotlib.pyplot as plt
    from matplotlib.axes import Axes
    from matplotlib.backends.backend_pdf import PdfPages
    from matplotlib.colorbar import Colorbar
    from matplotlib.colors import (
        LinearSegmentedColormap,
        ListedColormap,
        LogNorm,
        SymLogNorm,
    )
    from matplotlib.figure import Figure
    from matplotlib.image import AxesImage
    from matplotlib.lines import Line2D
    from matplotlib.patches import Patch
    from matplotlib.ticker import FuncFormatter, MaxNLocator, PercentFormatter
except ImportError as error:
    raise SystemExit(
        'Plotting requires NumPy and Matplotlib. Install with the selected '
        'interpreter: python -m pip install numpy matplotlib\n' + str(error)
    )


REPORT_DIRECTORY = Path(__file__).resolve().parent / 'picker_report'

MAX_N = 100
MIN_OBSERVATIONS_PER_N = 250

BACKGROUND_COLOR = '#F8F7F3'
TEXT_COLOR = '#192F3B'
MUTED_COLOR = '#667580'
GRID_COLOR = '#DDE2E2'
PRESORT_COLOR = '#BF5937'
LAZYSELECT_COLOR = '#087F8C'
FREQUENCY_COLOR = '#8697A5'
INVALID_COLOR = '#E5E9EB'

REPORT_STYLE = {
    'font.family': 'DejaVu Sans',
    'font.size': 11,
    'figure.facecolor': BACKGROUND_COLOR,
    'axes.facecolor': BACKGROUND_COLOR,
    'axes.edgecolor': GRID_COLOR,
    'axes.labelcolor': MUTED_COLOR,
    'text.color': TEXT_COLOR,
    'xtick.color': MUTED_COLOR,
    'ytick.color': MUTED_COLOR,
    'axes.spines.top': False,
    'axes.spines.right': False,
    'axes.titleweight': 'bold',
    'axes.labelsize': 11,
    'axes.axisbelow': True,
    'grid.color': GRID_COLOR,
    'grid.linewidth': 0.7,
    'legend.frameon': False,
    'legend.fontsize': 10,
    'lines.linewidth': 2.5,
    'savefig.facecolor': BACKGROUND_COLOR,
}


@dataclass
class AnalysisData:
    """The analyzed CSV values in plotting form; costs are in nanoseconds.

    Grid arrays use [k, N - 1]; cells with k > N are NaN. Per-N arrays
    use [N - 1]; undefined statistics are NaN so plots leave gaps.
    """

    max_n: int
    total_invocations: int

    # One measurement and invocation count for each valid (N, k) pair.
    presort_ns: np.ndarray
    lazyselect_ns: np.ndarray
    invocations: np.ndarray

    # Statistics conditional on a particular list size N.
    invocations_by_n: np.ndarray
    expected_presort_ns: np.ndarray
    expected_lazyselect_ns: np.ndarray
    expected_oracle_ns: np.ndarray
    first_presort_k: np.ndarray
    sustained_presort_k: np.ndarray
    mean_k: np.ndarray
    median_k: np.ndarray
    q25_k: np.ndarray
    q75_k: np.ndarray

    # Statistics over the entire workload.
    observed_n_mean: float
    observed_k_mean: float
    presort_mean_ns: float
    lazyselect_mean_ns: float
    oracle_mean_ns: float
    n_policy_mean_ns: float
    presort_win_share: float
    lazyselect_win_share: float
    tie_share: float

    @property
    def n_values(self) -> np.ndarray:
        """The benchmarked list sizes, 1 through max_n inclusive."""
        return np.arange(1, self.max_n + 1)

    @property
    def visits_percent(self) -> np.ndarray:
        """Each list size's percentage of all recorded invocations."""
        return self.invocations_by_n / self.total_invocations * 100

    @property
    def observed_max_n(self) -> int:
        """Largest list size with at least one workload observation."""
        return int(self.n_values[self.invocations_by_n > 0].max())

    @property
    def supported_n(self) -> np.ndarray:
        """Boolean mask of list sizes meeting the figures 05/06 sample cutoff."""
        return self.invocations_by_n >= MIN_OBSERVATIONS_PER_N

    @property
    def supported_max_n(self) -> int:
        """Last N meeting the sample cutoff, or zero if no list size qualifies."""
        return int(self.n_values[self.supported_n].max(initial=0))

    @property
    def zoom_n(self) -> int:
        """Upper bound of the detail view containing 99% of invocations."""
        cumulative_invocations = np.cumsum(self.invocations_by_n)
        percentile_n = int(
            np.searchsorted(cumulative_invocations, 0.99 * self.total_invocations)
            + 1
        )
        # Preserve the original chart's minimum detail-axis extent of 10.
        return max(10, min(self.max_n, percentile_n))

    @property
    def one_move_share(self) -> float:
        """Fraction of all invocations that stopped after exactly one move."""
        return float(np.nansum(self.invocations[1]) / self.total_invocations)

    @property
    def full_consumption_share(self) -> float:
        """Fraction that consumed every available move (k = N)."""
        full_invocations = sum(
            self.invocations[n, n - 1] for n in self.n_values
        )
        return float(full_invocations / self.total_invocations)


def load_report_data(cost_path: Path, frequency_path: Path) -> AnalysisData:
    max_n = MAX_N
    grid_shape = (max_n + 1, max_n)
    presort_ns = np.full(grid_shape, np.nan)
    lazyselect_ns = np.full(grid_shape, np.nan)
    invocations = np.full(grid_shape, np.nan)
    frequencies = {}

    for path, is_cost in ((cost_path, True), (frequency_path, False)):
        with path.open(encoding='utf-8', newline='') as input_file:
            rows = csv.reader(input_file)
            next(rows, None) # Skip the header

            for row in rows:
                if not row:
                    continue

                try:
                    n, k = int(row[0]), int(row[1])
                except (ValueError, IndexError) as error:
                    raise ValueError(f'Invalid N,k in {path}') from error
                if not (1 <= n <= max_n and 0 <= k <= n):
                    raise ValueError(f'Invalid N,k in {path}')
                
                if is_cost:
                    presort_ns[k, n - 1] = float(row[2])
                    lazyselect_ns[k, n - 1] = float(row[3])
                else:
                    count = int(row[2])
                    frequencies[n, k] = count

    by_n = {
        'invocations_by_n': np.full(max_n, np.nan),
        'expected_presort_ns': np.full(max_n, np.nan),
        'expected_lazyselect_ns': np.full(max_n, np.nan),
        'expected_oracle_ns': np.full(max_n, np.nan),
        'first_presort_k': np.full(max_n, np.nan),
        'sustained_presort_k': np.full(max_n, np.nan),
        'mean_k': np.full(max_n, np.nan),
        'median_k': np.full(max_n, np.nan),
        'q25_k': np.full(max_n, np.nan),
        'q75_k': np.full(max_n, np.nan)
    }
    total_invocations = observed_n_total = observed_k_total = 0
    presort_win_nodes = lazyselect_win_nodes = tie_nodes = 0
    presort_total = lazyselect_total = oracle_total = n_policy_total = 0.0

    for n in range(1, max_n + 1):
        column = n - 1
        count_at_n = k_total = 0
        presort_at_n = lazyselect_at_n = oracle_at_n = 0.0

        for k in range(n + 1):
            presort = presort_ns[k, column]
            lazyselect = lazyselect_ns[k, column]
            count = frequencies.get((n, k), 0)

            if not np.isfinite(presort) or not np.isfinite(lazyselect):
                raise ValueError(f'Missing cost for N={n}, k={k}')
            
            invocations[k, column] = count
            count_at_n += count
            k_total += k * count
            presort_at_n += presort * count
            lazyselect_at_n += lazyselect * count
            oracle_at_n += min(presort, lazyselect) * count

            if presort < lazyselect:
                presort_win_nodes += count
                if np.isnan(by_n['first_presort_k'][column]):
                    by_n['first_presort_k'][column] = k
            elif presort > lazyselect:
                lazyselect_win_nodes += count
            else:
                tie_nodes += count

        for k in range(n, -1, -1):
            if not presort_ns[k, column] < lazyselect_ns[k, column]:
                break
            by_n['sustained_presort_k'][column] = k

        by_n['invocations_by_n'][column] = count_at_n
        if count_at_n:
            by_n['expected_presort_ns'][column] = presort_at_n / count_at_n
            by_n['expected_lazyselect_ns'][column] = lazyselect_at_n / count_at_n
            by_n['expected_oracle_ns'][column] = oracle_at_n / count_at_n
            by_n['mean_k'][column] = k_total / count_at_n
            
            for quarter, name in ((1, 'q25_k'), (2, 'median_k'), (3, 'q75_k')):
                target = (count_at_n * quarter + 3) // 4  # ceil(total * quarter / 4)
                cumulative = 0
                for k in range(n + 1):
                    cumulative += frequencies.get((n, k), 0)
                    if cumulative >= target:
                        by_n[name][column] = k
                        break

        total_invocations += count_at_n
        observed_n_total += n * count_at_n
        observed_k_total += k_total
        presort_total += presort_at_n
        lazyselect_total += lazyselect_at_n
        oracle_total += oracle_at_n
        n_policy_total += min(presort_at_n, lazyselect_at_n)

    if total_invocations == 0:
        raise ValueError('No invocations: weighted means are undefined.')

    return AnalysisData(
        max_n=max_n,
        total_invocations=total_invocations,
        presort_ns=presort_ns,
        lazyselect_ns=lazyselect_ns,
        invocations=invocations,
        **by_n,
        observed_n_mean=observed_n_total / total_invocations,
        observed_k_mean=observed_k_total / total_invocations,
        presort_mean_ns=presort_total / total_invocations,
        lazyselect_mean_ns=lazyselect_total / total_invocations,
        oracle_mean_ns=oracle_total / total_invocations,
        n_policy_mean_ns=n_policy_total / total_invocations,
        presort_win_share=presort_win_nodes / total_invocations,
        lazyselect_win_share=lazyselect_win_nodes / total_invocations,
        tie_share=tie_nodes / total_invocations,
    )


def compact_number(value: float, _position=None) -> str:
    """Format tick labels using k for thousands and M for millions."""
    if abs(value) >= 1e6:
        return f'{value / 1e6:g}M'
    if abs(value) >= 1e3:
        return f'{value / 1e3:g}k'
    return f'{value:g}'


def create_page(number: str, title: str, subtitle: str) -> Figure:
    """Create the shared 14-by-8.5-inch page header and separator line."""
    figure = plt.figure(figsize=(14, 8.5))
    figure.text(
        0.065, 0.957, f'PICKER LAB   /   {number}',
        fontsize=10, weight='bold', color=LAZYSELECT_COLOR,
    )
    figure.text(0.065, 0.897, title, fontsize=25, weight='bold')
    figure.text(0.065, 0.851, subtitle, fontsize=11, color=MUTED_COLOR)
    figure.add_artist(Line2D(
        [0.065, 0.94], [0.828, 0.828], transform=figure.transFigure,
        color=GRID_COLOR, linewidth=0.8,
    ))
    return figure


def add_footer(figure: Figure, note: str) -> None:
    """Add the chart's interpretation note and the shared report signature."""
    figure.text(0.065, 0.029, note, fontsize=9, color=MUTED_COLOR)
    figure.text(
        0.94, 0.029, 'CHESS ENGINE  /  MOVE ORDERING',
        ha='right', fontsize=8, color=MUTED_COLOR,
    )


def save_png(figure: Figure, path: Path) -> None:
    """Save an overview at the original 180 DPI, then release its figure."""
    try:
        figure.savefig(path, dpi=180)
    finally:
        plt.close(figure)


def style_axis(axis: Axes, ylabel: str | None = None) -> None:
    """Apply the shared grid, tick labels, and optional vertical-axis label."""
    axis.grid(axis='y')
    axis.spines['left'].set_visible(False)
    axis.tick_params(axis='both', length=0, pad=7)
    axis.yaxis.set_major_formatter(FuncFormatter(compact_number))
    if ylabel:
        axis.set_ylabel(ylabel, labelpad=12)


def picker_legend_handles() -> list[Line2D]:
    """Return consistent Presort and LazySelect legend samples."""
    return [
        Line2D([], [], color=PRESORT_COLOR, label='Presort'),
        Line2D([], [], color=LAZYSELECT_COLOR, label='LazySelect'),
    ]


def mask_sparse_sizes(values: np.ndarray, data: AnalysisData) -> np.ndarray:
    """Leave gaps at excluded N values without changing the original data."""
    return np.where(data.supported_n, values, np.nan)


def finite_maximum(*series: np.ndarray) -> float:
    """Find a plot's upper bound, using 1 when all supplied values are missing."""
    values = np.concatenate(series)
    return float(values[np.isfinite(values)].max(initial=1.0))


def shade_sparse_sizes(axis: Axes, data: AnalysisData) -> None:
    """Shade every excluded N, including gaps between supported list sizes."""
    for n in data.n_values[~data.supported_n]:
        axis.axvspan(
            n - 0.5, n + 0.5,
            color=INVALID_COLOR, alpha=0.7, linewidth=0, zorder=-1,
        )


def annotate_sparse_tail(axis: Axes, data: AnalysisData) -> None:
    """Explain omitted values in the tail, or show an explicit empty state."""
    if data.supported_max_n == 0:
        axis.text(
            0.5, 0.5, f'No N has at least {MIN_OBSERVATIONS_PER_N:,} observations',
            transform=axis.transAxes, ha='center', color=MUTED_COLOR, fontsize=11,
        )
    elif data.supported_max_n < data.max_n:
        axis.text(
            (data.supported_max_n + data.max_n) / 2, 0.95,
            f'Fewer than {MIN_OBSERVATIONS_PER_N:,} observations\nValues omitted',
            transform=axis.get_xaxis_transform(),
            va='top', ha='center', color=MUTED_COLOR, fontsize=10,
        )


def plot_summary(data: AnalysisData) -> Figure:
    """Chart 01: weighted costs, average move counts, win shares, and savings."""
    presort_mean = data.presort_mean_ns
    lazyselect_mean = data.lazyselect_mean_ns
    if presort_mean < lazyselect_mean:
        title = 'Presort leads on this workload'
    elif lazyselect_mean < presort_mean:
        title = 'LazySelect leads on this workload'
    else:
        title = 'Both pickers have the same weighted cost'

    saving_ns = abs(presort_mean - lazyselect_mean)
    higher_mean = max(presort_mean, lazyselect_mean)
    reduction_percent = saving_ns / higher_mean * 100 if higher_mean else 0
    figure = create_page(
        '01', title,
        f'{data.total_invocations:,.0f} observed picker invocations  /  '
        f'N = 1–{data.max_n} benchmarked  /  '
        f'N = 1–{data.observed_max_n} observed',
    )

    workload_means = [
        (0.065, 'AVERAGE MOVES AVAILABLE, N', data.observed_n_mean),
        (0.51, 'AVERAGE MOVES CONSUMED, k', data.observed_k_mean),
    ]
    for x, label, value in workload_means:
        figure.text(x, 0.777, label, color=MUTED_COLOR, fontsize=10, weight='bold')
        figure.text(x, 0.735, f'{value:,.2f}', color=TEXT_COLOR, fontsize=24, weight='bold')

    comparision_values = [
        (0.065, 'PRESORT', f'{presort_mean:,.2f} ns', PRESORT_COLOR),
        (0.365, 'LAZYSELECT', f'{lazyselect_mean:,.2f} ns', LAZYSELECT_COLOR),
        (0.69, 'LOWER MODELED COST', f'{reduction_percent:.2f}%', TEXT_COLOR),
    ]
    for x, label, value, color in comparision_values:
        figure.text(x, 0.68, label, color=color, fontsize=10, weight='bold')
        figure.text(x, 0.61, value, fontsize=30, weight='bold', color=color)

    cost_axis = figure.add_axes([0.14, 0.38, 0.74, 0.18])
    means = [presort_mean, lazyselect_mean]
    cost_axis.barh(
        [1, 0], means, height=0.45, color=[PRESORT_COLOR, LAZYSELECT_COLOR],
    )
    for y, value in zip([1, 0], means):
        cost_axis.text(
            value + max(means + [1]) * 0.02, y, f'{value:,.2f}',
            va='center', weight='bold',
        )
    cost_axis.set_xlim(0, max(means + [1]) * 1.19)
    cost_axis.set_xlabel(
        'Frequency-weighted cost per picker invocation (ns)', labelpad=12,
    )
    style_axis(cost_axis)
    cost_axis.set_yticks([1, 0], ['Presort', 'LazySelect'])
    cost_axis.grid(False)
    cost_axis.grid(axis='x')

    figure.text(
        0.065, 0.259, 'Which picker wins each visit?', fontsize=12, weight='bold',
    )
    win_axis = figure.add_axes([0.065, 0.214, 0.87, 0.026])
    left_edge = 0
    win_segments = [
        (data.presort_win_share, PRESORT_COLOR),
        (data.tie_share, GRID_COLOR),
        (data.lazyselect_win_share, LAZYSELECT_COLOR),
    ]
    for fraction, color in win_segments:
        win_axis.barh(0, fraction, left=left_edge, height=1, color=color)
        left_edge += fraction
    win_axis.set(xlim=(0, 1), ylim=(-0.5, 0.5))
    win_axis.axis('off')
    figure.text(
        0.065, 0.185, f'Presort  {data.presort_win_share:.1%}',
        color=PRESORT_COLOR, fontsize=11,
    )
    figure.text(
        0.45, 0.185, f'Ties  {data.tie_share:.1%}',
        color=MUTED_COLOR, fontsize=11, ha='center',
    )
    figure.text(
        0.935, 0.185, f'LazySelect  {data.lazyselect_win_share:.1%}',
        color=LAZYSELECT_COLOR, fontsize=11, ha='right',
    )

    figure.text(
        0.065, 0.121,
        f'{saving_ns:.2f} ns saved per invocation  /  '
        f'{saving_ns * data.total_invocations / 1e9:.3f} s across the recorded workload',
        fontsize=13, weight='bold',
    )

    add_footer(
        figure,
        'Weighted by observed counts. Modeled picker time; not measured whole-engine speedup.',
    )

    return figure


def add_crossover_markers(
    cost_axis: Axes, frequency_axis: Axes, data: AnalysisData, n: int,
) -> None:
    """Mark the first strict Presort win and any different sustained win."""
    first_win = data.first_presort_k[n - 1]
    sustained_win = data.sustained_presort_k[n - 1]
    if np.isfinite(first_win):
        for axis in (cost_axis, frequency_axis):
            axis.axvline(
                first_win, color=TEXT_COLOR,
                linestyle=(0, (3, 3)), linewidth=1.25,
            )
        label_on_left = first_win > 0.7 * n
        label_x = first_win - 0.25 if label_on_left else first_win + 0.25
        cost_axis.text(
            label_x, 0.95, f'First Presort win\nk = {first_win:.0f}',
            transform=cost_axis.get_xaxis_transform(), va='top',
            ha='right' if label_on_left else 'left', fontsize=10, color=TEXT_COLOR,
            bbox=dict(facecolor=BACKGROUND_COLOR, edgecolor='none', alpha=0.9, pad=3),
        )

    if np.isfinite(sustained_win) and sustained_win != first_win:
        for axis in (cost_axis, frequency_axis):
            axis.axvline(
                sustained_win, color=PRESORT_COLOR, linestyle=':', linewidth=1.2,
            )


def plot_fixed_n(data: AnalysisData, n: int) -> Figure:
    """One page of chart 02: cost and consumption for a single list size."""
    column = n - 1
    consumed_moves = np.arange(n + 1)
    presort_cost = data.presort_ns[:n + 1, column]
    lazyselect_cost = data.lazyselect_ns[:n + 1, column]
    invocations = data.invocations[:n + 1, column]
    total_at_n = data.invocations_by_n[column]

    figure = create_page(
        '02', f'N = {n}  |  Cost meets consumption',
        f'{total_at_n:,.0f} observations  /  '
        f'{data.visits_percent[column]:.2f}% of all visits  /  '
        'cost curves and workload share the same k-axis',
    )
    cost_axis = figure.add_axes([0.09, 0.415, 0.83, 0.36])
    frequency_axis = figure.add_axes([0.09, 0.15, 0.83, 0.19], sharex=cost_axis)
    cost_axis.plot(
        consumed_moves, presort_cost, color=PRESORT_COLOR,
        marker='o', markersize=3, label='Presort',
    )
    cost_axis.plot(
        consumed_moves, lazyselect_cost, color=LAZYSELECT_COLOR,
        marker='o', markersize=3, label='LazySelect',
    )
    cost_axis.fill_between(
        consumed_moves, presort_cost, lazyselect_cost,
        where=lazyselect_cost > presort_cost,
        interpolate=True, color=PRESORT_COLOR, alpha=0.08,
    )
    cost_axis.fill_between(
        consumed_moves, presort_cost, lazyselect_cost,
        where=presort_cost > lazyselect_cost,
        interpolate=True, color=LAZYSELECT_COLOR, alpha=0.08,
    )
    cost_axis.set_ylim(
        0, max(float(presort_cost.max()), float(lazyselect_cost.max()), 1) * 1.15,
    )
    style_axis(cost_axis, 'Picker cost (ns)')
    cost_axis.tick_params(labelbottom=False)
    figure.legend(
        handles=picker_legend_handles(), loc='center left',
        bbox_to_anchor=(0.083, 0.794), ncol=2,
    )

    # These percentages describe k within this N, not within the whole workload.
    conditional_percent = invocations / total_at_n * 100 if total_at_n else invocations
    frequency_axis.bar(
        consumed_moves, conditional_percent, width=0.76, color=FREQUENCY_COLOR,
    )
    frequency_axis.set_ylim(
        0, min(100, max(float(conditional_percent.max()) * 1.25, 1)),
    )
    frequency_axis.set_xlim(-0.5, n + 0.5)
    frequency_axis.set_xlabel('Moves consumed before stopping, k', labelpad=10)
    frequency_axis.xaxis.set_major_locator(MaxNLocator(nbins=11, integer=True))
    style_axis(frequency_axis, 'Visits at this N (%)')
    frequency_axis.yaxis.set_major_formatter(PercentFormatter(100, decimals=0))
    add_crossover_markers(cost_axis, frequency_axis, data, n)

    if total_at_n:
        figure.text(
            0.92, 0.79,
            f'Presort {data.expected_presort_ns[column]:.2f} ns   /   '
            f'LazySelect {data.expected_lazyselect_ns[column]:.2f} ns   /   '
            f'Oracle {data.expected_oracle_ns[column]:.2f} ns',
            ha='right', fontsize=10, color=MUTED_COLOR,
        )
    else:
        # Conditional percentages are undefined when this N has no observations.
        frequency_axis.set_yticks([])
        frequency_axis.set_ylabel('Observed visits')
        frequency_axis.grid(False)
        frequency_axis.text(
            0.5, 0.6, 'No observations at this N',
            transform=frequency_axis.transAxes, ha='center', color=MUTED_COLOR,
        )
    add_footer(
        figure,
        'Dashed line = first measured k with Presort < LazySelect. Lines connect discrete measurements.',
    )
    return figure


def write_fixed_n_pdf(data: AnalysisData, path: Path) -> None:
    """Write pages for every N, including list sizes absent from the workload."""
    with PdfPages(path) as pdf:
        for n in range(1, data.max_n + 1):
            figure = plot_fixed_n(data, n)
            try:
                pdf.savefig(figure)
            finally:
                plt.close(figure)


def create_heatmap_axis(
    figure: Figure, rectangle: list[float], limit: int, title: str,
    data: AnalysisData,
) -> Axes:
    """Draw integer-centered cells with a cream valid and grey invalid domain."""
    axis = figure.add_axes(rectangle)
    axis.set(
        xlim=(0.5, limit + 0.5), ylim=(-0.5, limit + 0.5),
        xlabel='Legal moves available, N', ylabel='Moves consumed, k',
    )
    axis.set_title(title, loc='left', fontsize=12, pad=14)
    tick_step = 16 if limit > 80 else 10
    axis.set_xticks([1] + list(range(tick_step, limit + 1, tick_step)))
    axis.set_yticks([0] + list(range(tick_step, limit + 1, tick_step)))
    axis.tick_params(length=0, pad=7)
    axis.set_aspect('equal', adjustable='box')
    axis.imshow(
        ~np.isfinite(data.presort_ns), origin='lower',
        extent=(0.5, data.max_n + 0.5, -0.5, data.max_n + 0.5),
        interpolation='nearest', cmap=ListedColormap([BACKGROUND_COLOR, INVALID_COLOR]),
        vmin=0, vmax=1, zorder=0, rasterized=True,
    )
    axis.text(
        0.05, 0.9, 'k > N\nOutside the valid domain', transform=axis.transAxes,
        color=MUTED_COLOR, fontsize=10, va='top',
    )
    return axis


def create_heatmap_panels(figure: Figure, data: AnalysisData) -> list[Axes]:
    """Place the full benchmark and 99%-of-visits detail panels side by side."""
    return [
        create_heatmap_axis(
            figure, [0.07, 0.21, 0.40, 0.56], data.max_n,
            f'Full benchmark domain  /  N ≤ {data.max_n}', data,
        ),
        create_heatmap_axis(
            figure, [0.565, 0.21, 0.36, 0.56], data.zoom_n,
            f'Workload detail  /  N ≤ {data.zoom_n} contains 99% of visits', data,
        ),
    ]


def draw_heatmap(
    axis: Axes, values: np.ma.MaskedArray, colormap: LinearSegmentedColormap,
    normalization: LogNorm | SymLogNorm, data: AnalysisData,
) -> AxesImage:
    """Overlay the measured cells, leaving masked cells transparent."""
    return axis.imshow(
        values, origin='lower',
        extent=(0.5, data.max_n + 0.5, -0.5, data.max_n + 0.5),
        interpolation='nearest', cmap=colormap, norm=normalization,
        zorder=1, rasterized=True,
    )


def add_heatmap_colorbar(figure: Figure, image: AxesImage) -> Colorbar:
    """Add the shared horizontal color scale below the heatmap panels."""
    colorbar_axis = figure.add_axes([0.245, 0.112, 0.51, 0.019])
    colorbar = figure.colorbar(image, cax=colorbar_axis, orientation='horizontal')
    colorbar.outline.set_visible(False)
    colorbar.ax.tick_params(length=0, labelsize=9, pad=6)
    return colorbar


def plot_frequency_heatmap(data: AnalysisData) -> Figure:
    """Chart 03: invocation counts with lines at the global mean N and k."""
    figure = create_page(
        '03', 'The shape of the search workload',
        'Observed counts on a logarithmic scale. Dashed lines: '
        f'average N = {data.observed_n_mean:.2f}; average k = {data.observed_k_mean:.2f} (all visits).',
    )
    axes = create_heatmap_panels(figure, data)
    # Logarithms cannot display zero; transparency exposes the cream background.
    values = np.ma.masked_where(
        ~np.isfinite(data.invocations) | (data.invocations <= 0), data.invocations,
    )
    colormap = LinearSegmentedColormap.from_list(
        'workload', ['#DCECEA', '#83B9B3', '#218B8D', '#145168', '#172E41'],
    )
    colormap.set_bad((0, 0, 0, 0))
    normalization = LogNorm(vmin=1, vmax=max(10.0, float(values.max())))
    for axis in axes:
        image = draw_heatmap(axis, values, colormap, normalization, data)
        # Use the same global means in both panels, including the zoomed view.
        axis.axvline(data.observed_n_mean, color=TEXT_COLOR, linestyle='--', linewidth=1.2)
        axis.axhline(data.observed_k_mean, color=TEXT_COLOR, linestyle='--', linewidth=1.2)
        # Keep the invalid-domain note readable where a reference line crosses it.
        for annotation in axis.texts:
            annotation.set_bbox(dict(facecolor=INVALID_COLOR, edgecolor='none', pad=2))

    colorbar = add_heatmap_colorbar(figure, image)
    colorbar.set_ticks([
        10 ** exponent for exponent in range(10)
        if 10 ** exponent <= normalization.vmax
    ])
    colorbar.ax.xaxis.set_major_formatter(FuncFormatter(compact_number))
    colorbar.set_label(
        'Observed count per (N, k) cell  /  logarithmic color scale', labelpad=6,
    )
    axes[0].text(
        0.05, 0.71,
        f'k = 1: {data.one_move_share:.1%} of visits\n'
        f'k = N: {data.full_consumption_share:.1%} of visits',
        transform=axes[0].transAxes, fontsize=10,
        color=LAZYSELECT_COLOR, linespacing=1.6,
        bbox=dict(facecolor=INVALID_COLOR, edgecolor='none', pad=2),
    )
    add_footer(
        figure,
        'k = 1 and k = N shares overlap at N = 1. Missing frequency rows are zero; the grey triangle is invalid.',
    )
    return figure


def plot_winner_heatmap(data: AnalysisData) -> Figure:
    """Chart 04: unweighted LazySelect-minus-Presort costs and first wins."""
    figure = create_page(
        '04', 'Where each picker is faster',
        'Signed cost difference: LazySelect − Presort. Orange favors Presort; teal favors LazySelect.',
    )
    axes = create_heatmap_panels(figure, data)
    values = np.ma.masked_invalid(data.lazyselect_ns - data.presort_ns)
    color_limit = max(10.0, float(np.max(np.abs(values))))
    colormap = LinearSegmentedColormap.from_list(
        'picker_delta',
        [LAZYSELECT_COLOR, '#B6D9DB', BACKGROUND_COLOR, '#EDC4B3', PRESORT_COLOR],
    )
    colormap.set_bad((0, 0, 0, 0))
    normalization = SymLogNorm(
        linthresh=10, linscale=1, vmin=-color_limit, vmax=color_limit, base=10,
    )
    for axis in axes:
        image = draw_heatmap(axis, values, colormap, normalization, data)
        axis.plot(
            data.n_values, data.first_presort_k, color=TEXT_COLOR,
            linewidth=1, linestyle=(0, (3, 3)), alpha=0.7,
        )

    colorbar = add_heatmap_colorbar(figure, image)
    tick_candidates = [-10000, -1000, -100, -10, 0, 10, 100, 1000, 10000]
    colorbar.set_ticks([value for value in tick_candidates if abs(value) <= color_limit])
    colorbar.ax.xaxis.set_major_formatter(FuncFormatter(compact_number))
    colorbar.set_label(
        'LazySelect advantage  ←   cost difference (ns)   →  Presort advantage',
        labelpad=6,
    )
    add_footer(
        figure,
        'Symmetric log color scale; linear within ±10 ns. Dashed = first Presort win. Costs are unweighted.',
    )
    return figure


def add_frontier_detail(figure: Figure, data: AnalysisData) -> None:
    """Inset the same sample-filtered consumption and crossover curves."""
    q25_k = mask_sparse_sizes(data.q25_k, data)
    q75_k = mask_sparse_sizes(data.q75_k, data)
    median_k = mask_sparse_sizes(data.median_k, data)
    mean_k = mask_sparse_sizes(data.mean_k, data)
    sustained_k = mask_sparse_sizes(data.sustained_presort_k, data)
    first_k = mask_sparse_sizes(data.first_presort_k, data)
    detail = figure.add_axes([0.675, 0.47, 0.225, 0.20], facecolor=BACKGROUND_COLOR)
    detail.fill_between(
        data.n_values, q25_k, q75_k, color=LAZYSELECT_COLOR, alpha=0.11,
    )
    detail.plot(data.n_values, median_k, color=LAZYSELECT_COLOR, linewidth=1.6)
    detail.plot(
        data.n_values, mean_k, color=LAZYSELECT_COLOR,
        linewidth=1, linestyle=':',
    )
    detail.plot(
        data.n_values, sustained_k, color=PRESORT_COLOR, linewidth=1.7,
    )
    detail.plot(
        data.n_values, first_k, color=TEXT_COLOR,
        linewidth=1, linestyle='--',
    )
    shade_sparse_sizes(detail, data)
    visible_max = finite_maximum(
        q75_k[:data.zoom_n], mean_k[:data.zoom_n],
        first_k[:data.zoom_n], sustained_k[:data.zoom_n],
    )
    detail.set(xlim=(0.5, data.zoom_n + 0.5), ylim=(0, visible_max * 1.1))
    detail.set_title(
        f'Detail: 99% of visits / N ≤ {data.zoom_n}', fontsize=9, loc='left', pad=8,
    )
    style_axis(detail)
    detail.tick_params(labelsize=8)
    detail.set_xlabel('N', fontsize=9, labelpad=1)


def plot_crossover_frontier(data: AnalysisData) -> Figure:
    """Chart 05: compare win thresholds and consumption at supported N only."""
    q25_k = mask_sparse_sizes(data.q25_k, data)
    q75_k = mask_sparse_sizes(data.q75_k, data)
    median_k = mask_sparse_sizes(data.median_k, data)
    mean_k = mask_sparse_sizes(data.mean_k, data)
    sustained_k = mask_sparse_sizes(data.sustained_presort_k, data)
    first_k = mask_sparse_sizes(data.first_presort_k, data)
    supported_n = data.n_values[data.supported_n]
    visits_percent = data.visits_percent[data.supported_n]
    figure = create_page(
        '05', 'Does search reach the break-even point?',
        f'Consumption and benchmark frontiers shown for N with at least '
        f'{MIN_OBSERVATIONS_PER_N:,} observations. Shaded sizes are omitted.',
    )
    frontier_axis = figure.add_axes([0.08, 0.345, 0.845, 0.425])
    frequency_axis = figure.add_axes([0.08, 0.15, 0.845, 0.12], sharex=frontier_axis)
    frontier_axis.fill_between(
        data.n_values, q25_k, q75_k,
        color=LAZYSELECT_COLOR, alpha=0.11, label='Observed middle 50%',
    )
    frontier_axis.plot(
        data.n_values, median_k, color=LAZYSELECT_COLOR,
        linewidth=2.5, label='Observed median k',
    )
    frontier_axis.plot(
        data.n_values, mean_k, color=LAZYSELECT_COLOR,
        linewidth=1.2, linestyle=':', label='Observed mean k',
    )
    frontier_axis.plot(
        data.n_values, sustained_k, color=PRESORT_COLOR,
        linewidth=2.5, label='Presort stays strictly faster',
    )
    frontier_axis.plot(
        data.n_values, first_k, color=TEXT_COLOR,
        linestyle=(0, (4, 3)), linewidth=1.3, marker='.', markersize=3,
        label='First strict Presort win',
    )
    shade_sparse_sizes(frontier_axis, data)
    largest_k = finite_maximum(q75_k, mean_k, median_k, first_k, sustained_k)
    frontier_axis.set(xlim=(0.5, data.max_n + 0.5), ylim=(0, largest_k * 1.24))
    annotate_sparse_tail(frontier_axis, data)
    style_axis(frontier_axis, 'Moves consumed, k')
    frontier_axis.tick_params(labelbottom=False)
    frontier_axis.legend(loc='upper left', ncol=2, fontsize=9)
    if 0 < data.supported_max_n < data.max_n - 25:
        add_frontier_detail(figure, data)

    frequency_axis.bar(
        supported_n, visits_percent, color=FREQUENCY_COLOR, width=0.82,
    )
    shade_sparse_sizes(frequency_axis, data)
    style_axis(frequency_axis, 'All visits (%)')
    frequency_axis.set_xlabel('Legal moves available, N', labelpad=10)
    frequency_axis.set_ylim(0, max(float(visits_percent.max(initial=0)) * 1.18, 1))
    add_footer(
        figure,
        'No marker means no strict win. Mean/median k cannot substitute for frequency-weighted expected cost.',
    )
    return figure


def add_expected_cost_detail(figure: Figure, data: AnalysisData) -> None:
    """Inset the same sample-filtered conditional costs as the main panel."""
    presort_ns = mask_sparse_sizes(data.expected_presort_ns, data)
    lazyselect_ns = mask_sparse_sizes(data.expected_lazyselect_ns, data)
    detail = figure.add_axes([0.68, 0.415, 0.22, 0.22], facecolor=BACKGROUND_COLOR)
    detail.plot(
        data.n_values, presort_ns, color=PRESORT_COLOR, linewidth=1.8,
    )
    detail.plot(
        data.n_values, lazyselect_ns, color=LAZYSELECT_COLOR, linewidth=1.8,
    )
    shade_sparse_sizes(detail, data)
    visible_max = finite_maximum(presort_ns[:data.zoom_n], lazyselect_ns[:data.zoom_n])
    detail.set(xlim=(0.5, data.zoom_n + 0.5), ylim=(0, visible_max * 1.1))
    detail.set_title(
        f'Detail: 99% of visits / N ≤ {data.zoom_n}', fontsize=9, loc='left', pad=8,
    )
    style_axis(detail)
    detail.tick_params(labelsize=8)
    detail.set_xlabel('N', fontsize=9, labelpad=1)


def plot_expected_cost(data: AnalysisData) -> Figure:
    """Chart 06: conditional costs and visit shares at supported N only."""
    presort_ns = mask_sparse_sizes(data.expected_presort_ns, data)
    lazyselect_ns = mask_sparse_sizes(data.expected_lazyselect_ns, data)
    supported_n = data.n_values[data.supported_n]
    visits_percent = data.visits_percent[data.supported_n]
    figure = create_page(
        '06', 'The best picker depends on list size',
        f'Expected costs shown for N with at least {MIN_OBSERVATIONS_PER_N:,} observations. '
        'The lower strip shows each N’s share of all visits.',
    )
    cost_axis = figure.add_axes([0.09, 0.37, 0.83, 0.40])
    frequency_axis = figure.add_axes([0.09, 0.15, 0.83, 0.145], sharex=cost_axis)
    cost_axis.plot(
        data.n_values, presort_ns, color=PRESORT_COLOR, label='Presort',
    )
    cost_axis.plot(
        data.n_values, lazyselect_ns, color=LAZYSELECT_COLOR, label='LazySelect',
    )
    cost_axis.fill_between(
        data.n_values, presort_ns, lazyselect_ns,
        where=lazyselect_ns >= presort_ns,
        color=PRESORT_COLOR, alpha=0.08, interpolate=True,
    )
    cost_axis.fill_between(
        data.n_values, presort_ns, lazyselect_ns,
        where=presort_ns > lazyselect_ns,
        color=LAZYSELECT_COLOR, alpha=0.08, interpolate=True,
    )
    cost_axis.set_ylim(0, finite_maximum(presort_ns, lazyselect_ns) * 1.15)
    shade_sparse_sizes(cost_axis, data)
    cost_axis.set_xlim(0.5, data.max_n + 0.5)
    annotate_sparse_tail(cost_axis, data)
    style_axis(cost_axis, 'Expected picker cost (ns)')
    cost_axis.legend(handles=picker_legend_handles(), loc='upper left', ncol=2)
    cost_axis.tick_params(labelbottom=False)
    if 0 < data.supported_max_n < data.max_n - 25:
        add_expected_cost_detail(figure, data)

    bar_colors = []
    for presort, lazyselect in zip(presort_ns[data.supported_n], lazyselect_ns[data.supported_n]):
        if presort < lazyselect:
            bar_colors.append(PRESORT_COLOR)
        elif lazyselect < presort:
            bar_colors.append(LAZYSELECT_COLOR)
        else:
            # Ties and undefined means both retain the neutral frequency color.
            bar_colors.append(FREQUENCY_COLOR)
    frequency_axis.bar(supported_n, visits_percent, color=bar_colors, width=0.82)
    shade_sparse_sizes(frequency_axis, data)
    style_axis(frequency_axis, 'All visits (%)')
    frequency_axis.set_xlabel('Legal moves available, N', labelpad=10)
    frequency_axis.set_ylim(0, max(float(visits_percent.max(initial=0)) * 1.15, 1))
    add_footer(
        figure,
        'E[cost | N] = Σk count(N,k) × cost(N,k) / Σk count(N,k). Bar color = lower conditional mean.',
    )
    return figure


def plot_workload_impact(data: AnalysisData) -> Figure:
    """Chart 07: each N's contribution to the global cost difference."""
    contribution_ns = np.nan_to_num(
        (data.expected_lazyselect_ns - data.expected_presort_ns)
        * data.invocations_by_n / data.total_invocations
    )
    figure = create_page(
        '07', 'Where the total cost difference comes from',
        'Each bar combines prevalence and cost advantage. Together, the bars sum to the global weighted difference.',
    )
    impact_axis = figure.add_axes([0.09, 0.37, 0.83, 0.40])
    impact_axis.bar(
        data.n_values, contribution_ns,
        color=np.where(contribution_ns >= 0, PRESORT_COLOR, LAZYSELECT_COLOR), width=0.82,
    )
    impact_axis.axhline(0, color=TEXT_COLOR, linewidth=1)
    impact_axis.set_xlim(0.5, data.max_n + 0.5)
    style_axis(impact_axis, 'Contribution to mean difference (ns)')
    impact_axis.set_xlabel('Legal moves available, N', labelpad=10)
    impact_axis.legend(
        handles=[
            Patch(color=PRESORT_COLOR, label='Presort saves time'),
            Patch(color=LAZYSELECT_COLOR, label='LazySelect saves time'),
        ],
        loc='upper right', ncol=2,
    )

    best_single_mean = min(data.presort_mean_ns, data.lazyselect_mean_ns)
    policy_gain_percent = (
        (best_single_mean - data.n_policy_mean_ns) / best_single_mean * 100
        if best_single_mean else 0
    )
    policy_comparisons = [
        (0.09, 'BEST SINGLE PICKER', best_single_mean, 'Frequency-weighted mean'),
        (0.39, 'CHOOSE USING N', data.n_policy_mean_ns,
         f'{policy_gain_percent:.2f}% below best single picker'),
        (0.69, 'KNOW N AND FUTURE k', data.oracle_mean_ns, 'Clairvoyant lower bound'),
    ]
    for x, label, value, note in policy_comparisons:
        figure.text(x, 0.225, label, color=MUTED_COLOR, fontsize=9, weight='bold')
        figure.text(x, 0.165, f'{value:.2f} ns', color=TEXT_COLOR, fontsize=25, weight='bold')
        figure.text(x, 0.125, note, color=MUTED_COLOR, fontsize=10)
    add_footer(
        figure,
        'N-only choice is fitted on this workload, without dispatch overhead. Future k is unknown when choosing a picker.',
    )
    return figure


def main() -> None:
    data = load_report_data(REPORT_DIRECTORY / 'cost.csv', REPORT_DIRECTORY / 'freq.csv')
    
    REPORT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    with plt.rc_context(REPORT_STYLE):
        save_png(plot_summary(data), REPORT_DIRECTORY / '01_summary.png')
        write_fixed_n_pdf(data, REPORT_DIRECTORY / '02_fixed_n.pdf')
        save_png(plot_frequency_heatmap(data), REPORT_DIRECTORY / '03_frequency_heatmap.png')
        save_png(plot_winner_heatmap(data), REPORT_DIRECTORY / '04_winner_heatmap.png')
        save_png(plot_crossover_frontier(data), REPORT_DIRECTORY / '05_crossover_frontier.png')
        save_png(plot_expected_cost(data), REPORT_DIRECTORY / '06_expected_cost_by_n.png')
        save_png(plot_workload_impact(data), REPORT_DIRECTORY / '07_workload_impact.png')


if __name__ == '__main__':
    main()
