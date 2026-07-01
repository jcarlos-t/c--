#!/bin/bash
# Ejecuta mediciones y análisis de la comparativa

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================"
echo "Comparativa C-- vs GCC/Clang"
echo "========================================"

python3 "$SCRIPT_DIR/run_benchmarks.py"
python3 "$SCRIPT_DIR/analyze_results.py"

echo ""
echo "Listo. Resultados en comparativa/results/"
echo "Gráficos SVG en comparativa/results/charts/"
