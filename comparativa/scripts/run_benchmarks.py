#!/usr/bin/env python3
"""
Ejecuta benchmarks C-- vs GCC/Clang con mediana de N repeticiones.
Genera CSV en comparativa/results/.
"""

import csv
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
COMPARATIVA_DIR = SCRIPT_DIR.parent
PROJECT_DIR = COMPARATIVA_DIR.parent
BENCHMARKS_CNN = COMPARATIVA_DIR / "benchmarks_cnn"
BENCHMARKS_C = COMPARATIVA_DIR / "benchmarks_c"
RESULTS_DIR = COMPARATIVA_DIR / "results"
BUILD_DIR = COMPARATIVA_DIR / "build"
COMPILER = PROJECT_DIR / "c--"

BENCHMARKS = [
    "bench_fib",
    "bench_matmul",
    "bench_float",
    "bench_struct",
    "bench_prime",
    "bench_mixed",
    # Microbenchmarks dirigidos a las optimizaciones de C-- (ver 2.1 del
    # informe): constant folding y mirilla de stores de constantes.
    "bench_constfold",
    "bench_conststore",
]

RUNS = 7
EXEC_TIMEOUT = 120

COMPILATION_COLUMNS = [
    "C--_Codegen",
    "C--_Full",
    "GCC_O0",
    "GCC_O2",
    "Clang_O0",
    "Clang_O2",
]

EXECUTION_COLUMNS = [
    "C--_Compiler",
    "GCC_O0",
    "GCC_O2",
    "Clang_O0",
    "Clang_O2",
]

SIZE_COLUMNS = EXECUTION_COLUMNS


def run_cmd(cmd, timeout=None, cwd=None):
    try:
        return subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=cwd,
        )
    except subprocess.TimeoutExpired:
        return None
    except FileNotFoundError:
        return None


def median_elapsed(run_fn, runs=RUNS):
    samples = []
    for _ in range(runs):
        t0 = time.perf_counter()
        ok = run_fn()
        elapsed = time.perf_counter() - t0
        if ok:
            samples.append(elapsed)
    if not samples:
        return "ERROR"
    return f"{statistics.median(samples):.6f}"


def ensure_compiler():
    if COMPILER.is_file():
        return True
    print("Compilando c--...")
    r = run_cmd(["make", "-C", str(PROJECT_DIR), "all"])
    return r is not None and r.returncode == 0 and COMPILER.is_file()


def tool_available(name):
    return shutil.which(name) is not None


def write_env_info():
    info_path = RESULTS_DIR / "environment.txt"
    lines = [
        f"date: {subprocess.getoutput('date -Iseconds')}",
        f"runs_per_measurement: {RUNS}",
        f"metric: median",
        f"exec_timeout_sec: {EXEC_TIMEOUT}",
        "",
    ]
    for label, cmd in [
        ("gcc", ["gcc", "--version"]),
        ("clang", ["clang", "--version"]),
        ("python", [sys.executable, "--version"]),
    ]:
        if cmd[0] == "clang" and not tool_available("clang"):
            lines.append("clang: not installed")
            continue
        r = run_cmd(cmd)
        if r and r.returncode == 0:
            lines.append(f"{label}: {r.stdout.splitlines()[0]}")
        else:
            lines.append(f"{label}: unavailable")
    info_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Entorno guardado en {info_path}")


def measure_cnn_codegen(cnn_file):
    def once():
        r = run_cmd([str(COMPILER), str(cnn_file), "-c", "-o", os.devnull])
        return r is not None and r.returncode == 0
    return median_elapsed(once)


def measure_cnn_full(cnn_file, out_bin):
    out_bin.parent.mkdir(parents=True, exist_ok=True)
    if out_bin.exists():
        out_bin.unlink()

    def once():
        if out_bin.exists():
            out_bin.unlink()
        r = run_cmd([str(COMPILER), str(cnn_file), "--exec", "-o", str(out_bin)])
        return r is not None and r.returncode == 0 and out_bin.is_file()
    return median_elapsed(once)


def measure_gcc_compile(c_file, opt, out_bin):
    out_bin.parent.mkdir(parents=True, exist_ok=True)

    def once():
        if out_bin.exists():
            out_bin.unlink()
        r = run_cmd(["gcc", opt, str(c_file), "-o", str(out_bin)])
        return r is not None and r.returncode == 0 and out_bin.is_file()
    return median_elapsed(once)


def measure_clang_compile(c_file, opt, out_bin):
    if not tool_available("clang"):
        return "N/A"
    out_bin.parent.mkdir(parents=True, exist_ok=True)

    def once():
        if out_bin.exists():
            out_bin.unlink()
        r = run_cmd(["clang", opt, str(c_file), "-o", str(out_bin)])
        return r is not None and r.returncode == 0 and out_bin.is_file()
    return median_elapsed(once)


def measure_execution(bin_path):
    if not bin_path.is_file():
        return "ERROR"

    def once():
        r = run_cmd([str(bin_path)], timeout=EXEC_TIMEOUT)
        return r is not None and r.returncode == 0
    return median_elapsed(once)


def file_size(path):
    if path.is_file():
        return str(path.stat().st_size)
    return "ERROR"


def write_csv(filename, columns, rows):
    path = RESULTS_DIR / filename
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Benchmark"] + columns)
        for bench, values in rows:
            w.writerow([bench] + values)
    print(f"Escrito {path}")


def main():
    if not ensure_compiler():
        print("Error: no se pudo construir el compilador c--", file=sys.stderr)
        sys.exit(1)

    if not tool_available("gcc"):
        print("Error: gcc no está instalado", file=sys.stderr)
        sys.exit(1)

    if not tool_available("clang"):
        print("Aviso: clang no encontrado; columnas Clang serán N/A")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    write_env_info()

    comp_rows = []
    exec_rows = []
    size_rows = []

    for bench in BENCHMARKS:
        print(f"\n=== {bench} ===")
        cnn_file = BENCHMARKS_CNN / f"{bench}.cnn"
        c_file = BENCHMARKS_C / f"{bench}.c"
        cnn_bin = BUILD_DIR / f"{bench}_cnn"
        gcc_o0_bin = BUILD_DIR / f"{bench}_gcc_o0"
        gcc_o2_bin = BUILD_DIR / f"{bench}_gcc_o2"
        clang_o0_bin = BUILD_DIR / f"{bench}_clang_o0"
        clang_o2_bin = BUILD_DIR / f"{bench}_clang_o2"

        if not cnn_file.is_file() or not c_file.is_file():
            print(f"  Omitido: faltan fuentes")
            continue

        print("  compilación...")
        t_codegen = measure_cnn_codegen(cnn_file)
        t_full = measure_cnn_full(cnn_file, cnn_bin)
        t_gcc_o0 = measure_gcc_compile(c_file, "-O0", gcc_o0_bin)
        t_gcc_o2 = measure_gcc_compile(c_file, "-O2", gcc_o2_bin)
        t_clang_o0 = measure_clang_compile(c_file, "-O0", clang_o0_bin)
        t_clang_o2 = measure_clang_compile(c_file, "-O2", clang_o2_bin)

        comp_rows.append((bench, [
            t_codegen, t_full, t_gcc_o0, t_gcc_o2, t_clang_o0, t_clang_o2
        ]))

        print("  ejecución...")
        # Asegurar binario C-- (measure_cnn_full ya lo generó)
        if not cnn_bin.is_file():
            r = run_cmd([str(COMPILER), str(cnn_file), "--exec", "-o", str(cnn_bin)])
            if r is None or r.returncode != 0:
                t_cnn_exec = "ERROR"
            else:
                t_cnn_exec = measure_execution(cnn_bin)
        else:
            t_cnn_exec = measure_execution(cnn_bin)

        if not gcc_o0_bin.is_file():
            run_cmd(["gcc", "-O0", str(c_file), "-o", str(gcc_o0_bin)])
        if not gcc_o2_bin.is_file():
            run_cmd(["gcc", "-O2", str(c_file), "-o", str(gcc_o2_bin)])
        if tool_available("clang"):
            if not clang_o0_bin.is_file():
                run_cmd(["clang", "-O0", str(c_file), "-o", str(clang_o0_bin)])
            if not clang_o2_bin.is_file():
                run_cmd(["clang", "-O2", str(c_file), "-o", str(clang_o2_bin)])

        exec_rows.append((bench, [
            t_cnn_exec,
            measure_execution(gcc_o0_bin),
            measure_execution(gcc_o2_bin),
            measure_execution(clang_o0_bin) if tool_available("clang") else "N/A",
            measure_execution(clang_o2_bin) if tool_available("clang") else "N/A",
        ]))

        size_rows.append((bench, [
            file_size(cnn_bin),
            file_size(gcc_o0_bin),
            file_size(gcc_o2_bin),
            file_size(clang_o0_bin) if tool_available("clang") else "N/A",
            file_size(clang_o2_bin) if tool_available("clang") else "N/A",
        ]))

    write_csv("compilation_times.csv", COMPILATION_COLUMNS, comp_rows)
    write_csv("execution_times.csv", EXECUTION_COLUMNS, exec_rows)
    write_csv("binary_sizes.csv", SIZE_COLUMNS, size_rows)

    print("\nBenchmarks completados.")


if __name__ == "__main__":
    main()
