#!/bin/bash
# Master script to run all benchmark measurements

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================"
echo "Running All Benchmark Measurements"
echo "========================================"
echo ""

echo "1. Measuring compilation times..."
"$SCRIPT_DIR/measure_compilation.sh"

echo ""
echo "2. Measuring execution times..."
"$SCRIPT_DIR/measure_execution.sh"

echo ""
echo "3. Measuring binary sizes..."
"$SCRIPT_DIR/measure_size.sh"
echo ""

echo "========================================"
echo "All measurements completed!"
echo "Results saved in comparativa/results/"
echo "========================================"
