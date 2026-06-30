#!/usr/bin/env python3
"""
Script to analyze benchmark results and generate comparison tables
"""

import csv
import os
from pathlib import Path

RESULTS_DIR = Path(__file__).parent.parent / "results"

def read_csv(filename):
    """Read CSV file and return data"""
    filepath = RESULTS_DIR / filename
    if not filepath.exists():
        print(f"Warning: {filename} not found")
        return None
    
    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        return list(reader)

def format_time(value):
    """Format time value for display"""
    try:
        val = float(value)
        if val < 1:
            return f"{val*1000:.1f}ms"
        else:
            return f"{val:.3f}s"
    except:
        return value

def format_size(value):
    """Format size value for display"""
    try:
        val = int(value)
        if val >= 1024:
            return f"{val/1024:.1f}KB"
        return f"{val}B"
    except:
        return value

def print_table(title, data, value_formatter):
    """Print a formatted table"""
    print(f"\n{title}")
    print("=" * 80)
    
    if not data:
        print("No data available")
        return
    
    # Print header
    headers = list(data[0].keys())
    print(f"{headers[0]:<20} {headers[1]:<15} {headers[2]:<15} {headers[3]:<15} {headers[4]:<15}")
    print("-" * 80)
    
    # Print rows
    for row in data:
        benchmark = row[headers[0]] if row[headers[0]] else "N/A"
        values = []
        for h in headers[1:]:
            val = row[h] if row[h] else "N/A"
            values.append(value_formatter(val))
        print(f"{benchmark:<20} {values[0]:<15} {values[1]:<15} {values[2]:<15} {values[3]:<15}")

def calculate_speedup(data, baseline_col, compare_col):
    """Calculate speedup ratios"""
    results = []
    for row in data:
        benchmark = row['Benchmark']
        try:
            baseline = float(row[baseline_col])
            compare = float(row[compare_col])
            if baseline > 0 and compare > 0:
                speedup = baseline / compare
                results.append((benchmark, speedup))
        except:
            pass
    return results

def main():
    print("=" * 80)
    print("BENCHMARK RESULTS ANALYSIS")
    print("=" * 80)
    
    # Read all result files
    compilation_data = read_csv("compilation_times.csv")
    execution_data = read_csv("execution_times.csv")
    size_data = read_csv("binary_sizes.csv")
    
    # Print tables
    if compilation_data:
        print_table("Compilation Time Comparison", compilation_data, format_time)
    
    if execution_data:
        print_table("Execution Time Comparison", execution_data, format_time)
    
    if size_data:
        print_table("Binary Size Comparison", size_data, format_size)
    
    # Analysis
    print("\n" + "=" * 80)
    print("TECHNICAL ANALYSIS")
    print("=" * 80)
    
    if execution_data:
        print("\nSpeedup Analysis (C-- Compiler vs GCC -O2):")
        print("-" * 80)
        speedups = calculate_speedup(execution_data, 'C--_Compiler', 'GCC_O2')
        for bench, speedup in speedups:
            print(f"{bench:<20} GCC-O2 is {speedup:.2f}x faster")
    
    print("\n" + "=" * 80)
    print("KEY OBSERVATIONS")
    print("=" * 80)
    
    print("""
1. Compilation Time:
   - C-- compiler should compile significantly faster than GCC/Clang
   - Reason: Simple pipeline (scanner + parser + typecheck + codegen)
   - GCC/Clang have hundreds of optimization passes

2. Execution Time:
   - GCC -O2 and Clang -O2 should be significantly faster
   - Reasons:
     * Register allocation: GCC keeps variables in registers, C-- spills to stack
     * Loop unrolling: GCC unrolls loops for better performance
     * Vectorization: GCC uses SSE/AVX for parallel operations
     * Instruction selection: GCC chooses optimal instructions
   - C-- compiler focuses on correctness over optimization

3. Binary Size:
   - GCC -O2 often produces smaller code due to optimizations
   - C-- may produce larger binaries due to lack of optimization

4. Where C-- Excels:
   - Fast compilation (simple, focused pipeline)
   - Clear, understandable generated code
   - Good for educational purposes and understanding compilation

5. Optimization Opportunities for C--:
   - Register allocation (reduce stack spills)
   - Constant propagation
   - Dead code elimination
   - Basic loop optimizations
   - Better instruction selection

6. Implemented Optimizations:
   - Constant folding (already implemented)
   - Future: peephole optimization, register allocation
""")

if __name__ == "__main__":
    main()
