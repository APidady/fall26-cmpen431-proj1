def plot_line_chart(metric_key: str, norm_vals, out_path: Path, include_running_best: bool = True):
    spec = METRIC_SPECS[metric_key]
    valid_pts, invalid_count = filter_valid_points(norm_vals)

    xs = [x for x, _ in valid_pts]
    ys = [y for _, y in valid_pts]
    is_area_metric = metric_key in ("EDAP", "ED2AP")

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
    if is_area_metric:
        ax.set_yscale("log")

    ax.set_title(f"{spec['label']} Exploration: Normalized Geomean vs. Design Point")
    ax.set_xlabel("Number of Designs Considered")
    ax.set_ylabel(f"Normalized {spec['label']} (Geomean{', Log Scale' if is_area_metric else ''})")
    ax.grid(True, which="both", linestyle="--", alpha=0.35)
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
    is_area_metric = metric_key in ("EDAP", "ED2AP")

    fig, ax = plt.subplots(figsize=(7.5, 4.6), dpi=300)
    bars = ax.bar(labels, values, width=0.55)
    bars[-1].set_alpha(1.0)
    for b in bars[:-1]:
        b.set_alpha(0.82)

    if not is_area_metric:
        ax.axhline(1.0, color="gray", linestyle="--", linewidth=1.0, label="Baseline (1.0)")
        ax.legend(loc="best", frameon=True)

    ymax = max(v for v in values if math.isfinite(v))
    ax.set_ylim(0, ymax * 1.18 if is_area_metric else max(1.08, ymax * 1.15))

    title_suffix = " (Baseline = 1.0)" if is_area_metric else ""
    ax.set_title(f"Best {spec['label']} Design: Normalized Metric by Benchmark & Geomean{title_suffix}")
    ax.set_xlabel("Workload / Aggregate")
    ax.set_ylabel(f"Normalized {spec['label']} (Lower is Better)")
    ax.grid(True, axis="y", linestyle="--", alpha=0.35)

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