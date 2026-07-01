# Comparativa de Rendimiento del Compilador C--

Este directorio contiene los benchmarks y scripts para comparar el rendimiento del compilador C-- contra GCC y Clang.

## Estructura

```
comparativa/
├── benchmarks_cnn/      # Programas benchmark en C--
├── benchmarks_c/        # Programas equivalentes en C
├── scripts/             # Scripts de medición
│   ├── measure_compilation.sh   # Tiempo de compilación
│   ├── measure_execution.sh      # Tiempo de ejecución
│   ├── measure_size.sh           # Tamaño de binarios
│   ├── run_all.sh                # Ejecutar todas las mediciones
│   └── analyze_results.py        # Análisis de resultados
├── results/             # Resultados de las mediciones (CSV)
└── README.md            # Este archivo
```

## Benchmarks

| Benchmark | Descripción | Características ejercitadas |
|-----------|-------------|----------------------------|
| bench_fib | Fibonacci recursivo | Llamadas recursivas, control de flujo |
| bench_matmul | Multiplicación de matrices | Arreglos multidimensionales, loops anidados |
| bench_float | Aritmética float | Operaciones float/double intensivas |
| bench_struct | Structs y punteros | Acceso a structs, operaciones con punteros |
| bench_prime | Números primos | Loops, módulo, condicionales |
| bench_mixed | Operaciones mixtas | Combinación de todas las características |

## Uso

### 1. Compilar el compilador C--

```bash
cd /home/tkaos/UTEC/C5/compi/c--
make
```

### 2. Ejecutar todas las mediciones

```bash
cd comparativa/scripts
./run_all.sh
```

Esto ejecutará:
- Medición de tiempos de compilación
- Medición de tiempos de ejecución
- Medición de tamaños de binario

### 3. Analizar los resultados

```bash
cd comparativa/scripts
python3 analyze_results.py
```

Esto generará una tabla comparativa y análisis técnico.

### 4. Ejecutar mediciones individualmente

```bash
# Solo tiempo de compilación
./measure_compilation.sh

# Solo tiempo de ejecución
./measure_execution.sh

# Solo tamaño de binario
./measure_size.sh
```

## Resultados

Los resultados se guardan en `comparativa/results/`:

- `compilation_times.csv` - Tiempos de compilación en segundos
- `execution_times.csv` - Tiempos de ejecución en segundos
- `binary_sizes.csv` - Tamaños de binario en bytes

## Métricas

### Tiempo de Compilación
Mide la madurez del pipeline del compilador:
- C--: scanner + parser + typecheck + constant folding + codegen
- GCC/Clang: cientos de passes de optimización

### Tiempo de Ejecución
Mide la calidad del codegen:
- Asignación de registros
- Selección de instrucciones
- Optimizaciones de loops
- Vectorización

### Tamaño de Binario
Código más pequeño suele implicar mejor aprovechamiento de caché.

## Análisis Esperado

### Por qué GCC -O2 es más rápido:
- Registro de variables en registros (C-- spillea todo al stack)
- Alineamiento de memoria
- Desenrollado de loops (loop unrolling)
- Reasociación algebraica
- Vectorización SSE/AVX
- Eliminación de código muerto

### Optimizaciones implementadas en C--:
- Constant folding (evaluación de expresiones constantes en tiempo de compilación)

### Fortalezas de C--:
- Compilación instantánea (parser simple)
- Código generado claro y entendible
- Excelente para propósitos educativos

### Brechas más grandes:
- Arreglos multidimensionales (GCC vectoriza, C-- acceso lineal)
- Float (GCC usa SSE/AVX, C-- usa SSE escalar)
- Registro allocation (GCC optimiza, C-- usa stack)

## Requisitos

- Compilador C-- construido
- GCC instalado
- Clang instalado (opcional)
- Python 3 (para análisis)
- bc (para cálculos en scripts bash)

## Notas

Los benchmarks están diseñados para ejecutarse el tiempo suficiente para ser medibles (n=1000000 iteraciones o similar).
