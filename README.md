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
ensamblador x86-64 (sintaxis AT&T/GAS), que `gcc` ensambla y enlaza a un
ejecutable nativo.

Este informe va primero a lo empírico: la sección 2 mide y explica qué
tan rápido compila y ejecuta `C--` frente a GCC y Clang, y por qué. La
sección 3 documenta el diseño del lenguaje (gramática, tipos, semántica,
arquitectura interna) para quien quiera profundizar en cómo está
construido. La sección 4 cierra con las conclusiones generales.

## 2. Análisis

### 2.1 Optimizaciones implementadas

`C--` aplica dos optimizaciones sobre el código generado: **constant
folding** y un **atajo tipo mirilla (peephole)** para stores de
constantes.

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

**Por qué ninguna de las dos explica las diferencias de rendimiento de
2.4**: ninguno de los seis benchmarks tiene loops calientes que operen
sobre literales puros o reasignen variables a constantes repetidamente
— todos leen y escriben variables calculadas en runtime (contadores de
loop, elementos de arreglo, campos de struct). Ambas optimizaciones
siguen siendo correctas y útiles para el caso general, pero las brechas
de tiempo de ejecución frente a GCC/Clang vienen de optimizaciones que
`C--` no implementa: asignación de registros, vectorización SIMD,
desenrollado de loops e inlining (ver 2.6).

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

**Métricas**:

| Métrica | Descripción |
|---------|-------------|
| Compilación (codegen) | `c-- -c` — solo frontend + typecheck + codegen |
| Compilación (full) | `c-- --exec` — pipeline completo hasta ejecutable |
| Compilación GCC | `gcc` con `-O0` y `-O2` hasta binario |
| Compilación Clang | `clang` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |

**Entorno**: GCC 16.1.1 (20260625), Clang 22.1.6, Python 3.14.6,
7 repeticiones por medición, timeout de ejecución de 120 s (fecha de la
medición: 2026-07-03 14:43 -05:00). La comparación es a tres vías
(`C--` / GCC / Clang).

**Limitaciones**: mediciones realizadas en un entorno Linux nativo. Las
tres rutas de compilación (`C--`, GCC, Clang) comparten el mismo costo
de *spawn* de proceso del sistema operativo, por lo que las diferencias
de tiempo de compilación reflejan directamente el trabajo de cada
compilador. `C--` solo aplica constant folding (ver 2.1), mientras GCC
-O2 y Clang -O2 aplican decenas de passes de optimización (vectorización,
inlining, desenrollado de loops, eliminación de código muerto, etc.).
Mediciones en una sola máquina, sin normalizar por frecuencia de CPU.

### 2.3 Tiempos de compilación

| Benchmark | C-- codegen | C-- full | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|------------:|---------:|--------:|--------:|----------:|----------:|
| bench_fib | 4.3 ms | 45.2 ms | 50.9 ms | 95.3 ms | 100.3 ms | 125.4 ms |
| bench_matmul | 2.8 ms | 29.6 ms | 51.9 ms | 72.3 ms | 74.8 ms | 121.4 ms |
| bench_float | 2.6 ms | 29.4 ms | 51.6 ms | 56.0 ms | 69.8 ms | 73.4 ms |
| bench_struct | 2.7 ms | 29.2 ms | 67.0 ms | 72.3 ms | 69.2 ms | 89.7 ms |
| bench_prime | 3.2 ms | 28.9 ms | 50.8 ms | 65.4 ms | 75.2 ms | 87.3 ms |
| bench_mixed | 3.1 ms | 32.0 ms | 50.0 ms | 56.1 ms | 69.9 ms | 72.1 ms |

![Tiempos de compilación](comparativa/results/charts/compilation_times.svg)

En modo `c-- -c` (solo parseo + typecheck + codegen), `C--` toma
consistentemente ~2.6–4.3 ms — entre 15 y 30 veces menos tiempo que su
propio pipeline completo (es decir, si el pipeline completo tarda, por
ejemplo, 30 ms, el modo codegen puro tarda entre 1 y 2 ms sobre el mismo
programa). El pipeline completo (`--exec`) delega en `gcc` para
ensamblar/enlazar, y en esta medición resultó más rápido (~28.9–45.2 ms)
que `GCC -O0`/`-O2` (~50.0–95.3 ms) y que `Clang -O0`/`-O2`
(~69.2–125.4 ms) en los seis benchmarks — una ventaja real del pipeline
completo de `C--` en este entorno, consistente con que `C--` no corre
ningún análisis pesado (solo constant folding) antes de invocar a `gcc`.

### 2.4 Tiempos de ejecución

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 129.5 ms | 100.6 ms | 24.8 ms | 90.0 ms | 50.2 ms |
| bench_matmul | 7.2 ms | 3.7 ms | 1.2 ms | 3.1 ms | 1.4 ms |
| bench_float | 14.0 ms | 4.5 ms | 1.9 ms | 7.7 ms | 1.7 ms |
| bench_struct | 6.3 ms | 3.7 ms | 1.7 ms | 3.1 ms | 1.0 ms |
| bench_prime | 6.9 ms | 5.0 ms | 6.2 ms | 6.5 ms | 5.3 ms |
| bench_mixed | 4.4 ms | 2.5 ms | 1.1 ms | 2.9 ms | 1.0 ms |

![Tiempos de ejecución](comparativa/results/charts/execution_times.svg)

**Speedups de ejecución (vs. C--):**

| Benchmark | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|--------:|--------:|----------:|----------:|
| bench_fib | 1.29× | 5.21× | 1.44× | 2.58× |
| bench_matmul | 1.96× | 5.86× | 2.30× | 5.23× |
| bench_float | 3.11× | 7.54× | 1.81× | 8.36× |
| bench_struct | 1.68× | 3.62× | 2.00× | 6.30× |
| bench_prime | 1.39× | 1.12× | 1.07× | 1.30× |
| bench_mixed | 1.72× | 3.87× | 1.50× | 4.23× |

> **Comentario:** un valor de `5.21×` en la columna
> `GCC -O2`, fila `bench_fib`, significa que `C--` tardó **5.21 veces
> más tiempo** en ejecutar ese programa que el binario compilado con
> `gcc -O2` — si GCC -O2 tarda 1.0 segundo, `C--` tarda 5.21 segundos en
> completar el mismo trabajo. 

La mediana de speedup de GCC -O2 sobre `C--` es **~4.54×** (en la
mediana de los seis benchmarks, `C--` tarda 4.54 veces más que GCC -O2);
la de Clang -O2 es **~4.73×**. Las brechas más grandes se dan en:

- **bench_float**: GCC (7.54×) y sobre todo Clang (8.36×) vectorizan con SSE/AVX; `C--` usa SSE escalar.
- **bench_struct (6.30× Clang)** y **bench_matmul (5.86× GCC, 5.23× Clang)**: desenrollado de loops y reutilización de registros que `C--` no hace (spillea todo al stack).
- **bench_fib (5.21× GCC)** y **bench_mixed (3.87–4.23×)**: mezcla de aritmética y recursión/tipos donde GCC/Clang reutilizan registros.

Para entender **por qué** cada benchmark abre una brecha distinta, vale
la pena mirar qué domina su tiempo de ejecución y qué palanca de
optimización explota GCC/Clang ahí:

| Benchmark | Qué domina el tiempo de ejecución | Por qué la brecha es grande o chica |
|---|---|---|
| bench_float | Aritmética float en un loop de 500k iteraciones | GCC/Clang vectorizan con SSE/AVX (varias operaciones float por instrucción); `C--` emite una instrucción escalar por operación → la brecha **más grande** de la tabla (7.54×–8.36×) |
| bench_matmul | Loops anidados 80×80 con acceso a arreglos 2D | GCC/Clang desenrollan los loops internos y mantienen índices en registros; `C--` recalcula direcciones y recarga desde el stack en cada iteración → brecha grande (5.23×–5.86×) |
| bench_struct | Acceso a campos de struct vía puntero en un loop de 500k | Mismo patrón que matmul, agravado por la indirección del puntero → brecha grande, sobre todo en Clang (6.30×) |
| bench_fib | Recursión (`fib(35)`) con muchas llamadas a función | GCC reutiliza registros entre llamadas; `C--` respeta la convención de llamada al pie de la letra en cada frame, sin inlining → brecha considerable (5.21× GCC) |
| bench_mixed | Struct con campos `int` y `float` combinados (150k) | Combina los costos de bench_float y bench_struct pero a menor escala → brecha intermedia (3.87×–4.23×) |
| bench_prime | Criba de primos con módulo (40k) | Dominado por enteros y control de flujo, con poco margen de vectorización incluso para GCC/Clang → la brecha **más chica** de la tabla (1.12×–1.39×), con ruido de medición notable |

En **bench_prime** la brecha es la más chica de la tabla y con una
anomalía: GCC -O2 (6.2 ms) midió más lento que GCC -O0 (5.0 ms) — un
ratio de solo 1.12× (es decir, `-O2` fue apenas un 12% más rápido que
`-O0`, cuando lo normal sería que `-O2` ganara por más margen). Con
cargas de ~5–6 ms y 7 repeticiones, el ruido de scheduling del sistema
operativo pesa más que el efecto real de `-O2` sobre este patrón de
loops/módulo; no debe sobre-interpretarse ese resultado puntual como una
regresión real de `-O2`.

### 2.5 Tamaño de binario

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_matmul | 15.9 KB | 15.7 KB | 15.7 KB | 15.7 KB | 15.7 KB |
| bench_float | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_struct | 15.7 KB | 15.7 KB | 15.6 KB | 15.7 KB | 15.6 KB |
| bench_prime | 15.9 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_mixed | 15.7 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |

![Tamaños de binario](comparativa/results/charts/binary_sizes.svg)

Los binarios de `C--` son comparables a los de GCC y Clang (diferencia
de apenas un par de cientos de bytes en esta corrida); no hay una
penalización sistemática relevante, a pesar de que `C--` no elimina
código muerto.

### 2.6 Fortalezas y debilidades observadas

**Fortalezas de `C--`:**
- Compilación (parseo + typecheck + codegen) extremadamente rápida (~2.6–4.3 ms)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes y ramas muertas en tiempo de compilación (ver 2.1)
- Mirilla local para stores de constantes evita el paso redundante por `%rax` en asignaciones/inicializaciones (ver 2.1)

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
- Sin eliminación de código muerto
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
ejecución con `dynamic_cast`. Cada variable recibe un slot de pila de 8
bytes (o su tamaño real si es mayor), y el frame se alinea a 16 bytes,
evitando la complejidad de un *bin packing* de offsets sin penalización
práctica en x86-64.

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
- La única optimización implementada es constant folding (2.1); es
  correcta y barata, pero no incide en el rendimiento de los benchmarks
  medidos porque estos operan sobre variables, no sobre literales.
- El costo de no tener asignación de registros, vectorización, inlining
  ni eliminación de código muerto se paga en tiempo de ejecución: el
  código generado por `C--` es, en la mediana de los seis benchmarks,
  **4.54 veces más lento** que el de GCC -O2 y **4.73 veces más lento**
  que el de Clang -O2 (es decir, tarda entre ~4.5 y ~4.7 veces más para
  el mismo trabajo). La brecha es más amplia en cargas aritméticas
  vectorizables (`bench_float`) y más estrecha en cargas dominadas por
  enteros y control de flujo (`bench_prime`).
- El tamaño de binario es prácticamente idéntico al de GCC y Clang en
  esta corrida, por lo que la ausencia de eliminación de código muerto
  no representa hoy un costo significativo en los programas de prueba.
- Las pruebas de integración (`tests/integracion/`) cubren las
  características centrales del lenguaje documentadas en la sección 3,
  dando una base de regresión para futuras optimizaciones.
