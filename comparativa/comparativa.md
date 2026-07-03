# Comparativa de rendimiento — Compilador C--

Comparación del compilador **C--** con **GCC** en tiempo de compilación,
tiempo de ejecución y tamaño de binario.

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
| Compilación GCC | `gcc` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |

### Limitaciones

1. `C--` usa `printf("%d\n", ...)` / `printf("%f\n", ...)` — soporte nativo de
   formato desde el fix de printf (jul-2026); antes de ese fix los benchmarks
   usaban `printf(valor)` sin formato, sintaxis ya no aceptada por el parser.
2. `C--` aplica constant folding; GCC -O2 aplica decenas de passes de optimización.
3. Mediciones tomadas dentro de un filesystem WSL2 (`\\wsl.localhost\...`); el
   *spawn* de procesos externos (`gcc`, el binario resultante) sufre overhead
   de E/S notablemente mayor que en un Linux nativo. Esto **infla los tiempos
   de compilación de GCC y de `C-- --exec`** (que también invoca `gcc` para
   ensamblar/enlazar) más de lo que infla el modo `c-- -c` (que no toca disco
   más que para leer el fuente). Por eso el ratio codegen-only vs. GCC de esta
   corrida es más extremo que en una máquina Linux nativa — no se debe leer
   como una mejora arquitectónica del compilador, sino como un artefacto del
   entorno de medición.
4. **Clang no está instalado en este entorno** — las columnas Clang de una
   corrida anterior (2026-07-01, máquina distinta) no se reprodujeron aquí.
5. Mediciones en una sola máquina; no se normaliza por frecuencia de CPU.

### Entorno

```
Fecha: 2026-07-03
GCC: 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1)
Clang: no instalado
Python: 3.12.3
Repeticiones: 7 por medición
Timeout ejec: 120 s
```

## Resultados

### Tiempos de compilación

| Benchmark | C-- codegen | C-- full | GCC -O0 | GCC -O2 |
|-----------|------------:|---------:|--------:|--------:|
| bench_fib | 1.7 ms | 478.5 ms | 470.4 ms | 554.1 ms |
| bench_matmul | 1.2 ms | 195.5 ms | 278.5 ms | 220.6 ms |
| bench_float | 1.1 ms | 242.1 ms | 197.6 ms | 220.1 ms |
| bench_struct | 1.1 ms | 180.4 ms | 166.2 ms | 193.3 ms |
| bench_prime | 1.1 ms | 270.2 ms | 165.4 ms | 277.9 ms |
| bench_mixed | 1.1 ms | 286.2 ms | 491.0 ms | 335.7 ms |

![Tiempos de compilación](results/charts/compilation_times.svg)

### Tiempos de ejecución

| Benchmark | C-- | GCC -O0 | GCC -O2 |
|-----------|----:|--------:|--------:|
| bench_fib | 72.4 ms | 57.1 ms | 13.5 ms |
| bench_matmul | 3.3 ms | 1.7 ms | 0.8 ms |
| bench_float | 6.3 ms | 1.5 ms | 0.7 ms |
| bench_struct | 1.9 ms | 2.5 ms | 1.3 ms |
| bench_prime | 2.5 ms | 4.3 ms | 1.6 ms |
| bench_mixed | 3.0 ms | 1.3 ms | 0.5 ms |

![Tiempos de ejecución](results/charts/execution_times.svg)

### Tamaños de binario

| Benchmark | C-- | GCC -O0 | GCC -O2 |
|-----------|----:|--------:|--------:|
| bench_fib | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_matmul | 15.7 KB | 15.6 KB | 15.7 KB |
| bench_float | 15.5 KB | 15.6 KB | 15.6 KB |
| bench_struct | 15.5 KB | 15.6 KB | 15.6 KB |
| bench_prime | 15.7 KB | 15.6 KB | 15.6 KB |
| bench_mixed | 15.5 KB | 15.6 KB | 15.6 KB |

![Tamaños de binario](results/charts/binary_sizes.svg)

### Speedups de ejecución (vs C--)

| Benchmark | GCC -O0 | GCC -O2 |
|-----------|--------:|--------:|
| bench_fib | 1.27× | 5.36× |
| bench_matmul | 2.00× | 4.19× |
| bench_float | 4.22× | 9.26× |
| bench_struct | 0.79× | 1.53× |
| bench_prime | 0.59× | 1.54× |
| bench_mixed | 2.28× | 6.34× |

> Ratio > 1 significa que GCC es más rápido que C--; ratio < 1 significa que
> C-- fue más rápido en esta corrida (ver nota sobre ruido en Análisis).

## Análisis

### Compilación

En modo `c-- -c` (solo parseo + typecheck + codegen, sin tocar disco salvo
para leer el fuente), C-- toma consistentemente ~1.1–1.7 ms. El pipeline
completo (`--exec`) delega en `gcc` para ensamblar/enlazar y por eso escala
con el mismo overhead de proceso externo que sufre GCC en este entorno WSL2
(ver Limitaciones, punto 3) — de ahí que `C-- full` y `GCC -O0/-O2` queden en
el mismo orden de magnitud (~165–555 ms), mientras que el modo codegen puro
se mantiene ~100–450× más rápido. Esa brecha extrema no debe interpretarse
como una ventaja arquitectónica real de C--, sino como el resultado de que
solo el modo codegen evita por completo el costo de I/O de este entorno.

### Ejecución

La mediana de speedup de GCC -O2 sobre C-- es **~4.78×**. Las brechas más
grandes se dan en:

- **bench_float (9.26×)**: GCC vectoriza con SSE/AVX; C-- usa SSE escalar.
- **bench_mixed (6.34×)** y **bench_fib (5.36×)**: mezcla de aritmética y
  recursión donde GCC reutiliza registros; C-- spillea todo al stack.
- **bench_matmul (4.19×)**: GCC desenrolla loops y vectoriza; C-- accede linealmente.

En **bench_struct (1.53×)** y **bench_prime (1.54×)** la brecha es menor
porque el cuello de botella es aritmética entera y control de flujo, donde
C-- genera código directo sin sobrecarga de optimización. Contra GCC -O0,
**bench_struct** y **bench_prime** incluso midieron a C-- más rápido
(0.79× y 0.59×) — con cargas de ~2–4 ms y 7 repeticiones, el ruido de
scheduling en WSL2 pesa más que la diferencia real de codegen; no se debe
sobre-interpretar ese resultado puntual.

### Tamaño de binario

Los binarios de C-- son comparables a los de GCC (diferencia de apenas
decenas de bytes en esta corrida, dentro del margen de las secciones fijas
de runtime/`printf`). No hay una penalización sistemática relevante.

### Fortalezas y debilidades

**Fortalezas de C--:**
- Compilación (parseo + typecheck + codegen) extremadamente rápida (~1.1 ms)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes en tiempo de compilación

**Debilidades frente a GCC -O2:**
- Sin asignación de registros (todo spillea al stack)
- Sin vectorización SIMD
- Sin desenrollado de loops
- Sin eliminación de código muerto
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
