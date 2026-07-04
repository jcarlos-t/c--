# c--

Implementación de compilador para un subset de C.

## Requisitos

- g++ con soporte para C++17
- gcc (para generar ejecutables con `--exec`)
- make

## Compilar

```bash
make
```

Genera el ejecutable `c--`. Los archivos `.o` intermedios se eliminan automáticamente.

## Uso

```
./c-- <archivo.c--> [opciones]
```

### Flags

| Corta | Larga         | Descripción                                                  |
|-------|---------------|--------------------------------------------------------------|
| `-t`  | `--tokens`    | Imprime los tokens y termina                                 |
| `-a`  | `--ast`       | Imprime el AST y termina                                     |
| `-c`  | `--codegen`   | Imprime el assembly x86-64 y termina                         |
| `-A`  | `--all`       | Imprime tokens, AST y assembly                               |
| `-e`  | `--exec`      | Genera ejecutable a partir del assembly                      |
|       | `-o <archivo>`| Guarda texto de salida (sin `--exec`); nombre del binario (con `--exec`) |

### Comportamiento de `-o`

- **Sin `--exec`**: guarda la salida del modo en el archivo (tokens, AST o assembly).
- **Con `--exec`**: define el nombre del ejecutable generado; la salida de texto va a stdout.
- **Sin `-o`**: la salida de texto va a stdout.

### Ejemplos

```bash
./c-- programa.c--                # Assembly por stdout
./c-- programa.c-- -o salida.s    # Assembly a salida.s
./c-- programa.c-- -c             # Solo assembly por stdout
./c-- programa.c-- -c -o salida.s # Solo assembly a salida.s
./c-- programa.c-- -t             # Solo tokens por stdout
./c-- programa.c-- -t -o salida.txt # Solo tokens a salida.txt
./c-- programa.c-- -a             # Solo AST por stdout
./c-- programa.c-- -A             # Tokens + AST + assembly por stdout

./c-- programa.c-- --exec                  # Assembly por stdout + binario programa.out
./c-- programa.c-- --exec -o ejecutable    # Assembly por stdout + binario ejecutable
./c-- programa.c-- -A --exec               # Tokens + AST + assembly + binario programa.out
./c-- programa.c-- -A --exec -o ejecutable # Todo por stdout + binario ejecutable
```

## Pipeline

1. **Scanner** → tokeniza el fuente
2. **Parser** → construye el AST
3. **TypeChecker** → verifica tipos y resuelve tipos de expresiones
4. **ConstantFolding** → evalua expresiones constantes en tiempo de compilación
5. **GenCodeVisitor** → genera assembly x86-64
6. **gcc** (con `--exec`) → ensambla y linkea a ejecutable

---

# Informe del Proyecto

**Curso:** Compiladores
**Proyecto:** Diseño e implementación del compilador `C--`
**Universidad:** UTEC

## Índice

1. [Presentación](#1-presentación)
2. [Análisis](#2-análisis)
3. [Descripción del lenguaje](#3-descripción-del-lenguaje)
4. [Conclusiones](#4-conclusiones)

## 1. Presentación

`C--` es un compilador para un subconjunto del lenguaje C, extendido con
punteros, arreglos multidimensionales, structs, `long long` y `unsigned`.
Implementa un pipeline clásico de cinco etapas — scanner → parser →
typechecker → constant folding → generación de código — y produce
ensamblador x86-64 , que `gcc` ensambla y enlaza a un
ejecutable nativo.

Este informe va primero a lo empírico: la sección 2 mide y explica qué
tan rápido compila y ejecuta `C--` frente a GCC y Clang, y por qué. La
sección 3 documenta el diseño del lenguaje (gramática, tipos, semántica,
arquitectura interna). La sección 4 cierra con las conclusiones generales.

## 2. Análisis

### 2.1 Optimizaciones implementadas

`C--` aplica tres optimizaciones: **constant folding**, un **atajo tipo
mirilla (peephole)** para stores de constantes, y **bin packing** de
offsets en el stack frame. Además, la eliminación de ramas muertas que
hace constant folding (ver más abajo).

**Constant folding** (`ConstantFolding.cpp`) es un paso del pipeline que
corre después del `TypeChecker` y antes de `GenCodeVisitor`.

- **Qué hace**: evalúa en tiempo de compilación cualquier subexpresión
  cuyos operandos sean *todos* literales (`NumberLiteralNode`,
  `FloatLiteralNode`, `BoolLiteralNode`, `CharLiteralNode`) y anota el
  resultado en el nodo del AST (`isConstant` / `constantValue`). Por
  ejemplo, `2 + 3 * 4` queda anotado como constante con valor `14` antes
  de generar código. En cuanto un operando es una variable, una llamada a
  función, un acceso a arreglo/struct o una desreferencia, la
  subexpresión completa se marca como no-constante — el folding no es
  parcial dentro de una expresión mixta: en `a + 3 * 4`, si `a` no es
  constante, toda la suma se genera en tiempo de ejecución, aunque
  `3 * 4` sea trivialmente plegable de forma aislada.
- **Dónde se aprovecha en codegen**: además de sustituir la subexpresión
  constante por su valor (`movq $14, %rax` en vez de sumar y multiplicar
  en runtime), `GenCodeVisitor` consulta `isConstant` en las condiciones
  de `if`, `while`, `do-while` y `for` para eliminar la rama o el chequeo
  que nunca se ejecuta — p. ej. `while (1) { ... }` genera un loop sin
  `cmp`/`je` por iteración, y `if (0) { ... }` no genera código para la
  rama `then` en absoluto.

**Mirilla para stores de constantes** (`directStoreForConstant` en
`Gencode.cpp`) es un atajo local en el generador de código, no un paso
separado que reescanea el ensamblador ya emitido: cuando el valor a
asignar o inicializar es un literal (o una subexpresión ya plegada por
constant folding) y el destino es una variable simple de `int`, `char`,
`long long`, `unsigned` o `float`, `GenCodeVisitor` reconoce el patrón
"cargar constante en un registro, luego guardar el registro en memoria"
y lo colapsa en una única instrucción de store con la constante como
inmediato — la esencia de una optimización de mirilla, aplicada en el
momento de emitir código en vez de como una pasada posterior.

- **Ejemplo real**: compilando `int x = 5; x = 7;` con `./c-- -c`, ambas
  líneas emiten un solo `movl $5, -16(%rbp)` y `movl $7, -16(%rbp)`
  respectivamente — nunca se genera el `movq $5, %rax` /
  `movq %rax, -16(%rbp)` que produciría el camino genérico (evaluar la
  expresión en `%rax` y luego guardarla).
- **Dónde no aplica**: variables `double` (un inmediato de 64 bits no
  cabe como operando inmediato de una instrucción x86-64) y cualquier
  destino cuyo valor no sea conocido en tiempo de compilación (una
  variable, una llamada a función, etc.) — ahí se usa el camino normal.
- **Alcance**: cubre asignaciones (`x = 5;`) e inicializaciones de
  variables locales (`int x = 5;`); no aplica a variables globales, que
  ya se inicializan directamente en `.data` por un mecanismo aparte.

**Bin packing de offsets en el stack frame** (`TypeChecker::assignOffsets`)
decide en qué posición del frame vive cada variable local. En vez de
darle a cada variable su propio slot de 8 bytes sin importar su tamaño
real, agrupa las variables por tamaño (>8 B, 8 B, 4 B, 1 B) y empaqueta
varias en el mismo bloque de 8 bytes: dos variables de 4 bytes (`int`,
`float`, `unsigned`) comparten un bloque, y hasta cuatro de 1 byte
(`char`, `bool`) comparten un bloque en sub-slots de 2 bytes.

- **Ejemplo real**: `int a=1; int b=2; int c=3; char d='x'; char e='y';`
  compilado con `./c-- -c` reserva un frame de **32 bytes** (`subq $32,
  %rsp`): `a`/`b` comparten un bloque de 8 B (offsets `-32`/`-28`), `c`
  ocupa el siguiente bloque de 8 B él solo (queda impar), y `d`/`e`
  comparten el último bloque de 8 B (offsets `-16`/`-14`). Sin bin
  packing (un slot de 8 B por variable, 5 variables) el frame habría
  necesitado 40 bytes crudos (48 alineados a 16) en vez de los 24 crudos
  (32 alineados) que efectivamente usa.
- **Qué no hace**: no reordena variables para minimizar *padding* entre
  bloques de distinto tamaño más allá de la agrupación por clase, ni
  reutiliza el espacio de variables cuyo *lifetime* ya terminó (cada
  variable declarada en la función ocupa su bloque durante todo el
  frame, sin importar en qué scope se usa).


### 2.2 Metodología

Comparación empírica del compilador `C--` contra **GCC** y **Clang** en
tiempo de compilación, tiempo de ejecución y tamaño de binario. El
detalle completo de la metodología, scripts y datos crudos vive en
[`comparativa/`](comparativa/comparativa.md).

**Benchmarks** (cada uno existe en par equivalente `benchmarks_cnn/*.cnn`
y `benchmarks_c/*.c`):

| Nombre | Descripción |
|--------|------------|
| bench_fib | Fibonacci recursivo (n=35) — recursión y llamadas a función |
| bench_matmul | Matmul 80×80 — arreglos 2D y loops anidados |
| bench_float | Suma de cuadrados float (n=500k) — aritmética float |
| bench_struct | Struct + puntero en loop (n=500k) — structs, punteros y `->` |
| bench_prime | Criba de primos hasta 40k — loops, módulo, condicionales |
| bench_mixed | Struct con int y float (n=150k) — tipos combinados |
| bench_constfold | Expresión literal reevaluada 12M veces en un loop — mide constant folding (2.1) |
| bench_conststore | 6 variables reseteadas a literales 15M veces en un loop — mide la mirilla de stores de constantes (2.1) |

Los últimos dos son microbenchmarks dirigidos: cada uno satura el
patrón de código que explota una optimización específica de 2.1, en vez
de representar una carga de trabajo general como los primeros seis.

**Métricas**:

| Métrica | Descripción |
|---------|-------------|
| Compilación (codegen) | `c-- -c` — solo frontend + typecheck + codegen |
| Compilación (full) | `c-- --exec` — pipeline completo hasta ejecutable |
| Compilación GCC | `gcc` con `-O0` y `-O2` hasta binario |
| Compilación Clang | `clang` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |

**Entorno**: GCC 16.1.1, Clang 22.1.6, Python 3.14.6,
7 repeticiones por medición, timeout de ejecución de 120 s. La comparación es a tres vías
(`C--` / GCC / Clang).

**Limitaciones**: mediciones realizadas en un entorno Linux nativo. Las
tres rutas de compilación (`C--`, GCC, Clang) comparten el mismo costo
de *spawn* de proceso del sistema operativo, por lo que las diferencias
de tiempo de compilación reflejan directamente el trabajo de cada
compilador. `C--` aplica constant folding, mirilla de stores de
constantes y bin packing de offsets (ver 2.1), mientras GCC -O2 y
Clang -O2 aplican decenas de passes de optimización (vectorización,
inlining, desenrollado de loops, eliminación de código muerto,
razonamiento sobre loops completos, etc.).

### 2.3 Tiempos de compilación

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

![Tiempos de compilación](comparativa/results/charts/compilation_times.svg)

En modo `c-- -c` (solo parseo + typecheck + codegen), `C--` toma
consistentemente ~2.9–4.6 ms — entre 10 y 16 veces menos tiempo que su
propio pipeline completo (es decir, si el pipeline completo tarda, por
ejemplo, 30 ms, el modo codegen puro tarda entre 2 y 3 ms sobre el mismo
programa). El pipeline completo (`--exec`) delega en `gcc` para
ensamblar/enlazar, y en esta medición resultó más rápido (~31.0–49.3 ms)
que `GCC -O0`/`-O2` (~52.5–101.3 ms) y que `Clang -O0`/`-O2`
(~70.5–93.3 ms) en los ocho benchmarks — una ventaja real del pipeline
completo de `C--` en este entorno, consistente con que `C--` no corre
ningún análisis pesado antes de invocar a `gcc`.

### 2.4 Tiempos de ejecución

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

![Tiempos de ejecución](comparativa/results/charts/execution_times.svg)

**Speedups de ejecución (vs. C--):**

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

> **Comentario:** un valor de `5.25×` en la columna
> `GCC -O2`, fila `bench_fib`, significa que `C--` tardó **5.25 veces
> más tiempo** en ejecutar ese programa que el binario compilado con
> `gcc -O2` — si GCC -O2 tarda 1.0 segundo, `C--` tarda 5.25 segundos en
> completar el mismo trabajo.

La mediana de speedup de GCC -O2 sobre `C--` es **~5.31×** (contando los
ocho benchmarks); la de Clang -O2 es **~4.96×**. Entre los seis
benchmarks generales, las brechas más grandes se dan en:

- **bench_float**: GCC (8.04×) y sobre todo Clang (7.99×) vectorizan con SSE/AVX; `C--` usa SSE escalar.
- **bench_struct (5.48× Clang)** y **bench_matmul (5.37× GCC)**: desenrollado de loops y reutilización de registros que `C--` no hace (spillea todo al stack).
- **bench_fib (5.25× GCC)** y **bench_mixed (4.41–4.44×)**: mezcla de aritmética y recursión/tipos donde GCC/Clang reutilizan registros.

Para entender **por qué** cada benchmark abre una brecha distinta, vale
la pena mirar qué domina su tiempo de ejecución y qué palanca de
optimización explota GCC/Clang ahí:

| Benchmark | Qué domina el tiempo de ejecución | Por qué la brecha es grande o chica |
|---|---|---|
| bench_float | Aritmética float en un loop de 500k iteraciones | GCC/Clang vectorizan con SSE/AVX (varias operaciones float por instrucción); `C--` emite una instrucción escalar por operación → brecha grande (7.99×–8.04×) |
| bench_matmul | Loops anidados 80×80 con acceso a arreglos 2D | GCC/Clang desenrollan los loops internos y mantienen índices en registros; `C--` recalcula direcciones y recarga desde el stack en cada iteración → brecha grande (3.70×–5.37×) |
| bench_struct | Acceso a campos de struct vía puntero en un loop de 500k | Mismo patrón que matmul, agravado por la indirección del puntero → brecha grande, sobre todo en Clang (5.48×) |
| bench_fib | Recursión (`fib(35)`) con muchas llamadas a función | GCC reutiliza registros entre llamadas; `C--` respeta la convención de llamada al pie de la letra en cada frame, sin inlining → brecha considerable (5.25× GCC) |
| bench_mixed | Struct con campos `int` y `float` combinados (150k) | Combina los costos de bench_float y bench_struct pero a menor escala → brecha intermedia (4.41×–4.44×) |
| bench_prime | Criba de primos con módulo (40k) | Dominado por enteros y control de flujo, con poco margen de vectorización incluso para GCC/Clang → la brecha **más chica** entre los seis generales (1.51×–1.67×) |

`bench_constfold` (44.42×–47.77×) y `bench_conststore` (82.11×–85.22×)
quedan muy por encima de todo lo anterior, y por una razón distinta a
"GCC genera mejor código": inspeccionando `gcc -O2 -S` sobre ambos
fuentes, el `main` generado no tiene **ningún** salto (`jmp`/`jne`/`jl`)
— GCC prueba que el resultado final del loop es una expresión cerrada en
tiempo de compilación (constante de inducción más la suma aritmética de
`i` sobre un rango conocido) y lo reemplaza directo por un
`movabsq $73359558000000, %rdx` seguido del `printf`, sin ejecutar ni una
iteración. `C--` sí aplica constant folding y mirilla de stores (2.1) —
por eso sigue siendo más rápido que sus propias compilaciones sin esas
optimizaciones — pero ninguna de las dos razona sobre el loop *completo*:
cada una de las 12–15 millones de iteraciones de `C--` se sigue
ejecutando en runtime. Estos dos microbenchmarks miden exactamente el
techo que las optimizaciones actuales de `C--` no cruzan (folding local
de una subexpresión vs. razonamiento sobre la recurrencia entera de un
loop), así que sus ratios no son comparables 1:1 con los otros seis.

### 2.5 Tamaño de binario

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

![Tamaños de binario](comparativa/results/charts/binary_sizes.svg)

Los binarios de `C--` son comparables a los de GCC y Clang (diferencia
de apenas un par de cientos de bytes en esta corrida); no hay una
penalización sistemática relevante, a pesar de que `C--` no tiene una
pasada general de eliminación de código muerto (ver 2.6).

### 2.6 Fortalezas y debilidades observadas

**Fortalezas de `C--`:**
- Compilación (parseo + typecheck + codegen) extremadamente rápida (~2.9–4.6 ms)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes y ramas muertas en tiempo de compilación (ver 2.1)
- Mirilla local para stores de constantes evita el paso redundante por `%rax` en asignaciones/inicializaciones (ver 2.1)
- Bin packing de offsets agrupa variables pequeñas en el mismo bloque de 8 bytes del stack frame, en vez de un slot completo por variable (ver 2.1)

**Debilidades frente a GCC/Clang -O2:**
- Sin asignación de registros — cada variable vive en un slot de stack
  y se recarga en cada uso. Por ejemplo, `pt.x + pt.y` (dos enteros de un
  struct) genera:
  ```
  leaq -16(%rbp), %rax
  movslq 0(%rax), %rax   ; recarga pt.x desde el stack
  pushq %rax             ; lo apila para preservarlo
  leaq -16(%rbp), %rax
  movslq 4(%rax), %rax   ; recarga pt.y desde el stack
  movq %rax, %rcx
  popq %rax              ; recupera pt.x
  addl %ecx, %eax        ; recién aquí se suma
  ```
  GCC/Clang habrían mantenido ambos campos en registros y sumado
  directamente, sin tocar memoria ni la pila.
- Sin vectorización SIMD
- Sin desenrollado de loops
- Sin eliminación general de código muerto — solo se poda la rama de un
  `if`/`while`/`for` con condición constante (ver 2.1); código
  inalcanzable después de un `return`, variables no usadas o funciones
  no llamadas no se detectan ni se eliminan
- Sin razonamiento sobre la recurrencia completa de un loop — el constant
  folding de `C--` es local a una subexpresión por iteración; no prueba
  que un loop entero sea reducible a un resultado cerrado y lo reemplace,
  como sí hacen GCC/Clang -O2 (ver `bench_constfold`/`bench_conststore`
  en 2.4)
- Sin inlining de funciones

## 3. Descripción del lenguaje

### 3.1 Descripción general

`C--` es un subset de C con las siguientes extensiones y restricciones
respecto al C estándar:

| Se soporta | No se soporta |
|---|---|
| Tipos primitivos (`int`, `char`, `float`, `double`, `bool`, `void`, `long`), `unsigned` | Unions, enums |
| Punteros (`T*`), arreglos multidimensionales (`T[n][m]`) | Punteros a función |
| Structs (`struct Nombre { ... };`) | Herencia / OOP |
| `malloc` / `free`, `sizeof`, `printf` | Librería estándar (`stdio.h`, `stdlib.h`, etc.) |
| `if`/`else`, `switch`, `while`, `do-while`, `for`, `break`/`continue` | Asignación compuesta (`+=`, `-=`, ...), operador ternario |

### 3.2 Gramática formal (EBNF)

```
// ============================================================
// Programa
// ============================================================
program         = decl*

decl            = fundec | vardec | struct_decl

fundec          = type ID "(" param_list ")" body
vardec          = type ID arrsuf [ "=" expr ] ";"
struct_decl     = "struct" ID "{" vardec* "}" ";"

// ============================================================
// Tipos
// ============================================================
type            = btype "*"*

btype           = "void" | "int" | "char" | "float" | "double"
                | "bool" | "long" stype

stype           = "struct" ID

arrsuf          = ("[" [ expr ] "]")*

// ============================================================
// Parámetros
// ============================================================
param_list      = param ("," param)* | ε
param           = type ID arrsuf

// ============================================================
// Sentencias
// ============================================================
stmt            = body | expr_stmt | sel_stmt | iter_stmt
                | jump_stmt | free_stmt | vardec

body            = "{" stmt* "}"
expr_stmt       = [ expr ] ";"

sel_stmt        = if_stmt | switch_stmt
if_stmt         = "if" "(" expr ")" stmt [ "else" stmt ]
switch_stmt     = "switch" "(" expr ")" "{" { case_clause | default_clause } "}"
case_clause     = "case" expr ":" stmt*
default_clause  = "default" ":" stmt*

iter_stmt       = while_stmt | do_stmt | for_stmt
while_stmt      = "while" "(" expr ")" stmt
do_stmt         = "do" stmt "while" "(" expr ")" ";"
for_stmt        = "for" "(" [ init ] ";" [ cond ] ";" [ inc ] ")" stmt
init            = expr | vardec
cond            = expr
inc             = expr

jump_stmt       = break_stmt | cont_stmt | ret_stmt
break_stmt      = "break" ";"
cont_stmt       = "continue" ";"
ret_stmt        = "return" [ expr ] ";"
free_stmt       = "free" "(" expr ")" ";"

// ============================================================
// Expresiones (precedencia descendente)
// ============================================================
expr            = assign_expr
assign_expr     = unary_expr assign_op assign_expr
assign_op       = "="
lor_expr        = land_expr ("||" land_expr)*
land_expr       = eq_expr ("&&" eq_expr)*
eq_expr         = rel_expr { ("==" | "!=") rel_expr }
rel_expr        = add_expr { ("<" | ">" | "<=" | ">=") add_expr }
add_expr        = mul_expr { ("+" | "-") mul_expr }
mul_expr        = pow_expr { ("*" | "/" | "%") pow_expr }
pow_expr        = unary_expr [ "**" pow_expr ]          // right-assoc
unary_expr      = post_expr | "++" unary_expr | "--" unary_expr
                | unary_op unary_expr
unary_op        = "&" | "*" | "-" | "!"
post_expr       = prim_expr { "[" expr "]"                      // index
                            | "(" [ arg_list ] ")"              // call
                            | "." ID | "->" ID
                            | "++" | "--" }
arg_list        = assign_expr ("," assign_expr)*
prim_expr       = ID | const | "(" expr ")"
                | "malloc" "(" expr ")"
                | "sizeof" "(" type ")"
                | "printf" "(" arg_list ")"

// ============================================================
// Constantes
// ============================================================
const           = intc | floatc | charc | boolc | str
intc            = NUM
floatc          = FNUM
charc           = "'" CHAR "'"
boolc           = "true" | "false"
str             = '"' CHAR* '"'

// ============================================================
// Tokens léxicos
// ============================================================
ID              = letter { letter | digit | "_" }
NUM             = digit digit*
FNUM            = digit digit* "." digit digit*
                  [ ("e" | "E") [ "+" | "-" ] digit digit* ]
CHAR            = printable_ascii | escape_sequence
escape_sequence = "\\" ( "'" | '"' | "\\" | "n" | "t" | "r" | "0" )
letter          = "A".."Z" | "a".."z"
digit           = "0".."9"
```

### 3.3 Sistema de tipos

| Tipo      | Palabra      | Tamaño | Observaciones                                     |
|-----------|--------------|--------|---------------------------------------------------|
| void      | `void`       | —      | Solo como retorno de función                      |
| int       | `int`        | 4      | Entero con signo                                  |
| long      | `long long`  | 8      | Entero largo con signo                            |
| char      | `char`       | 1      | Tratado como int en aritmética                    |
| float     | `float`      | 4      | Punto flotante precisión simple (SSE)             |
| double    | `double`     | 8      | Punto flotante precisión doble (SSE)              |
| bool      | `bool`       | 1      | `true` = 1, `false` = 0                           |
| unsigned  | `unsigned`   | 4      | Modificador sin signo (mapea a `int`)             |
| puntero   | `T*`         | 8      | `&x`, `*p`, `p->m`, `arr[i]` sobre puntero       |
| arreglo   | `T[n]`       | n×\|T\| | Multidimensional: `int m[2][3]`                   |
| struct    | `struct`     | ∑      | Declaración: `struct Nombre { ... };`             |
| string    | n/a          | 8      | Literal: `"hola"` → `int` (dirección en .rodata) |

**Conversiones y promociones** (`check_assign`):

| Destino (T) | Valor (V)              | Acción                      |
|-------------|------------------------|-----------------------------|
| `T`         | `T`                    | Directa (match)             |
| `T*`        | `U*` (cualquier)       | Coerción de puntero         |
| `int`       | `char` o `long`        | Promoción/truncamiento      |
| `char`      | `int` o `long`         | Truncamiento                |
| `long`      | `int`, `char` o `bool` | Promoción                   |
| `float`     | `int`, `char` o `long` | `cvtsi2ss` (int→float)      |
| `double`    | `int`, `char` o `long` | `cvtsi2sd` (int→double)     |
| `double`    | `float`                | `cvtss2sd` (float→double)   |
| `bool`      | `int`, `char` o `long` | Conversión a booleano       |
| `int`/`char`/`long` | `bool`          | Extensión                  |

En aritmética, el tipo de resultado se determina por el operando más
grande: `double` domina sobre `float`, que domina sobre `long`, que
domina sobre `int` (`char` promueve a `int`).

### 3.4 Semántica

**Scoping**: búsqueda de identificadores del scope más interno al más
externo (shadowing permitido); no se permite redeclarar una variable en
el mismo scope; funciones y structs viven en scope global y no pueden
redefinirse.

| Constructo                     | Regla                                                              |
|--------------------------------|--------------------------------------------------------------------|
| Asignación `=`                 | Tipos compatibles (ver conversiones); promoción automática         |
| Condición `if` `while` `for`   | Debe ser `bool`                                                    |
| `&&` `\|\|` `!`                | Operandos `bool`; resultado `bool`                                 |
| `==` `!=` `<` `>` `<=` `>=`    | Operandos numéricos; resultado `bool`                              |
| `+` `-` `*` `/` `%`           | Numéricos; promoción al tipo más grande                             |
| `**` potencia                  | Numéricos; exponente entero para código nativo                     |
| `[]` indexación                | Base: arreglo o puntero; índice: `int` o `char`                    |
| `.` / `->`                     | Objeto (o puntero) debe ser struct; miembro debe existir            |
| Llamada a función              | Aridad y tipos coinciden con la firma declarada                    |
| `return`                       | Tipo debe coincidir con el de retorno (o ser asignable)             |
| `break` / `continue`           | Solo dentro de `while`, `for`, `do-while` o `switch`                |
| `malloc` / `free`              | `malloc` retorna `void*`; `free` recibe `void*`                    |
| `sizeof`                       | Retorna `int` con el tamaño en bytes del tipo                      |

**Manejo de errores**: errores léxicos abortan la compilación en el
primer carácter no reconocido; errores sintácticos lanzan una excepción
con el token esperado vs. encontrado; los errores semánticos se
acumulan en `TypeChecker::errors` y, si hay al menos uno, el compilador
termina con `exit(1)` sin generar código.

### 3.5 Operadores

| Símbolo | Precedencia | Asoc.     | Descripción                |
|---------|-------------|-----------|-----------------------------|
| `=`     | 1           | derecha   | Asignación                 |
| `\|\|`  | 2           | izquierda | OR lógico                  |
| `&&`    | 3           | izquierda | AND lógico                 |
| `==` `!=` | 4         | izquierda | Igualdad / desigualdad      |
| `<` `>` `<=` `>=` | 5   | izquierda | Comparación relacional      |
| `+` `-` | 6           | izquierda | Suma / resta                |
| `*` `/` `%` | 7       | izquierda | Multiplicación / división / módulo |
| `**`    | 8           | derecha   | Potencia (exponenciación)   |

Unarios: `-` (negación aritmética), `!` (negación lógica), `&`
(dirección), `*` (desreferencia), `++x`/`--x` (pre) y `x++`/`x--`
(post). No existen operadores de asignación compuesta (`+=`, `-=`, etc.)
ni el operador ternario.

### 3.6 Arquitectura del compilador

```
Código fuente
    │
    ▼
Scanner (lexer) ──► tokens
    │
    ▼
Parser (recursive descent) ──► AST (Program*)
    │
    ├─► TypeChecker ──► verificación semántica + offsets de stack frame
    ├─► ConstantFolding ──► plegado de expresiones constantes
    └─► GenCodeVisitor ──► código ensamblador x86-64 (AT&T/GAS)
```

El AST usa **triple dispatch**: cada nodo implementa `accept` para tres
visitors (`Visitor` de interpretación, `TypeVisitor` de typechecking,
`CodeGenVisitor` de generación de código), identificados en tiempo de
ejecución con `dynamic_cast`. Los offsets de cada variable dentro del
stack frame se calculan con un algoritmo de *bin packing*
(`TypeChecker::assignOffsets`, ver 2.1) que agrupa variables por tamaño
para no desperdiciar un slot de 8 bytes completo en cada una; el frame
resultante se alinea a 16 bytes.

Ver [`docs/lenguaje.md`](docs/lenguaje.md) para el detalle completo de
la jerarquía de nodos del AST, el manejo de l-values (`captureLVal` /
`storeTarget`) y la gestión de memoria del árbol.

### 3.7 Ejemplos

**Struct:**

```c
struct Punto {
    int x;
    int y;
};

int main() {
    struct Punto pt;
    pt.x = 10;
    pt.y = 20;
    printf("%d\n", pt.x + pt.y); // esperado: 30
    return 0;
}
```

**Long long:**

```c
int main() {
    long long big;
    big = 100000;
    printf("%d\n", big); // esperado: 100000
    return 0;
}
```

**Unsigned:**

```c
int main() {
    unsigned u;
    u = 42;
    printf("%d\n", u); // esperado: 42
    return 0;
}
```


## 4. Conclusiones

- `C--` cumple su objetivo como compilador didáctico: implementa un
  pipeline completo (scanner → parser → typechecker → constant folding
  → codegen x86-64) capaz de compilar un subset expresivo de C —
  incluyendo punteros, structs, arreglos multidimensionales, `long long`
  y `unsigned` — a código ensamblador nativo funcional.
- Las optimizaciones implementadas (constant folding, mirilla para
  stores de constantes y bin packing de offsets, ver 2.1) son correctas
  y baratas, y `bench_constfold`/`bench_conststore` (2.2) confirman que
  sí se activan en la práctica — pero son locales (una subexpresión o un
  store por vez) y no reemplazan el rendimiento de un análisis que razone
  sobre el loop completo, como sí hacen GCC/Clang -O2 en esos mismos dos
  benchmarks (ver 2.4).
- El costo de no tener asignación de registros, vectorización, inlining
  ni eliminación de código muerto se paga en tiempo de ejecución: el
  código generado por `C--` es, en la mediana de los ocho benchmarks,
  **5.31 veces más lento** que el de GCC -O2 y **4.96 veces más lento**
  que el de Clang -O2 (es decir, tarda entre ~5 y ~5.3 veces más para
  el mismo trabajo). Entre los seis benchmarks generales, la brecha es
  más amplia en cargas aritméticas vectorizables (`bench_float`) y más
  estrecha en cargas dominadas por enteros y control de flujo
  (`bench_prime`); `bench_constfold` y `bench_conststore` abren una
  brecha muchísimo mayor (44×–85×) porque GCC/Clang prueban que el loop
  completo es reducible a un valor constante y lo reemplazan sin
  ejecutarlo (ver 2.4).
- El tamaño de binario es prácticamente idéntico al de GCC y Clang en
  esta corrida, por lo que la ausencia de eliminación de código muerto
  no representa hoy un costo significativo en los programas de prueba.
