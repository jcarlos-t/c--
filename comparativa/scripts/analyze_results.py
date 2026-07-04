#!/usr/bin/env python3
"""
Analiza CSV de comparativa/ y genera tablas, speedups y gráficos con matplotlib.
"""

import csv
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

RESULTS_DIR = Path(__file__).parent.parent / "results"
CHARTS_DIR = RESULTS_DIR / "charts"


def read_csv(filename):
    path = RESULTS_DIR / filename
    if not path.exists():
        print(f"Aviso: no existe {filename}")
        return None, []
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        rows = list(reader)
    if not rows:
        return None, []
    columns = [c for c in rows[0].keys() if c != "Benchmark"]
    return columns, rows


def to_float(val):
    try:
        return float(val)
    except (TypeError, ValueError):
        return None


def format_time(value):
    v = to_float(value)
    if v is None:
        return str(value) if value else "N/A"
    if v < 1:
        return f"{v * 1000:.1f} ms"
    return f"{v:.3f} s"


def format_size(value):
    try:
        v = int(value)
    except (TypeError, ValueError):
        return str(value) if value else "N/A"
    if v >= 1024 * 1024:
        return f"{v / (1024 * 1024):.2f} MB"
    if v >= 1024:
        return f"{v / 1024:.1f} KB"
    return f"{v} B"


def print_table(title, columns, rows, formatter):
    print(f"\n{title}")
    print("=" * min(100, 12 + 18 * len(columns)))
    if not rows:
        print("Sin datos")
        return
    header = f"{'Benchmark':<18}"
    for c in columns:
        header += f"{c:>16}"
    print(header)
    print("-" * len(header))
    for row in rows:
        line = f"{row['Benchmark']:<18}"
        for c in columns:
            line += f"{formatter(row.get(c, '')):>16}"
        print(line)


def speedup_rows(rows, baseline_col, target_col):
    out = []
    for row in rows:
        b = to_float(row.get(baseline_col))
        t = to_float(row.get(target_col))
        if b and t and t > 0:
            out.append((row["Benchmark"], b / t, baseline_col, target_col))
    return out


def plot_grouped_bars(path, title, labels, series, y_label, log_scale=False):
    """series: list of (name, [values aligned with labels])"""
    fig, ax = plt.subplots(figsize=(max(8, len(labels) * 1.2), 5))

    colors = ["#4C72B0", "#55A868", "#C44E52", "#8172B2", "#CCB974", "#64B5CD"]
    n_series = len(series)
    x = range(len(labels))
    width = 0.8 / n_series

    for si, (sname, vals) in enumerate(series):
        clean = [v if v is not None and v > 0 else 0 for v in vals]
        offset = (si - n_series / 2 + 0.5) * width
        bars = ax.bar(
            [xi + offset for xi in x], clean, width * 0.9,
            label=sname, color=colors[si % len(colors)]
        )

    short_labels = [l.replace("bench_", "") for l in labels]
    ax.set_xticks(x)
    ax.set_xticklabels(short_labels, rotation=25, ha="right", fontsize=9)

    ax.set_ylabel(y_label)
    ax.set_title(title)
    ax.legend(fontsize=8)

    if log_scale:
        ax.set_yscale("log")
        ax.grid(True, which="both", axis="y", linestyle=":", alpha=0.4)
    else:
        ax.grid(True, axis="y", linestyle=":", alpha=0.4)
    ax.set_axisbelow(True)

    plt.tight_layout()
    fig.savefig(path, format="svg", bbox_inches="tight")
    plt.close(fig)


def generate_charts(exec_cols, exec_rows, size_cols, size_rows, comp_cols, comp_rows,
                     frame_cols=None, frame_rows=None):
    CHARTS_DIR.mkdir(parents=True, exist_ok=True)
    labels = [r["Benchmark"] for r in exec_rows]

    def col_values(rows, col):
        return [to_float(r.get(col)) for r in rows]

    if exec_rows:
        series = []
        for c in exec_cols:
            if c == "C--_Compiler":
                series.append((c, col_values(exec_rows, c)))
        for c in ["GCC_O0", "GCC_O2", "Clang_O0", "Clang_O2"]:
            if c in exec_cols:
                series.append((c, col_values(exec_rows, c)))
        plot_grouped_bars(
            CHARTS_DIR / "execution_times.svg",
            "Tiempo de ejecución (mediana)",
            labels, series, "segundos", log_scale=False,
        )
        print(f"Gráfico: {CHARTS_DIR / 'execution_times.svg'}")

    if comp_rows:
        series = [(c, col_values(comp_rows, c)) for c in comp_cols if c in comp_rows[0]]
        plot_grouped_bars(
            CHARTS_DIR / "compilation_times.svg",
            "Tiempo de compilación (mediana)",
            [r["Benchmark"] for r in comp_rows],
            series, "segundos", log_scale=True,
        )
        print(f"Gráfico: {CHARTS_DIR / 'compilation_times.svg'}")

    if size_rows:
        series = [(c, [to_float(r.get(c)) for r in size_rows]) for c in size_cols]
        plot_grouped_bars(
            CHARTS_DIR / "binary_sizes.svg",
            "Tamaño de binario",
            [r["Benchmark"] for r in size_rows],
            series, "bytes",
        )
        print(f"Gráfico: {CHARTS_DIR / 'binary_sizes.svg'}")

    if frame_rows:
        series = [(c, col_values(frame_rows, c)) for c in (frame_cols or [])]
        plot_grouped_bars(
            CHARTS_DIR / "stack_frame_sizes.svg",
            "Tamaño de stack frame de main() (C--)",
            [r["Benchmark"] for r in frame_rows],
            series, "bytes",
        )
        print(f"Gráfico: {CHARTS_DIR / 'stack_frame_sizes.svg'}")


def print_speedup_table(title, rows, baseline, targets):
    print(f"\n{title}")
    print("-" * 72)
    for target in targets:
        print(f"\n  vs {target} (ratio > 1 significa que {target} es más rápido):")
        for bench, ratio, _, _ in speedup_rows(rows, baseline, target):
            print(f"    {bench:<18} {ratio:.2f}x")


def main():
    print("=" * 72)
    print("ANÁLISIS DE BENCHMARKS — Compilador C--")
    print("=" * 72)

    env_file = RESULTS_DIR / "environment.txt"
    if env_file.exists():
        print(f"\n--- Entorno ({env_file.name}) ---")
        print(env_file.read_text(encoding="utf-8").strip())

    comp_cols, comp_rows = read_csv("compilation_times.csv")
    exec_cols, exec_rows = read_csv("execution_times.csv")
    size_cols, size_rows = read_csv("binary_sizes.csv")
    frame_cols, frame_rows = read_csv("stack_frame_sizes.csv")

    if comp_cols and comp_rows:
        print_table("Tiempos de compilación", comp_cols, comp_rows, format_time)
    if exec_cols and exec_rows:
        print_table("Tiempos de ejecución", exec_cols, exec_rows, format_time)
    if size_cols and size_rows:
        print_table("Tamaños de binario", size_cols, size_rows, format_size)
    if frame_cols and frame_rows:
        print_table("Tamaño de stack frame de main() (C--)", frame_cols, frame_rows, format_size)

    if exec_cols and exec_rows and "C--_Compiler" in exec_cols:
        print_speedup_table(
            "Speedup de ejecución",
            exec_rows,
            "C--_Compiler",
            [c for c in ["GCC_O0", "GCC_O2", "Clang_O0", "Clang_O2"] if c in exec_cols],
        )

    generate_charts(exec_cols or [], exec_rows or [], size_cols or [], size_rows or [],
                    comp_cols or [], comp_rows or [], frame_cols or [], frame_rows or [])




if __name__ == "__main__":
    main()
