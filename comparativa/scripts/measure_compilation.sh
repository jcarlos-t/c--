#!/bin/bash
# Script to measure compilation time for C-- compiler vs GCC/Clang

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BENCHMARKS_CNN="$PROJECT_DIR/benchmarks_cnn"
BENCHMARKS_C="$PROJECT_DIR/benchmarks_c"
RESULTS_DIR="$PROJECT_DIR/results"
COMPILER="$PROJECT_DIR/../main"

# Create results directory
mkdir -p "$RESULTS_DIR"

# Build the C-- compiler first
echo "Building C-- compiler..."
cd "$PROJECT_DIR/.."
g++ -o main -std=c++17 main.cpp scanner.cpp token.cpp parser.cpp ast.cpp visitor.cpp TypeChecker.cpp ConstantFolding.cpp Gencode.cpp
if [ $? -ne 0 ]; then
    echo "Error: Failed to build C-- compiler"
    exit 1
fi
cd "$SCRIPT_DIR"

echo "Compilation Time Measurements" > "$RESULTS_DIR/compilation_times.csv"
echo "Benchmark,C--_Compiler,GCC_O0,GCC_O2,Clang_O2" >> "$RESULTS_DIR/compilation_times.csv"

benchmarks=("bench_fib" "bench_matmul" "bench_float" "bench_struct" "bench_prime" "bench_mixed")

for bench in "${benchmarks[@]}"; do
    echo "Measuring compilation time for $bench..."
    
    # C-- compiler
    cnn_file="$BENCHMARKS_CNN/$bench.cnn"
    if [ -f "$cnn_file" ]; then
        time_cnn=$( (time "$COMPILER" "$cnn_file" > /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
        # Convert time to seconds (remove 'm' and 's')
        time_cnn=$(echo "$time_cnn" | awk -F'[ms]' '{print $1*60 + $2}')
    else
        time_cnn="N/A"
    fi
    
    # GCC -O0
    c_file="$BENCHMARKS_C/$bench.c"
    if [ -f "$c_file" ]; then
        time_gcc_o0=$( (time gcc -O0 -S "$c_file" -o /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
        time_gcc_o0=$(echo "$time_gcc_o0" | awk -F'[ms]' '{print $1*60 + $2}')
    else
        time_gcc_o0="N/A"
    fi
    
    # GCC -O2
    if [ -f "$c_file" ]; then
        time_gcc_o2=$( (time gcc -O2 -S "$c_file" -o /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
        time_gcc_o2=$(echo "$time_gcc_o2" | awk -F'[ms]' '{print $1*60 + $2}')
    else
        time_gcc_o2="N/A"
    fi
    
    # Clang -O2
    if [ -f "$c_file" ]; then
        time_clang_o2=$( (time clang -O2 -S "$c_file" -o /dev/null 2>&1) 2>&1 | grep real | awk '{print $2}')
        time_clang_o2=$(echo "$time_clang_o2" | awk -F'[ms]' '{print $1*60 + $2}')
    else
        time_clang_o2="N/A"
    fi
    
    echo "$bench,$time_cnn,$time_gcc_o0,$time_gcc_o2,$time_clang_o2" >> "$RESULTS_DIR/compilation_times.csv"
done

echo "Compilation time measurements saved to $RESULTS_DIR/compilation_times.csv"
