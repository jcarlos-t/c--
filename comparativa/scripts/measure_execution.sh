#!/bin/bash
# Script to measure execution time of generated binaries

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BENCHMARKS_CNN="$PROJECT_DIR/benchmarks_cnn"
BENCHMARKS_C="$PROJECT_DIR/benchmarks_c"
RESULTS_DIR="$PROJECT_DIR/results"
COMPILER="$PROJECT_DIR/../main"
BUILD_DIR="$PROJECT_DIR/build"

# Create directories
mkdir -p "$RESULTS_DIR"
mkdir -p "$BUILD_DIR"

echo "Execution Time Measurements" > "$RESULTS_DIR/execution_times.csv"
echo "Benchmark,C--_Compiler,GCC_O0,GCC_O2,Clang_O2" >> "$RESULTS_DIR/execution_times.csv"

benchmarks=("bench_fib" "bench_matmul" "bench_float" "bench_struct" "bench_prime" "bench_mixed")

for bench in "${benchmarks[@]}"; do
    echo "Measuring execution time for $bench..."
    
    # C-- compiler
    cnn_file="$BENCHMARKS_CNN/$bench.cnn"
    if [ -f "$cnn_file" ]; then
        s_file="$BUILD_DIR/$bench.s"
        "$COMPILER" "$cnn_file" > "$s_file" 2>/dev/null
        gcc -no-pie "$s_file" -o "$BUILD_DIR/$bench" 2>/dev/null
        if [ -f "$BUILD_DIR/$bench" ]; then
            time_cnn=$( (time "$BUILD_DIR/$bench" > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
            time_cnn=$(echo "$time_cnn" | awk -F'[ms]' '{print $1*60 + $2}')
        else
            time_cnn="ERROR"
        fi
    else
        time_cnn="N/A"
    fi
    
    # GCC -O0
    c_file="$BENCHMARKS_C/$bench.c"
    if [ -f "$c_file" ]; then
        gcc -O0 "$c_file" -o "$BUILD_DIR/${bench}_gcc_o0" 2>/dev/null
        if [ -f "$BUILD_DIR/${bench}_gcc_o0" ]; then
            time_gcc_o0=$( (time "$BUILD_DIR/${bench}_gcc_o0" > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
            time_gcc_o0=$(echo "$time_gcc_o0" | awk -F'[ms]' '{print $1*60 + $2}')
        else
            time_gcc_o0="ERROR"
        fi
    else
        time_gcc_o0="N/A"
    fi
    
    # GCC -O2
    if [ -f "$c_file" ]; then
        gcc -O2 "$c_file" -o "$BUILD_DIR/${bench}_gcc_o2" 2>/dev/null
        if [ -f "$BUILD_DIR/${bench}_gcc_o2" ]; then
            time_gcc_o2=$( (time "$BUILD_DIR/${bench}_gcc_o2" > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
            time_gcc_o2=$(echo "$time_gcc_o2" | awk -F'[ms]' '{print $1*60 + $2}')
        else
            time_gcc_o2="ERROR"
        fi
    else
        time_gcc_o2="N/A"
    fi
    
    # Clang -O2
    if [ -f "$c_file" ]; then
        clang -O2 "$c_file" -o "$BUILD_DIR/${bench}_clang_o2" 2>/dev/null
        if [ -f "$BUILD_DIR/${bench}_clang_o2" ]; then
            time_clang_o2=$( (time "$BUILD_DIR/${bench}_clang_o2" > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
            time_clang_o2=$(echo "$time_clang_o2" | awk -F'[ms]' '{print $1*60 + $2}')
        else
            time_clang_o2="ERROR"
        fi
    else
        time_clang_o2="N/A"
    fi
    
    echo "$bench,$time_cnn,$time_gcc_o0,$time_gcc_o2,$time_clang_o2" >> "$RESULTS_DIR/execution_times.csv"
done

echo "Execution time measurements saved to $RESULTS_DIR/execution_times.csv"
