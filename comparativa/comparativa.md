# Comparativa de rendimiento — Compilador C--

Comparación del compilador **C--** con **GCC** y **Clang** en tiempo de compilación, tiempo de ejecución y tamaño de binario.

## Metodología

### Benchmarks

| Nombre | Descripción |
|--------|------------|
| bench_fib | Fibonacci recursivo (n=35) — recursión y llamadas a función |
| bench_matmul | Matmul 80×80 — arreglos 2D y loops anidados |
| bench_float | Suma de cuadrados float (n=500k) — aritmética float |
| bench_struct | Struct + puntero en loop (n=500k) — structs, punteros y `->` |
| bench_prime | Criba de primos hasta 40k — loops, módulo, condicionales |
| bench_mixed | Struct con int y float (n=150k) — tipos combinados |

Cada benchmark existe en par equivalente: `benchmarks_cnn/*.cnn` y `benchmarks_c/*.c`.

### Métricas

| Métrica | Descripción |
|---------|-------------|
| Compilación (codegen) | `c-- -c` — solo frontend + typecheck + codegen |
| Compilación (full) | `c-- --exec` — pipeline completo hasta ejecutable |
| Compilación GCC/Clang | `gcc` / `clang` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |

### Limitaciones

1. C-- usa `printf(int)` y `printf(double)`, no `printf("%ld\n", ...)` — en benchmarks CPU-bound el impacto es bajo.
2. C-- aplica constant folding; GCC/Clang -O2 aplican decenas de passes de optimización.
3. Mediciones en una sola máquina; no se normaliza por frecuencia de CPU.

### Entorno

```
Fecha: 2026-07-01
GCC: 16.1.1
Clang: 22.1.6
Python: 3.14.6
Repeticiones: 7 por medición
Timeoutejec: 120 s
```

## Resultados

### Tiempos de compilación

| Benchmark | C-- codegen | C-- full | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|------------:|---------:|--------:|--------:|----------:|----------:|
| bench_fib | 1.3 ms | 17.3 ms | 23.9 ms | 49.7 ms | 38.2 ms | 37.5 ms |
| bench_matmul | 1.3 ms | 18.9 ms | 26.2 ms | 37.9 ms | 39.0 ms | 46.8 ms |
| bench_float | 1.4 ms | 15.8 ms | 27.5 ms | 29.3 ms | 35.9 ms | 41.1 ms |
| bench_struct | 1.3 ms | 18.9 ms | 24.8 ms | 29.1 ms | 39.1 ms | 36.4 ms |
| bench_prime | 1.4 ms | 16.6 ms | 24.5 ms | 29.8 ms | 35.8 ms | 38.6 ms |
| bench_mixed | 1.3 ms | 16.9 ms | 24.9 ms | 30.4 ms | 35.5 ms | 36.8 ms |

![Tiempos de compilación](results/charts/compilation_times.svg)

### Tiempos de ejecución

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 61.7 ms | 51.6 ms | 13.1 ms | 43.7 ms | 25.7 ms |
| bench_matmul | 3.2 ms | 2.4 ms | 0.8 ms | 1.8 ms | 0.9 ms |
| bench_float | 9.0 ms | 2.3 ms | 1.0 ms | 4.4 ms | 1.1 ms |
| bench_struct | 2.3 ms | 1.9 ms | 0.9 ms | 1.7 ms | 0.5 ms |
| bench_prime | 3.7 ms | 2.6 ms | 2.5 ms | 2.8 ms | 2.5 ms |
| bench_mixed | 2.4 ms | 1.4 ms | 0.7 ms | 1.6 ms | 0.5 ms |

![Tiempos de ejecución](results/charts/execution_times.svg)

### Tamaños de binario

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 16.0 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_matmul | 16.1 KB | 15.7 KB | 15.7 KB | 15.7 KB | 15.7 KB |
| bench_float | 15.9 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_struct | 15.9 KB | 15.7 KB | 15.6 KB | 15.7 KB | 15.6 KB |
| bench_prime | 16.1 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_mixed | 15.9 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |

![Tamaños de binario](results/charts/binary_sizes.svg)

### Speedups de ejecución (vs C--)

| Benchmark | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|--------:|--------:|----------:|----------:|
| bench_fib | 1.20× | 4.70× | 1.41× | 2.40× |
| bench_matmul | 1.35× | 3.93× | 1.78× | 3.64× |
| bench_float | 3.88× | 9.03× | 2.06× | 8.31× |
| bench_struct | 1.20× | 2.61× | 1.34× | 4.39× |
| bench_prime | 1.40× | 1.47× | 1.32× | 1.45× |
| bench_mixed | 1.74× | 3.57× | 1.48× | 4.59× |

> Ratio > 1 significa que el compilador es más rápido que C--.

## Análisis

### Compilación

C-- codegen es **~20–35× más rápido** que GCC/Clang -O2. Esto es esperable: el frontend de C-- es un parser simple sin optimizaciones multi-pase. El pipeline completo (`--exec`) añade el tiempo de `gcc` para ensamblar y linkear, acercándose a los tiempos de GCC -O0.

### Ejecución

La mediana de speedup de GCC -O2 sobre C-- es **3.75×**. Las brechas más grandes se dan en:

- **bench_float (9.03×)**: GCC vectoriza con SSE/AVX; C-- usa SSE escalar.
- **bench_fib (4.70×)**: GCC optimiza la recursión con reuso de registros; C-- spillea todo al stack.
- **bench_matmul (3.93×)**: GCC desenrolla loops y vectoriza; C-- accede linealmente.

En **bench_prime (1.47×)** la brecha es menor porque el cuello de botella es la aritmética entera, donde C-- genera código directo sin sobrecarga de llamadas a función.

### Tamaño de binario

Los binarios de C-- son consistentemente **~400-500 bytes más grandes** que los de GCC. Esto se debe a que C-- incluye código de soporte (potencia, formato `printf`) que GCC maneja vía libc, y a que C-- no elimina código muerto.

### Fortalezas y debilidades

**Fortalezas de C--:**
- Compilación extremadamente rápida (~1.3 ms codegen)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes en tiempo de compilación

**Debilidades frente a GCC/Clang -O2:**
- Sin asignación de registros (todo spillea al stack)
- Sin vectorización SIMD
- Sin desenrollado de loops
- Sin eliminación de código muerto
- Sin inlining de funciones

## Cómo reproducir

```bash
# 1. Compilar el compilador (desde la raíz del repo)
make -C /home/tkaos/UTEC/C5/compi/c-- all

# 2. Ejecutar mediciones
cd comparativa/scripts
chmod +x run_all.sh
./run_all.sh
```

O por pasos:

```bash
python3 run_benchmarks.py
python3 analyze_results.py
```
