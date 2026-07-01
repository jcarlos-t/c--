#!/bin/bash
# Deprecated: usar run_benchmarks.py o run_all.sh
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Nota: este script está obsoleto. Ejecutando run_benchmarks.py..."
exec python3 "$SCRIPT_DIR/run_benchmarks.py"
