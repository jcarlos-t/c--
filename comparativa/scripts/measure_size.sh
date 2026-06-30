#!/bin/bash
# Script to measure binary size

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

echo "Binary Size Measurements (bytes)" > "$RESULTS_DIR/binary_sizes.csv"
echo "Benchmark,C--_Compiler,GCC_O0,GCC_O2,Clang_O2" >> "$RESULTS_DIR/binary_sizes.csv"

benchmarks=("bench_fib" "bench_matmul" "bench_float" "bench_struct" "bench_prime" "bench_mixed")

for bench in "${benchmarks[@]}"; do
    echo "Measuring binary size for $bench..."
    
    # C-- compiler
    cnn_file="$BENCHMARKS_CNN/$bench.cnn"
    if [ -f "$cnn_file" ]; then
        s_file="$BUILD_DIR/$bench.s"
        "$COMPILER" "$cnn_file" > "$s_file" 2>/dev/null
        gcc -no-pie "$s_file" -o "$BUILD_DIR/$bench" 2>/dev/null
        if [ -f "$BUILD_DIR/$bench" ]; then
            size_cnn=$(wc -c < "$BUILD_DIR/$bench")
        else
            size_cnn="ERROR"
        fi
    else
        size_cnn="N/A"
    fi
    
    # GCC -O0
    c_file="$BENCHMARKS_C/$bench.c"
    if [ -f "$c_file" ]; then
        gcc -O0 "$c_file" -o "$BUILD_DIR/${bench}_gcc_o0" 2>/dev/null
        if [ -f "$BUILD_DIR/${bench}_gcc_o0" ]; then
            size_gcc_o0=$(wc -c < "$BUILD_DIR/${bench}_gcc_o0")
        else
            size_gcc_o0="ERROR"
        fi
    else
        size_gcc_o0="N/A"
    fi
    
    # GCC -O2
    if [ -f "$c_file" ]; then
        gcc -O2 "$c_file" -o "$BUILD_DIR/${bench}_gcc_o2" 2>/dev/null
        if [ -f "$BUILD_DIR/${bench}_gcc_o2" ]; then
            size_gcc_o2=$(wc -c < "$BUILD_DIR/${bench}_gcc_o2")
        else
            size_gcc_o2="ERROR"
        fi
    else
        size_gcc_o2="N/A"
    fi
    
    # Clang -O2
    if [ -f "$c_file" ]; then
        clang -O2 "$c_file" -o "$BUILD_DIR/${bench}_clang_o2" 2>/dev/null
        if [ -f "$BUILD_DIR/${bench}_clang_o2" ]; then
            size_clang_o2=$(wc -c < "$BUILD_DIR/${bench}_clang_o2")
        else
            size_clang_o2="ERROR"
        fi
    else
        size_clang_o2="N/A"
    fi
    
    echo "$bench,$size_cnn,$size_gcc_o0,$size_gcc_o2,$size_clang_o2" >> "$RESULTS_DIR/binary_sizes.csv"
done

echo "Binary size measurements saved to $RESULTS_DIR/binary_sizes.csv"
