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

1. [Documentación del lenguaje](#1-documentación-del-lenguaje)
2. [Optimizaciones aplicadas](#2-optimizaciones-aplicadas)
3. [Resultados](#3-resultados)
4. [Conclusiones generales](#4-conclusiones-generales)

## Introducción

`C--` es un compilador para un subconjunto del lenguaje C, extendido con
punteros, arreglos multidimensionales, structs, templates genéricos y
lambdas con captura por valor. El compilador implementa un pipeline
clásico de cinco etapas (scanner → parser → typechecker → constant
folding → generación de código) y produce código ensamblador x86-64
(sintaxis AT&T/GAS), que puede ensamblarse y enlazarse a un ejecutable
nativo mediante `gcc`.

Este informe documenta el diseño del lenguaje, las optimizaciones
implementadas sobre el código generado, y una comparación empírica de
rendimiento frente a `gcc` y `clang`.

## 1. Documentación del lenguaje

### 1.1 Descripción general

`C--` es un subset de C con las siguientes extensiones y restricciones
respecto al C estándar:

| Se soporta | No se soporta |
|---|---|
| Tipos primitivos (`int`, `char`, `float`, `double`, `bool`, `void`), `auto` | Unions, enums |
| Punteros (`T*`), arreglos multidimensionales (`T[n][m]`) | Punteros a función (salvo lambdas) |
| Structs (`struct Nombre { ... };`) | Herencia / OOP |
| Templates genéricos (`template<typename T>`) sobre funciones y structs | Templates variádicos, especialización parcial |
| Lambdas con captura por valor (`[x](int a) -> int { ... }`) | Captura por referencia (`[&x]`, `[&]`) — error semántico |
| `malloc` / `free`, `sizeof`, `printf` | Librería estándar (`stdio.h`, `stdlib.h`, etc.) |
| `if`/`else`, `switch`, `while`, `do-while`, `for`, `break`/`continue` | Asignación compuesta (`+=`, `-=`, ...), operador ternario |

### 1.2 Gramática formal (EBNF)

```
// ============================================================
// Programa
// ============================================================
program         = decl*

decl            = fundec | vardec | struct_decl | templ_decl

fundec          = type ID "(" param_list ")" body
vardec          = type ID arrsuf [ "=" expr ] ";"
struct_decl     = "struct" ID "{" vardec* "}" ";"

// ============================================================
// Tipos (incluye templates)
// ============================================================
type            = btype "*"*

btype           = "void" | "int" | "char" | "float" | "double"
                | "bool" | "auto" | stype | template_type

stype           = "struct" ID
template_type   = ID "<" type_list ">"              // Vector<int>
                | "struct" ID "<" type_list ">"      // struct Vector<int>

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
                            | "<" type_list ">" "(" arg_list ")" // template call f<T>(a)
                            | "." ID | "->" ID
                            | "++" | "--" }
arg_list        = assign_expr ("," assign_expr)*
type_list       = type ("," type)*
prim_expr       = ID | const | "(" expr ")"
                | "malloc" "(" expr ")"
                | "sizeof" "(" type ")"
                | "printf" "(" arg_list ")"
                | lambda_expr

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
// Lambdas
// ============================================================
lambda_expr     = "[" [ cap_list ] "]" "(" param_list ")" "->" type body
cap_list        = cap ("," cap)*
cap             = "="              // [=] default by-value (parsed, no-op)
                | "&"              // [&] default by-ref (error semántico)
                | ID               // [x] named by-value
                | "&" ID           // [&x] named by-ref (error semántico)

// ============================================================
// Templates
// ============================================================
templ_decl      = "template" "<" tparam_list ">" ( fundec | struct_decl )
tparam_list     = "typename" ID ("," "typename" ID)*

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

### 1.3 Sistema de tipos

| Tipo      | Palabra | Tamaño | Observaciones                                     |
|-----------|---------|--------|---------------------------------------------------|
| void      | `void`  | —      | Solo como retorno de función                      |
| int       | `int`   | 4      | Entero con signo                                  |
| char      | `char`  | 1      | Tratado como int en aritmética                    |
| float     | `float` | 4      | Punto flotante precisión simple (SSE)             |
| double    | `double`| 8      | Punto flotante precisión doble (SSE)              |
| bool      | `bool`  | 1      | `true` = 1, `false` = 0                           |
| auto      | `auto`  | —      | Inferido del inicializador obligatorio            |
| puntero   | `T*`    | 8      | `&x`, `*p`, `p->m`, `arr[i]` sobre puntero       |
| arreglo   | `T[n]`  | n×\|T\| | Multidimensional: `int m[2][3]`                   |
| struct    | `struct`| ∑      | Declaración: `struct Nombre { ... };`             |
| string    | n/a     | 8      | Literal: `"hola"` → `int` (dirección en .rodata) |
| lambda    | n/a     | 8      | Expresión lambda → `void*` (puntero a función)   |

**Conversiones y promociones** (`check_assign`):

| Destino (T) | Valor (V)         | Acción                      |
|-------------|-------------------|-----------------------------|
| `T`         | `T`               | Directa (match)             |
| `T*`        | `U*` (cualquier)  | Coerción de puntero         |
| `int`       | `char`            | Promoción                   |
| `char`      | `int`             | Truncamiento                |
| `float`     | `int` o `char`    | `cvtsi2ss` (int→float)      |
| `double`    | `int` o `char`    | `cvtsi2sd` (int→double)     |
| `double`    | `float`           | `cvtss2sd` (float→double)   |

En aritmética, el tipo de resultado se determina por el operando más
grande: `double` domina sobre `float`, que domina sobre `int` (`char`
promueve a `int`).

### 1.4 Semántica

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
| Llamada a template `f<T>(a)`   | Instancia el template con `T` y verifica argumentos                |
| `return`                       | Tipo debe coincidir con el de retorno (o ser asignable)             |
| `break` / `continue`           | Solo dentro de `while`, `for`, `do-while` o `switch`                |
| `malloc` / `free`              | `malloc` retorna `void*`; `free` recibe `void*`                    |
| `auto`                         | Tipo inferido del inicializador obligatorio                        |
| `sizeof`                       | Retorna `int` con el tamaño en bytes del tipo                      |

**Templates**: se declaran con `template<typename T>` sobre una función
o un struct. La instanciación en tipos usa `Par<int>` o
`struct Par<int>` (equivalentes); en llamadas usa `identidad<int>(42)`.
El nombre concreto se genera con *name mangling* (`Par<int>`) y las
instancias se cachean para evitar duplicados.

**Lambdas**: solo `[x]` (captura por valor con nombre) es funcional.
`[&x]` y `[&]` (captura por referencia) están reconocidos por el parser
pero producen un error semántico, ya que el generador de código no
implementa upvalues por referencia.

**Manejo de errores**: errores léxicos abortan la compilación en el
primer carácter no reconocido; errores sintácticos lanzan una excepción
con el token esperado vs. encontrado; los errores semánticos se
acumulan en `TypeChecker::errors` y, si hay al menos uno, el compilador
termina con `exit(1)` sin generar código.

### 1.5 Operadores

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

### 1.6 Arquitectura del compilador

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

### 1.7 Ejemplos

**Templates genéricos** (función y struct):

```c
template <typename T>
struct Pair {
    T first;
    T second;
};

template <typename T>
T add(T a, T b) {
    return a + b;
}

int main() {
    int s;
    s = add<int>(10, 20);
    printf(s); // esperado: 30

    Pair<int> p;
    p.first = 5;
    p.second = 15;
    int sum;
    sum = p.first + p.second;
    printf(sum); // esperado: 20

    return 0;
}
```

**Lambdas con captura por valor:**

```c
int main() {
    auto add = [](int a, int b) -> int {
        return a + b;
    };
    int sum1;
    sum1 = add(5, 10);

    int x;
    x = 20;
    auto addX = [x](int a) -> int {
        return a + x;
    };
    int sum2;
    sum2 = addX(5);

    printf(sum1);  // esperado: 15
    printf(sum2);  // esperado: 25

    return 0;
}
```

### 1.8 Cobertura de pruebas

El lenguaje se valida con 20 pruebas de integración
(`tests/integracion/`), cada una enfocada en una característica:

| # | Test | Cubre |
|---|------|-------|
| 01 | `test01_funciones` | Declaración y llamada a funciones |
| 02 | `test02_control_flujo` | `if`/`else`, `while`, `for` |
| 03 | `test03_variables_ops` | Variables y operadores aritméticos |
| 04 | `test04_operadores_unarios` | `-`, `!`, `++`, `--` |
| 05 | `test05_tipos_char` | Tipo `char` |
| 06 | `test06_structs` | Declaración y acceso a `struct` |
| 07 | `test07_arrays` | Arreglos unidimensionales |
| 08 | `test08_malloc_free` | Memoria dinámica |
| 09 | `test09_switch` | `switch` / `case` / `default` |
| 10 | `test10_expresiones_complejas` | Precedencia y anidamiento de expresiones |
| 11 | `test11_pointers` | Punteros, `&`, `*`, `->` |
| 12 | `test12_sizeof` | Operador `sizeof` |
| 13 | `test13_type_inference` | Inferencia con `auto` |
| 14 | `test14_lambda` | Lambdas y captura por valor |
| 15 | `test15_templates` | Templates sobre funciones y structs |
| 16 | `test16_scope` | Scoping y shadowing |
| 17 | `test17_multidim_arrays` | Arreglos multidimensionales |
| 18 | `test18_type_promotion` | Promoción de tipos numéricos |
| 19 | `test19_strings` | Literales de string |
| 20 | `test20_float_double` | Aritmética `float`/`double` |

## 2. Optimizaciones aplicadas

> **Sección pendiente.** Se documentará en una siguiente iteración de
> este informe el detalle de las optimizaciones implementadas sobre el
> código generado (actualmente incluye *constant folding* sobre
> expresiones literales — ver `ConstantFolding.cpp`).

## 3. Resultados

Comparación empírica del compilador `C--` contra **GCC** y **Clang** en
tiempo de compilación, tiempo de ejecución y tamaño de binario. El
detalle completo de la metodología, scripts y datos crudos vive en
[`comparativa/`](comparativa/comparativa.md).

### 3.1 Metodología

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
| Compilación GCC/Clang | `gcc` / `clang` con `-O0` y `-O2` hasta binario |
| Ejecución | Mediana de **7 ejecuciones** (timeout 120 s por run) |
| Tamaño | Bytes del ejecutable en disco |

**Entorno**: GCC 16.1.1, Clang 22.1.6, Python 3.14.6, 7 repeticiones por
medición, timeout de ejecución de 120 s (fecha de la medición:
2026-07-01).

**Limitaciones**: `C--` usa `printf(int)`/`printf(double)` en vez de
formato estilo `printf("%ld\n", ...)`, por lo que en benchmarks
CPU-bound el impacto es bajo; `C--` solo aplica constant folding
mientras GCC/Clang -O2 aplican decenas de passes de optimización; las
mediciones se hicieron en una sola máquina sin normalizar por
frecuencia de CPU.

### 3.2 Tiempos de compilación

| Benchmark | C-- codegen | C-- full | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|------------:|---------:|--------:|--------:|----------:|----------:|
| bench_fib | 1.3 ms | 17.3 ms | 23.9 ms | 49.7 ms | 38.2 ms | 37.5 ms |
| bench_matmul | 1.3 ms | 18.9 ms | 26.2 ms | 37.9 ms | 39.0 ms | 46.8 ms |
| bench_float | 1.4 ms | 15.8 ms | 27.5 ms | 29.3 ms | 35.9 ms | 41.1 ms |
| bench_struct | 1.3 ms | 18.9 ms | 24.8 ms | 29.1 ms | 39.1 ms | 36.4 ms |
| bench_prime | 1.4 ms | 16.6 ms | 24.5 ms | 29.8 ms | 35.8 ms | 38.6 ms |
| bench_mixed | 1.3 ms | 16.9 ms | 24.9 ms | 30.4 ms | 35.5 ms | 36.8 ms |

![Tiempos de compilación](comparativa/results/charts/compilation_times.svg)

`C--` codegen es **~20–35× más rápido** que GCC/Clang -O2: el frontend
de `C--` es un parser simple sin optimizaciones multi-pase. El pipeline
completo (`--exec`) añade el tiempo de `gcc` para ensamblar y enlazar,
acercándose a los tiempos de GCC -O0.

### 3.3 Tiempos de ejecución

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 61.7 ms | 51.6 ms | 13.1 ms | 43.7 ms | 25.7 ms |
| bench_matmul | 3.2 ms | 2.4 ms | 0.8 ms | 1.8 ms | 0.9 ms |
| bench_float | 9.0 ms | 2.3 ms | 1.0 ms | 4.4 ms | 1.1 ms |
| bench_struct | 2.3 ms | 1.9 ms | 0.9 ms | 1.7 ms | 0.5 ms |
| bench_prime | 3.7 ms | 2.6 ms | 2.5 ms | 2.8 ms | 2.5 ms |
| bench_mixed | 2.4 ms | 1.4 ms | 0.7 ms | 1.6 ms | 0.5 ms |

![Tiempos de ejecución](comparativa/results/charts/execution_times.svg)

**Speedups de ejecución (vs. C--; ratio > 1 = más rápido que C--):**

| Benchmark | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|--------:|--------:|----------:|----------:|
| bench_fib | 1.20× | 4.70× | 1.41× | 2.40× |
| bench_matmul | 1.35× | 3.93× | 1.78× | 3.64× |
| bench_float | 3.88× | 9.03× | 2.06× | 8.31× |
| bench_struct | 1.20× | 2.61× | 1.34× | 4.39× |
| bench_prime | 1.40× | 1.47× | 1.32× | 1.45× |
| bench_mixed | 1.74× | 3.57× | 1.48× | 4.59× |

La mediana de speedup de GCC -O2 sobre `C--` es **3.75×**. Las brechas
más grandes se dan en:

- **bench_float (9.03×)**: GCC vectoriza con SSE/AVX; `C--` usa SSE escalar.
- **bench_fib (4.70×)**: GCC optimiza la recursión con reuso de registros; `C--` spillea todo al stack.
- **bench_matmul (3.93×)**: GCC desenrolla loops y vectoriza; `C--` accede linealmente.

En **bench_prime (1.47×)** la brecha es menor porque el cuello de
botella es la aritmética entera, donde `C--` genera código directo sin
sobrecarga de llamadas a función.

### 3.4 Tamaño de binario

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|-----------|----:|--------:|--------:|----------:|----------:|
| bench_fib | 16.0 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_matmul | 16.1 KB | 15.7 KB | 15.7 KB | 15.7 KB | 15.7 KB |
| bench_float | 15.9 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_struct | 15.9 KB | 15.7 KB | 15.6 KB | 15.7 KB | 15.6 KB |
| bench_prime | 16.1 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |
| bench_mixed | 15.9 KB | 15.6 KB | 15.6 KB | 15.6 KB | 15.6 KB |

![Tamaños de binario](comparativa/results/charts/binary_sizes.svg)

Los binarios de `C--` son consistentemente **~400-500 bytes más
grandes** que los de GCC. Esto se debe a que `C--` incluye código de
soporte (potencia, formato `printf`) que GCC maneja vía libc, y a que
`C--` no elimina código muerto.

### 3.5 Fortalezas y debilidades observadas

**Fortalezas de `C--`:**
- Compilación extremadamente rápida (~1.3 ms en modo codegen)
- Código generado claro, legible y didáctico
- Constant folding reduce expresiones constantes en tiempo de compilación

**Debilidades frente a GCC/Clang -O2:**
- Sin asignación de registros (todo spillea al stack)
- Sin vectorización SIMD
- Sin desenrollado de loops
- Sin eliminación de código muerto
- Sin inlining de funciones

## 4. Conclusiones generales

- `C--` cumple su objetivo como compilador didáctico: implementa un
  pipeline completo (scanner → parser → typechecker → constant folding
  → codegen x86-64) capaz de compilar un subset expresivo de C —
  incluyendo punteros, structs, arreglos multidimensionales, templates
  genéricos y lambdas — a código ensamblador nativo funcional.
- La velocidad de compilación es la principal ventaja frente a
  herramientas de producción: al no implementar optimizaciones
  multi-pase, `C--` compila entre 20× y 35× más rápido que GCC/Clang en
  modo `-O2` en los benchmarks evaluados.
- El costo de esa simplicidad se paga en tiempo de ejecución: sin
  asignación de registros, vectorización, inlining ni eliminación de
  código muerto, el código generado por `C--` es en mediana ~3.75×
  más lento que el de GCC -O2, con la brecha más amplia en cargas
  aritméticas vectorizables (`bench_float`) y la más estrecha en
  cargas dominadas por enteros y control de flujo (`bench_prime`).
- El tamaño de binario es comparable al de GCC/Clang (diferencia de
  ~400-500 bytes), por lo que la ausencia de eliminación de código
  muerto no representa hoy un costo significativo en los programas de
  prueba.
- Las 20 pruebas de integración cubren las características centrales
  del lenguaje documentadas en la sección 1, dando una base de
  regresión para futuras optimizaciones (sección 2, pendiente).
