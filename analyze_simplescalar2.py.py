#!/usr/bin/env python3
"""
Analyze SimpleScalar Design Space Exploration (.log and .best) outputs
and generate report benchmarking metrics and plots for EDP, ED2P, EDAP, or ED2AP.
"""

import argparse
import csv
import math
from pathlib import Path
import matplotlib.pyplot as plt

DIMENSION_NAMES = [
    "width",
    "fetchspeed",
    "scheduling",
    "RUUsize",
    "LSQsize",
    "Memports",
    "dl1sets",
    "dl1assoc",
    "il1sets",
    "il1assoc",
    "ul2sets",
    "ul2blocksize",
    "ul2assoc",
    "tlbsets",
    "dl1lat",
    "il1lat",
    "ul2lat",
    "bpred",
]

METRIC_SPECS = {
    "EDP": {
        "label": "EDP",
        "full_name": "Energy-Delay Product (EDP)",
        "log_norm_idx": 0,
        "log_abs_idx": 4,
        "best_line_idx": 0,
    },
    "ED2P": {
        "label": "ED2P",
        "full_name": "Energy-Delay² Product (ED²P)",
        "log_norm_idx": 1,
        "log_abs_idx": 5,
        "best_line_idx": 1,
    },
    "EDAP": {
        "label": "EDAP",
        "full_name": "Energy-Delay-Area Product (EDAP)",
        "log_norm_idx": 2,
        "log_abs_idx": 6,
        "best_line_idx": 2,
    },
    "ED2AP": {
        "label": "ED2AP",
        "full_name": "Energy-Delay²-Area Product (ED²AP)",
        "log_norm_idx": 3,
        "log_abs_idx": 7,
        "best_line_idx": 3,
    },
}


def load_log_series(log_path: Path, norm_idx: int, abs_idx: int):
    """Load normalized and absolute metric series from a .log CSV file."""
    norm_vals = []
    abs_vals = []

    with log_path.open("r", newline="") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or len(row) <= max(norm_idx, abs_idx):
                continue
            try:
                n_val = float(row[norm_idx].strip())
                a_val = float(row[abs_idx].strip())
            except ValueError:
                continue
            norm_vals.append(n_val)
            abs_vals.append(a_val)

    if not norm_vals:
        raise ValueError(f"No numeric rows found in log file: {log_path}")

    return norm_vals, abs_vals


def load_best_record(best_path: Path, line_idx: int):
    """
    Parse the line corresponding to the optimization metric from .best file.
    Expected layout per line (from FAQ):
      [0] config_str
      [1..4] norm EDP, norm ED2P, norm EDAP, norm ED2AP
      [5..8] abs EDP, abs ED2P, abs EDAP, abs ED2AP
      [9..18] pairs of (abs_bench_i, norm_bench_i) for i = 0..4
    """
    with best_path.open("r", newline="") as f:
        lines = [line.strip() for line in f if line.strip()]

    if line_idx >= len(lines):
        raise ValueError(
            f"Expected at least {line_idx + 1} lines in {best_path}, found {len(lines)}."
        )

    parts = [p.strip() for p in lines[line_idx].split(",") if p.strip() != ""]
    if len(parts) < 19:
        raise ValueError(
            f"Unexpected column count ({len(parts)}) on line {line_idx + 1} of {best_path}."
        )

    config_str = parts[0]
    norm_geomeans = {
        "EDP": float(parts[1]),
        "ED2P": float(parts[2]),
        "EDAP": float(parts[3]),
        "ED2AP": float(parts[4]),
    }
    abs_geomeans = {
        "EDP": float(parts[5]),
        "ED2P": float(parts[6]),
        "EDAP": float(parts[7]),
        "ED2AP": float(parts[8]),
    }

    bench_abs = []
    bench_norm = []
    for i in range(5):
        base = 9 + 2 * i
        bench_abs.append(float(parts[base]))
        bench_norm.append(float(parts[base + 1]))

    return {
        "config": config_str,
        "norm_geomeans": norm_geomeans,
        "abs_geomeans": abs_geomeans,
        "bench_abs": bench_abs,
        "bench_norm": bench_norm,
    }


def parse_config_tokens(config_str: str):
    """Parse space- or comma-separated tokens from a configuration string."""
    cleaned = config_str.replace(",", " ").split()
    return cleaned


def compute_geomean(values):
    positive = [v for v in values if v > 0 and math.isfinite(v)]
    if not positive:
        return float("nan")
    return math.exp(sum(math.log(v) for v in positive) / len(positive))


def filter_valid_points(values, max_threshold=1e6):
    """Separate plausible normalized values from 0xBAD / invalid spikes."""
    valid = [
        (idx + 1, v)
        for idx, v in enumerate(values)
        if math.isfinite(v) and 0.0 < v < max_threshold
    ]
    invalid_count = len(values) - len(valid)
    return valid, invalid_count


def print_benchmark_report(metric_key: str, norm_vals, abs_vals, best_rec: dict, decode_best: bool):
    spec = METRIC_SPECS[metric_key]
    valid_pts, invalid_count = filter_valid_points(norm_vals)
    valid_only_vals = [v for _, v in valid_pts]

    min_val = min(valid_only_vals) if valid_only_vals else min(norm_vals)
    min_idx = norm_vals.index(min_val) + 1 if min_val in norm_vals else -1
    running_best = []
    cur_best = float("inf")
    for v in norm_vals:
        if math.isfinite(v) and v > 0 and v < cur_best:
            cur_best = v
        running_best.append(cur_best)

    best_norm_metric = best_rec["norm_geomeans"][metric_key]
    best_abs_metric = best_rec["abs_geomeans"][metric_key]
    reduction_vs_baseline = (1.0 - best_norm_metric) * 100.0
    computed_bench_geomean = compute_geomean(best_rec["bench_norm"])

    print("=" * 68)
    print(f"BENCHMARK SUMMARY — {spec['full_name']} ({spec['label']})")
    print("=" * 68)
    print(f"Total points logged          : {len(norm_vals)}")
    print(f"Valid points (< 1e6)         : {len(valid_pts)}")
    print(f"Filtered/outlier points      : {invalid_count}")
    print(f"Best config string           : {best_rec['config']}")
    print(f"Best normalized {spec['label']}       : {best_norm_metric:.6g} (vs. baseline = 1.0)")
    print(f"Change vs. baseline          : {reduction_vs_baseline:+.2f}% (negative = lower/better)")
    print(f"Best absolute {spec['label']}         : {best_abs_metric:.6e}")
    print(f"Min valid point in .log      : {min_val:.6g} (at evaluation #{min_idx})")
    print(f"Final running-best norm {spec['label']}: {running_best[-1]:.6g}")
    print("-" * 68)
    print("Per-Benchmark Normalized Breakdown (Best Config):")
    for i, b_norm in enumerate(best_rec["bench_norm"]):
        b_abs = best_rec["bench_abs"][i]
        print(f"  Bench {i}: norm = {b_norm:.6g} | abs = {b_abs:.6e}")
    print(f"  Reported/Computed Geomean    : {best_norm_metric:.6g} (bench geomean: {computed_bench_geomean:.6g})")

    if decode_best:
        tokens = parse_config_tokens(best_rec["config"])
        print("-" * 68)
        print("Dimension Index Map (GLOB_dimensionnames):")
        for idx, name in enumerate(DIMENSION_NAMES):
            raw_val = tokens[idx] if idx < len(tokens) else "<N/A>"
            print(f"  [{idx:2d}] {name:<14}: index={raw_val}")

    print("=" * 68)


def plot_line_chart(metric_key: str, norm_vals, out_path: Path, include_running_best: bool = True):
    spec = METRIC_SPECS[metric_key]
    valid_pts, invalid_count = filter_valid_points(norm_vals)

    xs = [x for x, _ in valid_pts]
    ys = [y for _, y in valid_pts]

    fig, ax = plt.subplots(figsize=(8.5, 4.8), dpi=300)
    ax.plot(xs, ys, linewidth=1.1, alpha=0.85, label=f"Evaluated Normalized {spec['label']}")

    if include_running_best and ys:
        run_best = []
        best_so_far = float("inf")
        for y in ys:
            best_so_far = min(best_so_far, y)
            run_best.append(best_so_far)
        ax.plot(xs, run_best, linewidth=1.8, linestyle="--", label="Best-So-Far (Running Min)")

    ax.axhline(1.0, color="gray", linestyle=":", linewidth=1.0, label="Baseline (1.0)")
    ax.set_title(f"{spec['label']} Exploration: Normalized Geomean vs. Design Point")
    ax.set_xlabel("Number of Designs Considered")
    ax.set_ylabel(f"Normalized {spec['label']} (Geomean)")
    ax.grid(True, linestyle="--", alpha=0.35)
    ax.legend(loc="best", frameon=True)

    if invalid_count > 0:
        ax.text(
            0.01,
            0.02,
            f"Note: {invalid_count} extreme/invalid point(s) excluded from scale",
            transform=ax.transAxes,
            fontsize=8,
            color="dimgray",
        )

    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)


def plot_bar_chart(metric_key: str, best_rec: dict, out_path: Path):
    spec = METRIC_SPECS[metric_key]
    labels = [f"Bench {i}" for i in range(5)] + ["Geomean"]
    values = best_rec["bench_norm"] + [best_rec["norm_geomeans"][metric_key]]

    fig, ax = plt.subplots(figsize=(7.5, 4.6), dpi=300)
    bars = ax.bar(labels, values, width=0.55)
    bars[-1].set_alpha(1.0)
    for b in bars[:-1]:
        b.set_alpha(0.82)

    ax.axhline(1.0, color="gray", linestyle="--", linewidth=1.0, label="Baseline (1.0)")
    ax.set_title(f"Best {spec['label']} Design: Normalized Metric by Benchmark & Geomean")
    ax.set_xlabel("Workload / Aggregate")
    ax.set_ylabel(f"Normalized {spec['label']} (Lower is Better)")
    ax.grid(True, axis="y", linestyle="--", alpha=0.35)
    ax.legend(loc="best", frameon=True)

    for bar, val in zip(bars, values):
        if math.isfinite(val):
            ax.text(
                bar.get_x() + bar.get_width() / 2.0,
                bar.get_height(),
                f"{val:.3g}",
                ha="center",
                va="bottom",
                fontsize=8.5,
            )

    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Analyze DSE .log and .best files and generate report plots."
    )
    parser.add_argument("--log", required=True, type=Path, help="Path to .log file (1000 runs)")
    parser.add_argument("--best", required=True, type=Path, help="Path to .best file")
    parser.add_argument(
        "--metric",
        required=True,
        choices=["EDP", "ED2P", "EDAP", "ED2AP"],
        help="Optimization metric for this run",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("report_plots"),
        help="Directory to save generated plots",
    )
    parser.add_argument(
        "--prefix",
        type=str,
        default=None,
        help="Optional filename prefix (defaults to lowercase metric name)",
    )
    parser.add_argument(
        "--no-running-best",
        action="store_true",
        help="Plot only raw evaluated points on the line chart",
    )
    parser.add_argument(
        "--decode-best",
        action="store_true",
        help="Print dimension names alongside best configuration index values",
    )
    args = parser.parse_args()

    spec = METRIC_SPECS[args.metric]
    args.out_dir.mkdir(parents=True, exist_ok=True)
    prefix = args.prefix or args.metric.lower()

    norm_vals, abs_vals = load_log_series(
        args.log, spec["log_norm_idx"], spec["log_abs_idx"]
    )
    best_rec = load_best_record(args.best, spec["best_line_idx"])

    print_benchmark_report(
        args.metric, norm_vals, abs_vals, best_rec, decode_best=args.decode_best
    )

    line_out = args.out_dir / f"{prefix}_line_geomean.png"
    bar_out = args.out_dir / f"{prefix}_bar_benchmarks.png"

    plot_line_chart(
        args.metric,
        norm_vals,
        line_out,
        include_running_best=not args.no_running_best,
    )
    plot_bar_chart(args.metric, best_rec, bar_out)

    print(f"Saved line chart : {line_out}")
    print(f"Saved bar chart  : {bar_out}")


if __name__ == "__main__":
    main()