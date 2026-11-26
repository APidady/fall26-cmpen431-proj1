#!/usr/bin/env python3
"""
Analyze SimpleScalar project output files.

Expected inputs:
  minED2P.log  : 1001 CSV rows x 8 numeric columns (baseline first,
                 followed by evaluated designs). The file has no header.
  minED2P.best : one or more lines. Each line contains 18 space-separated
                 configuration indices before the first comma, followed by
                 comma-separated numeric result fields.

The script deliberately labels result columns generically (metric_1, ...)
because the supplied output files do not identify the meaning/order of all
numeric fields. Pass --log-labels if you know the framework's column names.

Outputs are written to ./analysis_output by default:
  log_with_baseline.csv
  log_summary.csv
  best_candidates.csv
  timeseries.png
  best_so_far.png
  metric_distributions.png
  best_candidate_metrics.png (when .best is provided)
"""

import argparse
import csv
import math
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


CONFIG_NAMES = [
    "width", "fetch_speed", "scheduling", "ruu_size", "lsq_size",
    "memport", "dl1_sets", "dl1_assoc", "il1_sets", "il1_assoc",
    "ul2_sets", "ul2_block", "ul2_assoc", "tlb_sets",
    "dl1_latency", "il1_latency", "ul2_latency", "branch_predictor",
]


def read_log(path, labels=None):
    """Read the headerless 8-column log; row 0 is treated as baseline."""
    raw = pd.read_csv(path, header=None, comment="#")
    if raw.shape[1] != 8:
        raise ValueError(
            f"{path}: expected 8 columns based on the supplied log, "
            f"but found {raw.shape[1]}."
        )
    raw = raw.apply(pd.to_numeric, errors="coerce")
    if raw.isna().any().any():
        bad = np.argwhere(raw.isna().to_numpy())[0]
        raise ValueError(f"{path}: non-numeric value at row {bad[0] + 1}, "
                         f"column {bad[1] + 1}.")
    names = labels or [f"metric_{i}" for i in range(1, 9)]
    if len(names) != 8:
        raise ValueError("--log-labels must contain exactly 8 comma-separated names.")
    raw.columns = names
    raw.insert(0, "evaluation", np.arange(len(raw)))
    raw.insert(1, "point_type",
               ["baseline"] + ["design"] * (len(raw) - 1))
    return raw


def read_best(path):
    """Parse .best lines: 18 integer indices, then comma-separated values."""
    rows = []
    with open(path, "r", encoding="utf-8") as handle:
        for line_no, line in enumerate(handle, start=1):
            line = line.strip()
            if not line:
                continue
            parts = line.split(",", 1)
            if len(parts) != 2:
                raise ValueError(f"{path}:{line_no}: missing comma after configuration.")
            config = parts[0].split()
            if len(config) != 18:
                raise ValueError(
                    f"{path}:{line_no}: expected 18 configuration indices, "
                    f"found {len(config)}."
                )
            try:
                config = [int(x) for x in config]
                values = [float(x) for x in parts[1].split(",") if x.strip()]
            except ValueError as exc:
                raise ValueError(f"{path}:{line_no}: invalid numeric field.") from exc

            row = dict(zip(CONFIG_NAMES, config))
            for i, value in enumerate(values, start=1):
                row[f"result_{i}"] = value
            row["source_line"] = line_no
            rows.append(row)

    if not rows:
        raise ValueError(f"{path}: no candidate rows found.")
    return pd.DataFrame(rows)


def save_log_analysis(log_df, outdir):
    outdir.mkdir(parents=True, exist_ok=True)
    log_df.to_csv(outdir / "log_with_baseline.csv", index=False)

    metric_cols = list(log_df.columns[2:])
    designs = log_df[log_df["point_type"] == "design"].copy()

    summary_rows = []
    for col in metric_cols:
        values = designs[col]
        baseline = float(log_df.loc[log_df["point_type"] == "baseline", col].iloc[0])
        min_idx = values.idxmin()
        max_idx = values.idxmax()
        min_value = float(log_df.loc[min_idx, col])
        summary_rows.append({
            "metric": col,
            "baseline": baseline,
            "design_count": int(values.count()),
            "minimum": min_value,
            "minimum_evaluation": int(log_df.loc[min_idx, "evaluation"]),
            "minimum_over_baseline": min_value / baseline if baseline else np.nan,
            "maximum": float(log_df.loc[max_idx, col]),
            "maximum_evaluation": int(log_df.loc[max_idx, "evaluation"]),
            "mean": float(values.mean()),
            "median": float(values.median()),
            "stddev": float(values.std()),
        })
    pd.DataFrame(summary_rows).to_csv(outdir / "log_summary.csv", index=False)

    # Plot all eight raw series. This is useful for spotting convergence,
    # repeated points, and abrupt changes without assuming metric semantics.
    fig, axes = plt.subplots(4, 2, figsize=(14, 13), sharex=True)
    for ax, col in zip(axes.flat, metric_cols):
        ax.plot(log_df["evaluation"], log_df[col], linewidth=1)
        ax.axhline(log_df.loc[0, col], linestyle="--", linewidth=0.9,
                   label="baseline")
        ax.set_title(col)
        ax.set_ylabel("reported value")
        ax.grid(True, alpha=0.25)
        ax.legend()
    axes[-1, 0].set_xlabel("evaluation index (0 = baseline)")
    axes[-1, 1].set_xlabel("evaluation index (0 = baseline)")
    fig.suptitle("SimpleScalar output metrics by evaluation")
    fig.tight_layout()
    fig.savefig(outdir / "timeseries.png", dpi=180)
    plt.close(fig)

    # Running minimum assumes smaller is better. This matches minimization
    # objectives, but users should map only the columns that are actual costs.
    fig, axes = plt.subplots(4, 2, figsize=(14, 13), sharex=True)
    for ax, col in zip(axes.flat, metric_cols):
        running = log_df[col].cummin()
        ax.plot(log_df["evaluation"], running, linewidth=1.4)
        ax.axhline(log_df.loc[0, col], linestyle="--", linewidth=0.9,
                   label="baseline")
        ax.set_title(f"{col}: running minimum")
        ax.set_ylabel("best value so far")
        ax.grid(True, alpha=0.25)
        ax.legend()
    axes[-1, 0].set_xlabel("evaluation index")
    axes[-1, 1].set_xlabel("evaluation index")
    fig.suptitle("Best-so-far curves (lower is better)")
    fig.tight_layout()
    fig.savefig(outdir / "best_so_far.png", dpi=180)
    plt.close(fig)

    # Boxplots of design results (excluding baseline).
    fig, ax = plt.subplots(figsize=(13, 6))
    ax.boxplot([designs[c].to_numpy() for c in metric_cols],
               tick_labels=metric_cols, showfliers=False)
    ax.set_title("Distribution of evaluated design results")
    ax.set_ylabel("reported value")
    ax.tick_params(axis="x", rotation=30)
    ax.grid(True, axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(outdir / "metric_distributions.png", dpi=180)
    plt.close(fig)


def save_best_analysis(best_df, outdir):
    best_df.to_csv(outdir / "best_candidates.csv", index=False)
    result_cols = [c for c in best_df.columns if c.startswith("result_")]
    if result_cols:
        fig, ax = plt.subplots(figsize=(12, 6))
        x = np.arange(len(best_df))
        for col in result_cols:
            ax.plot(x, best_df[col], marker="o", linewidth=1, label=col)
        ax.set_title("Result fields in minED2P.best")
        ax.set_xlabel("candidate row")
        ax.set_ylabel("reported value")
        ax.grid(True, alpha=0.25)
        ax.legend(ncol=3)
        fig.tight_layout()
        fig.savefig(outdir / "best_candidate_metrics.png", dpi=180)
        plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", default="minED2P.log",
                        help="SimpleScalar .log file (default: minED2P.log)")
    parser.add_argument("--best", default="minED2P.best",
                        help="Optional .best file (default: minED2P.best)")
    parser.add_argument("--out", default="analysis_output",
                        help="Output directory (default: analysis_output)")
    parser.add_argument(
        "--log-labels",
        help=("Optional comma-separated names for the 8 log columns, in file order. "
              "If omitted, generic metric_1..metric_8 labels are used.")
    )
    args = parser.parse_args()

    labels = args.log_labels.split(",") if args.log_labels else None
    outdir = Path(args.out)
    log_df = read_log(args.log, labels)
    save_log_analysis(log_df, outdir)

    print(f"Read {len(log_df) - 1} design rows plus 1 baseline row from {args.log}.")
    print(f"Log analysis saved in: {outdir.resolve()}")
    print("\nPer-column summary:")
    print(pd.read_csv(outdir / "log_summary.csv").to_string(index=False))

    if args.best and Path(args.best).exists():
        best_df = read_best(args.best)
        save_best_analysis(best_df, outdir)
        print(f"\nRead {len(best_df)} candidate rows from {args.best}.")
        print(f"Candidate table saved to: {(outdir / 'best_candidates.csv').resolve()}")
    elif args.best:
        print(f"\nBest file not found; skipped: {args.best}")


if __name__ == "__main__":
    main()
