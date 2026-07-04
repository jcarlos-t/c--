# Comparativa de rendimiento — Compilador C--

Comparación del compilador **C--** con **GCC** y **Clang** en tiempo de
compilación, tiempo de ejecución y tamaño de binario.

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
| bench_constfold | Expresión literal reevaluada 12M veces en un loop — mide **constant folding** (2.1) |
| bench_conststore | 6 variables reseteadas a literales 15M veces en un loop — mide la **mirilla de stores de constantes** (2.1) |
| bench_stackframe | 12 `int` + 12 `char` locales sumados en un loop de 1.5M — mide **bin packing** de offsets (2.1) |

Cada benchmark existe en par equivalente: `benchmarks_cnn/*.cnn` y `benchmarks_c/*.c`.
Los últimos tres (`bench_constfold`, `bench_conststore`, `bench_stackframe`)
son microbenchmarks dirigidos: cada uno satura el patrón de código que
explota una de las optimizaciones documentadas en la sección 2.1 del
informe, en vez de representar una carga de trabajo general.

### Métricas

| Métrica | Descripción |
|---------|-------------|
| Compilación (codegen) | `c-- -c` — solo frontend + typecheck + codegen |
| Compilación (full) | `c-- --exec` — pipeline completo hasta ejecutable |
| Compilación GCC | `gcc` con `-O0` y `-O2` hasta binario |
| Compilación Clang | `clang` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |
| Stack frame de `main` | Bytes reservados por `subq $N, %rsp` en el prólogo (`./c-- -c`), solo para `C--` — refleja directamente el resultado del bin packing de offsets (2.1); no es comparable 1:1 con GCC/Clang, que gestionan su propio layout de stack de forma distinta |

### Limitaciones

1. `C--` usa `printf("%d\n", ...)` / `printf("%f\n", ...)` — soporte nativo de
   formato desde el fix de printf (jul-2026); antes de ese fix los benchmarks
   usaban `printf(valor)` sin formato, sintaxis ya no aceptada por el parser.
2. `C--` aplica constant folding; GCC -O2 y Clang -O2 aplican decenas de
   passes de optimización (vectorización, inlining, desenrollado de loops,
   eliminación de código muerto, etc.).
3. Mediciones realizadas en un entorno Linux nativo. Las tres rutas de
   compilación (`C--`, GCC, Clang) comparten el mismo costo de *spawn* de
   proceso del sistema operativo, por lo que las diferencias de tiempo de
   compilación reflejan directamente el trabajo de cada compilador.
4. Esta corrida incluye **Clang 22.1.6** además de GCC 16.1.1, por lo que la
   comparación es a tres vías (`C--` / GCC / Clang).
5. Mediciones en una sola máquina; no se normaliza por frecuencia de CPU.

### Entorno

```
Fecha: 2026-07-03 14:43 (-05:00)
GCC: 16.1.1 20260625
Clang: 22.1.6
Python: 3.14.6
Repeticiones: 7 por medición
Timeout ejec: 120 s
```

## Resultados

### Tiempos de compilación

| Benchmark | C-- codegen | C-- full | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|------------:|---------:|--------:|--------:|----------:|----------:|
| bench_fib | 4.3 ms | 45.2 ms | 50.9 ms | 95.3 ms | 100.3 ms | 125.4 ms |
| bench_matmul | 2.8 ms | 29.6 ms | 51.9 ms | 72.3 ms | 74.8 ms | 121.4 ms |
| bench_float | 2.6 ms | 29.4 ms | 51.6 ms | 56.0 ms | 69.8 ms | 73.4 ms |
| bench_struct | 2.7 ms | 29.2 ms | 67.0 ms | 72.3 ms | 69.2 ms | 89.7 ms |
| bench_prime | 3.2 ms | 28.9 ms | 50.8 ms | 65.4 ms | 75.2 ms | 87.3 ms |
| bench_mixed | 3.1 ms | 32.0 ms | 50.0 ms | 56.1 ms | 69.9 ms | 72.1 ms |

![Tiempos de compilación](results/charts/compilation_times.svg)

### Tiempos de ejecución

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 129.5 ms | 100.6 ms | 24.8 ms | 90.0 ms | 50.2 ms |
| bench_matmul | 7.2 ms | 3.7 ms | 1.2 ms | 3.1 ms | 1.4 ms |
| bench_float | 14.0 ms | 4.5 ms | 1.9 ms | 7.7 ms | 1.7 ms |
| bench_struct | 6.3 ms | 3.7 ms | 1.7 ms | 3.1 ms | 1.0 ms |
| bench_prime | 6.9 ms | 5.0 ms | 6.2 ms | 6.5 ms | 5.3 ms |
| bench_mixed | 4.4 ms | 2.5 ms | 1.1 ms | 2.9 ms | 1.0 ms |

![Tiempos de ejecución](results/charts/execution_times.svg)

### Tamaños de binario

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_matmul | 15.9 KB | 15.7 KB | 15.7 KB | 15.7 KB | 15.7 KB |
| bench_float | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_struct | 15.7 KB | 15.7 KB | 15.6 KB | 15.7 KB | 15.6 KB |
| bench_prime | 15.9 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_mixed | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |

![Tamaños de binario](results/charts/binary_sizes.svg)

### Speedups de ejecución (vs C--)

| Benchmark | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|--------:|--------:|----------:|----------:|
| bench_fib | 1.29× | 5.21× | 1.44× | 2.58× |
| bench_matmul | 1.96× | 5.86× | 2.30× | 5.23× |
| bench_float | 3.11× | 7.54× | 1.81× | 8.36× |
| bench_struct | 1.68× | 3.62× | 2.00× | 6.30× |
| bench_prime | 1.39× | 1.12× | 1.07× | 1.30× |
| bench_mixed | 1.72× | 3.87× | 1.50× | 4.23× |

> Ratio > 1 significa que GCC/Clang es más rápido que C--; en esta corrida
> ningún benchmark quedó por debajo de 1× (a diferencia de una corrida
> anterior, ver nota sobre ruido en Análisis).

## Análisis

### Compilación

En modo `c-- -c` (solo parseo + typecheck + codegen, sin tocar disco salvo
para leer el fuente), C-- toma consistentemente ~2.6–4.3 ms, ~15–30× menos
que el propio pipeline completo de C--. El pipeline completo (`--exec`)
delega en `gcc` para ensamblar/enlazar, y en esta medición resultó más
rápido (~28.9–45.2 ms) que GCC -O0/-O2 (~50.0–95.3 ms) y que Clang -O0/-O2
(~69.2–125.4 ms) en los seis benchmarks — una ventaja real del pipeline
completo de `C--` en este entorno.

### Ejecución

La mediana de speedup de GCC -O2 sobre C-- es **~4.54×**; la de Clang -O2 es
**~4.73×**. Las brechas más grandes se dan en:

- **bench_float**: GCC (7.54×) y sobre todo Clang (8.36×) vectorizan con
  SSE/AVX; C-- usa SSE escalar.
- **bench_struct (6.30× Clang)** y **bench_matmul (5.86× GCC, 5.23× Clang)**:
  GCC/Clang desenrollan loops, vectorizan y reutilizan registros; C-- spillea
  todo al stack y accede linealmente.
- **bench_fib (5.21× GCC)** y **bench_mixed (3.87–4.23×)**: mezcla de
  aritmética y recursión/tipos donde GCC/Clang reutilizan registros.

En **bench_prime** la brecha es la más chica de la tabla y con una anomalía:
GCC -O2 (6.2 ms) midió **más lento** que GCC -O0 (5.0 ms) — ratio de solo
1.12× frente a un 1.39× de GCC -O0. Con cargas de ~5–6 ms y 7 repeticiones,
el ruido de scheduling pesa más que el efecto real de `-O2` sobre este
patrón de loops/módulo; no se debe sobre-interpretar ese resultado puntual.

### Tamaño de binario

Los binarios de C-- son comparables a los de GCC y Clang (diferencia de
apenas un par de cientos de bytes en esta corrida, dentro del margen de las
secciones fijas de runtime/`printf`). No hay una penalización sistemática
relevante.

### Fortalezas y debilidades

**Fortalezas de C--:**
- Compilación (parseo + typecheck + codegen) extremadamente rápida (~2.6–4.3 ms)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes en tiempo de compilación

**Debilidades frente a GCC/Clang -O2:**
- Sin asignación de registros (todo spillea al stack)
- Sin vectorización SIMD
- Sin desenrollado de loops
- Sin eliminación de código muerto
- Sin inlining de funciones

> **Nota sobre los datos de esta página.** Las tablas y gráficas de la
> sección [Resultados](#resultados) de este archivo (y las de la sección
> "2. Análisis" del README) corresponden a la corrida con
> `bench_fib`...`bench_mixed` (los 6 benchmarks generales) — es la
> última corrida "final" registrada. Los tres microbenchmarks nuevos
> (`bench_constfold`, `bench_conststore`, `bench_stackframe`) y la
> métrica de stack frame ya están integrados en `run_benchmarks.py` /
> `analyze_results.py`, pero **falta volver a ejecutar la corrida** en el
> entorno de referencia (GCC 16.1.1, Clang 22.1.6) para que
> `comparativa/results/*.csv` y las gráficas reflejen los 9 benchmarks.
> Hasta entonces, corran los pasos de abajo para generar y revisar los
> nuevos datos localmente.

## Cómo reproducir

```bash
# 1. Compilar el compilador (desde la raíz del repo)
make

# 2. Ejecutar mediciones (requiere matplotlib: pip install --user matplotlib)
cd comparativa/scripts
python3 run_benchmarks.py
python3 analyze_results.py
```

`run_all.sh` hace ambos pasos en uno, pero el archivo trae terminadores de
línea CRLF en este checkout — si falla con `required file not found` o
`$'\r': command not found`, ejecutar los dos scripts de Python por separado
como arriba, o convertir el script con `dos2unix run_all.sh` primero.
