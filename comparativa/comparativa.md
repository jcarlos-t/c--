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

Cada benchmark existe en par equivalente: `benchmarks_cnn/*.cnn` y `benchmarks_c/*.c`.
Los últimos dos (`bench_constfold`, `bench_conststore`) son microbenchmarks
dirigidos: cada uno satura el patrón de código que explota una de las
optimizaciones documentadas en la sección 2.1 del informe, en vez de
representar una carga de trabajo general.

### Métricas

| Métrica | Descripción |
|---------|-------------|
| Compilación (codegen) | `c-- -c` — solo frontend + typecheck + codegen |
| Compilación (full) | `c-- --exec` — pipeline completo hasta ejecutable |
| Compilación GCC | `gcc` con `-O0` y `-O2` hasta binario |
| Compilación Clang | `clang` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |

### Limitaciones

1. `C--` usa `printf("%d\n", ...)` / `printf("%f\n", ...)` — soporte nativo de
   formato desde el fix de printf (jul-2026); antes de ese fix los benchmarks
   usaban `printf(valor)` sin formato, sintaxis ya no aceptada por el parser.
2. `C--` aplica constant folding, mirilla de stores de constantes y bin
   packing de offsets en el stack (ver 2.1 del README); GCC -O2 y Clang -O2
   aplican decenas de passes de optimización (vectorización, inlining,
   desenrollado de loops, eliminación de código muerto, razonamiento sobre
   loops completos, etc.).
3. Mediciones realizadas en un entorno Linux nativo. Las tres rutas de
   compilación (`C--`, GCC, Clang) comparten el mismo costo de *spawn* de
   proceso del sistema operativo, por lo que las diferencias de tiempo de
   compilación reflejan directamente el trabajo de cada compilador.
4. Esta corrida incluye **Clang 22.1.6** además de GCC 16.1.1, por lo que la
   comparación es a tres vías (`C--` / GCC / Clang).
5. Mediciones en una sola máquina; no se normaliza por frecuencia de CPU.

### Entorno

```
Fecha: 2026-07-03 21:50 (-05:00)
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
| bench_fib | 4.6 ms | 49.3 ms | 53.7 ms | 101.3 ms | 74.6 ms | 82.4 ms |
| bench_matmul | 2.9 ms | 32.4 ms | 54.2 ms | 77.2 ms | 77.4 ms | 93.3 ms |
| bench_float | 3.5 ms | 31.5 ms | 55.4 ms | 63.5 ms | 74.3 ms | 82.4 ms |
| bench_struct | 3.2 ms | 31.6 ms | 54.8 ms | 62.8 ms | 74.1 ms | 78.7 ms |
| bench_prime | 3.1 ms | 33.4 ms | 53.7 ms | 65.1 ms | 75.8 ms | 81.6 ms |
| bench_mixed | 3.2 ms | 32.7 ms | 54.3 ms | 60.5 ms | 72.7 ms | 81.9 ms |
| bench_constfold | 3.0 ms | 31.0 ms | 54.0 ms | 58.5 ms | 72.1 ms | 77.2 ms |
| bench_conststore | 3.1 ms | 31.9 ms | 52.5 ms | 58.3 ms | 70.5 ms | 80.6 ms |

![Tiempos de compilación](results/charts/compilation_times.svg)

### Tiempos de ejecución

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 127.8 ms | 107.9 ms | 24.3 ms | 93.0 ms | 48.2 ms |
| bench_matmul | 7.3 ms | 3.9 ms | 1.4 ms | 3.3 ms | 2.0 ms |
| bench_float | 14.3 ms | 4.2 ms | 1.8 ms | 7.7 ms | 1.8 ms |
| bench_struct | 5.4 ms | 3.6 ms | 1.6 ms | 3.3 ms | 1.0 ms |
| bench_prime | 7.4 ms | 4.9 ms | 4.6 ms | 4.7 ms | 4.5 ms |
| bench_mixed | 4.4 ms | 2.7 ms | 1.0 ms | 3.0 ms | 1.0 ms |
| bench_constfold | 45.0 ms | 17.7 ms | 1.0 ms | 17.7 ms | 0.9 ms |
| bench_conststore | 94.3 ms | 50.4 ms | 1.1 ms | 50.7 ms | 1.1 ms |

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
| bench_constfold | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_conststore | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |

![Tamaños de binario](results/charts/binary_sizes.svg)

### Speedups de ejecución (vs C--)

| Benchmark | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|--------:|--------:|----------:|----------:|
| bench_fib | 1.19× | 5.25× | 1.37× | 2.65× |
| bench_matmul | 1.88× | 5.37× | 2.18× | 3.70× |
| bench_float | 3.38× | 8.04× | 1.84× | 7.99× |
| bench_struct | 1.51× | 3.31× | 1.65× | 5.48× |
| bench_prime | 1.51× | 1.61× | 1.59× | 1.67× |
| bench_mixed | 1.64× | 4.41× | 1.48× | 4.44× |
| bench_constfold | 2.54× | 44.42× | 2.54× | 47.77× |
| bench_conststore | 1.87× | 82.11× | 1.86× | 85.22× |

> Ratio > 1 significa que GCC/Clang es más rápido que C--; ningún benchmark
> quedó por debajo de 1×. `bench_constfold` y `bench_conststore` tienen
> ratios muy por encima del resto (44×–85×) — ver la explicación en
> Análisis, no son comparables directamente con los otros seis.

## Análisis

### Compilación

En modo `c-- -c` (solo parseo + typecheck + codegen, sin tocar disco salvo
para leer el fuente), C-- toma consistentemente ~2.9–4.6 ms, ~10–16× menos
que el propio pipeline completo de C--. El pipeline completo (`--exec`)
delega en `gcc` para ensamblar/enlazar, y en esta medición resultó más
rápido (~31.0–49.3 ms) que GCC -O0/-O2 (~52.5–101.3 ms) y que Clang -O0/-O2
(~70.5–93.3 ms) en los ocho benchmarks — una ventaja real del pipeline
completo de `C--` en este entorno.

### Ejecución

La mediana de speedup de GCC -O2 sobre C-- es **~5.31×**; la de Clang -O2 es
**~4.96×**. Las brechas más grandes entre los seis benchmarks generales se
dan en:

- **bench_float**: GCC (8.04×) y sobre todo Clang (7.99×) vectorizan con
  SSE/AVX; C-- usa SSE escalar.
- **bench_struct (5.48× Clang)** y **bench_matmul (5.37× GCC)**:
  GCC/Clang desenrollan loops, vectorizan y reutilizan registros; C-- spillea
  todo al stack y accede linealmente.
- **bench_fib (5.25× GCC)** y **bench_mixed (4.41–4.44×)**: mezcla de
  aritmética y recursión/tipos donde GCC/Clang reutilizan registros.

En **bench_prime** la brecha es la más chica de la tabla (1.51×–1.67×): es
el benchmark con más ramas dependientes de datos (criba de primos con
módulo), donde `-O2` tiene menos margen de vectorización/desenrollado.

**`bench_constfold` (44.42×–47.77×) y `bench_conststore` (82.11×–85.22×)
son un caso aparte**, muy por encima del resto de la tabla. Inspeccionando
el ensamblador de `gcc -O2 -S`: no queda **ningún** salto (`jmp`/`jne`/`jl`)
en el `main` generado — el compilador prueba que el resultado final del
loop es una expresión cerrada en tiempo de compilación (constante de
inducción + suma aritmética de `i` sobre un rango conocido) y lo reemplaza
directo por `movabsq $73359558000000, %rdx` seguido del `printf`, sin
ejecutar ni una iteración. `C--` sí aplica constant folding (2.1) y mirilla
de stores (2.1) — por eso sigue siendo más rápido que sus propias
compilaciones sin esas optimizaciones — pero ninguna de las dos analiza el
loop como un todo: cada iteración de `C--` sigue ejecutándose en runtime.
Es decir, estos dos microbenchmarks miden exactamente el techo que las
optimizaciones actuales de `C--` no cruzan: folding *local* de una
subexpresión constante dentro de una iteración, vs. razonamiento sobre la
recurrencia *completa* del loop. Los ratios de estos dos no son comparables
1:1 con los otros seis — no reflejan "GCC es 80× más rápido ejecutando
código equivalente", sino "GCC no ejecuta el loop en absoluto".

### Tamaño de binario

Los binarios de C-- son comparables a los de GCC y Clang (diferencia de
apenas un par de cientos de bytes en esta corrida, dentro del margen de las
secciones fijas de runtime/`printf`). No hay una penalización sistemática
relevante.

### Fortalezas y debilidades

**Fortalezas de C--:**
- Compilación (parseo + typecheck + codegen) extremadamente rápida (~2.9–4.6 ms)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes en tiempo de compilación
- Mirilla de stores de constantes evita el paso intermedio por registro

**Debilidades frente a GCC/Clang -O2:**
- Sin asignación de registros (todo spillea al stack)
- Sin vectorización SIMD
- Sin desenrollado de loops
- Sin razonamiento sobre la recurrencia completa de un loop (no reemplaza
  un loop entero por su resultado cerrado, aunque sea calculable)
- Sin inlining de funciones

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
