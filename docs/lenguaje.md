# C-- — Especificación del Lenguaje

Subset de C con punteros, structs, `long long`, `unsigned` y memoria dinámica.
Compilado a x86-64 (AT&T/GAS) vía recursive descent + triple dispatch.

---

## 1. Características

### 1.1 Soportadas (Esencia de C)
- **Tipos de datos básicos**: `int`, `char`, `float`, `double`, `long long`, `void`, `unsigned`.
- **Tipos definidos por el usuario**: `struct`.
- **Modificadores**: `const`.
- **Variables y manejo de alcance (scope)**: Locales y globales.
- **Funciones y Estructuras de control**: `if`, `for`, `while`, `switch`, `do while`.
- **Arreglos y Cadenas**: Arreglos de tipos básicos, strings basados en arreglos de `char`.
- **Arreglos multidimensionales**: Matrices contiguas en memoria.
- **Punteros y memoria dinámica**: `malloc`, `free`, aritmética de punteros.
- **Conversión y promoción automática**: Conversiones implícitas (ej. `int` a `float`).

### 1.2 NO soportadas (Lo que C NO tiene)
- **Tipos genéricos y plantillas (templates)**: No existe metaprogramación.
- **Inferencia de tipos**: No existe `auto` o `var` para deducción de tipos.
- **Funciones lambda**: No hay funciones anónimas nativas.

---

## 2. Gramática

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

---

## 2. Semántica

### 2.1 Sistema de tipos

| Tipo      | Palabra      | Tamaño | Observaciones                                     |
|-----------|--------------|--------|---------------------------------------------------|
| void      | `void`       | —      | Solo como retorno de función                      |
| int       | `int`        | 4      | Entero con signo                                  |
| long      | `long long`  | 8      | Entero largo con signo                            |
| char      | `char`       | 1      | Tratado como int en aritmética                    |
| float     | `float`      | 4      | Punto flotante precisión simple (SSE)             |
| double    | `double`     | 8      | Punto flotante precisión doble (SSE)              |
| bool      | `bool`       | 1      | `true` = 1, `false` = 0                           |
| unsigned  | `unsigned`   | 4      | Entero sin signo                                  |
| const     | `const`      | —      | Calificador de solo lectura                       |
| puntero   | `T*`         | 8      | `&x`, `*p`, `p->m`, `arr[i]` sobre puntero       |
| arreglo   | `T[n]`       | n×|T|  | Multidimensional: `int m[2][3]`                   |
| struct    | `struct`     | ∑      | Declaración: `struct Nombre { ... };`             |
| string    | n/a          | 8      | Literal: `"hola"` → `int` (dirección en .rodata) |

### 2.2 Scoping

| Nivel     | Entra con                                          | Contiene                      |
|-----------|----------------------------------------------------|-------------------------------|
| Global    | inicio del programa                                | funciones, structs, globales  |
| Función   | `fundec`                                           | parámetros + cuerpo           |
| Bloque    | `{` de `body`, `if`, `while`, `for`, `switch`      | variables locales             |

- Búsqueda: del scope más interno al más externo (shadowing permitido).
- No se permite redeclarar variable en el mismo scope.
- Funciones y structs en scope global; no se permite redefinirlos.

### 2.3 Reglas de tipado

| Constructo                     | Regla                                                              |
|--------------------------------|--------------------------------------------------------------------|
| Asignación `=`                 | Tipos compatibles (ver 2.4); promoción automática                  |
| Condición `if` `while` `for`   | Debe ser `bool`                                                    |
| `&&` `\|\|` `!`                | Operandos `bool`; resultado `bool`                                 |
| `==` `!=` `<` `>` `<=` `>=`    | Operandos `int`/`char`/`long`/`float`/`double`; resultado `bool`  |
| `+` `-` `*` `/` `%`           | `int`/`char`/`long`/`float`/`double`; promoción al más grande      |
| `**` potencia                  | `int`/`char`/`long`/`float`/`double`; exponente entero para nativo |
| `[]` indexación                | Base: arreglo o puntero; índice: `int` o `char`                    |
| `.` acceso miembro             | Objeto debe ser struct; miembro debe existir                       |
| `->` acceso flecha             | Operando debe ser puntero a struct                                 |
| Llamada a función              | Aridad y tipos coinciden con firma declarada                       |
| `return`                       | Tipo debe coincidir con tipo de retorno (o ser asignable)          |
| `break` / `continue`           | Solo dentro de `while`, `for`, `do-while` o `switch`               |
| `malloc` / `free`              | `malloc` retorna `void*`; `free` recibe `void*`                    |
| `sizeof`                       | Retorna `int` con el tamaño en bytes del tipo                      |
| `printf`                       | Acepta formato (string) y argumentos; usa convención variadic ABI  |

### 2.4 Conversiones y promociones

**Asignación** (`check_assign`): un valor de tipo `V` puede asignarse a destino `T` si:

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

**Aritmética**: el tipo del resultado se determina por el operando más grande:
- Si algún operando es `double` → resultado `double`
- Si no, si algún operando es `float` → resultado `float`
- Si no, si algún operando es `long` → resultado `long`
- Si no → resultado `int` (char promueve a int)

### 2.5 Strings

- Los literales de string (`"hola"`) se almacenan en la sección `.rodata`.
- El tipo semántico de un string es `int` (se trata como una dirección).
- `printf` acepta un string como primer argumento (formato) y args adicionales.
- No hay concatenación de strings ni mutación.

### 2.6 Manejo de errores

| Fase       | Estrategia                                                     |
|------------|----------------------------------------------------------------|
| Léxico     | Token `ERR` en caracteres no reconocidos → aborta compilación  |
| Sintáctico | `sync_error()` lanza excepción con token esperado vs encontrado|
| Semántico  | Errores acumulados en `TypeChecker::errors`; si ≥1 → `exit(1)` |

---

## 3. Arquitectura del compilador

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

### 3.1 Pipeline de compilación

1. **Scanner**: análisis léxico → tokens
2. **Parser**: análisis sintáctico → AST
3. **TypeChecker**: análisis semántico + resolución de tipos + asignación de offsets en stack frame
4. **ConstantFolding**: evaluación de expresiones constantes (implementado, deshabilitado por defecto en main.cpp)
5. **GenCodeVisitor**: generación de código x86-64

### 3.2 Asignación de offsets en stack frame

El TypeChecker asigna un slot de 8 bytes a cada variable <= 8 bytes
(char, int, float, double, puntero), y el tamaño real a las grandes
(arrays, structs > 8 bytes). Esto evita la complejidad del bin packing
sin penalización práctica en x86-64, donde el slot natural de pila es
de 8 bytes y las instrucciones pueden operar con tamaños parciales
(movb, movl, movq) sobre el mismo slot.

1. Cada variable recibe un slot de 8 bytes (o su tamaño real si > 8)
2. Los offsets se asignan secuencialmente desde 0
3. Frame size total = `(maxOffset + 15) & ~15` (alineado a 16 bytes)
4. Los offsets se convierten a negativos (relativos a %rbp)

---

## 4. Estructura del AST

### 4.1 Jerarquía de nodos

El AST usa **triple dispatch**: cada nodo implementa tres métodos `accept`:

| Visitor       | Clase Base        | Retorno Exp | Retorno Stm | Propósito                              |
|---------------|-------------------|-------------|-------------|----------------------------------------|
| `Visitor`     | `Visitor`         | `double`    | `int`       | Interpretación (`EVALVisitor`)         |
| `TypeVisitor` | `TypeVisitor`     | `Type*`     | `void`      | Type checking (`TypeChecker`)          |
| `CodeGenVisitor` | `CodeGenVisitor` | `void`      | `void`      | Code generation x86-64 (`GenCodeVisitor`) |

Además, cada nodo que puede ser l-value implementa `computeAddress(CodeGenVisitor*)`.

Las clases base del AST son:

- **`Exp`** — expresión (retorna `double` en EVAL, `Type*` en TypeChecker)
- **`Stm`** — sentencia (retorna `int` en EVAL, `void` en TypeChecker)
- **Nodos independientes** — `VarDecl`, `FunDecl`, `StructDecl`, `Program`

### 4.2 Árbol de relaciones

```
Program
├── FunDecl
│   ├── TypeNode (return_type)
│   ├── VarDecl* params
│   ├── frameSize (calculado por TypeChecker)
│   └── Body
│       └── Stm*
│
├── VarDecl (globals)
│   ├── TypeNode (type)
│   ├── array_sizes[]
│   ├── initializer (optional)
│   ├── resolvedType (asignado por TypeChecker)
│   ├── offset (asignado por TypeChecker, relativo a %rbp)
│   └── memSize (asignado por TypeChecker)
│
└── StructDecl
    ├── name
    ├── VarDecl* members
    ├── memberOffsets (calculados por TypeChecker)
    └── memberSizes (calculados por TypeChecker)

Statements (Stm)
├── Body ── stmts[]
├── ExprStmtNode ── expr (puede ser null: ;)
├── IfStmt ── condition, then_branch, [else_branch]
├── WhileStmt ── condition, body
├── DoWhileStmt ── body, condition
├── ForStmt ── init, condition, increment, body
├── SwitchStmt ── expr, cases[], default_body[]
├── CaseClause ── value, body[]
├── BreakStmt
├── ContinueStmt
├── ReturnStmt ── [expr]
├── FreeStmt ── expr
└── VarDecl (declaración local)

Expressions (Exp)
├── BinaryOpNode ── left, right, op
│   (ADD, SUB, MUL, DIV, MOD, EQ, NE, LT, GT, LE, GE, LOG_AND, LOG_OR, POW)
├── UnaryOpNode ── operand, op
│   (ADDR, DEREF, MINUS, LOG_NOT, PRE_INC, PRE_DEC, POST_INC, POST_DEC)
├── AssignmentNode ── target, value (ASSIGN)
├── FcallNode ── callee, args[]
├── IndexNode ── base, index
├── MemberAccessNode ── object, member
├── ArrowAccessNode ── pointer, member
├── MallocNode ── size
├── SizeOfNode ── target_type
├── PrintfNode ── format, args[]
├── IdentifierNode ── name, binding (VarDecl*)
├── IntegerLiteralNode ── value
├── FloatLiteralNode ── value
├── BoolLiteralNode ── value
├── CharLiteralNode ── value
├── StringLiteralNode ── value
└── TypeNode (base para tipos)
    ├── PrimitiveTypeNode (VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, LONG)
    ├── PointerTypeNode ── base
    └── StructTypeNode ── name

Type* semánticos (resolvedType)
├── Type (INT, CHAR, BOOL, FLOAT, DOUBLE, VOID, LONG)
├── PointerType ── base
├── ArrayType ── base, length
└── StructType ── name, members[]
```

La identificación de tipos de nodos en tiempo de ejecución se hace mediante
`dynamic_cast` (no se usa `enum class NodeKind`).

### 4.3 Lvalue handling

Para asignaciones y operadores `++`/`--`, el code generator distingue entre:

- **Rvalue** (`visit`) — evaluar una expresión, dejar el valor en `%rax`
- **Lvalue** — analizar la expresión como destino de escritura

El mecanismo usa la estructura `LVal` y dos métodos:

1. `captureLVal(Exp* e)`: recorre la expresión y llena un `LVal` con:
   - `kind`: tipo de l-value (`Id`, `Index`, `Member`, `Deref`)
   - `binding`: `VarDecl*` de la variable base
   - `indices[]`: índices para acceso a arreglo
   - `member` / `structName`: para acceso a struct

2. `storeTarget(LVal)`: genera el código para escribir `%rax` en la dirección:
   - `Id`: `storeBinding(vd)` → `movl %eax, offset(%rbp)`
   - `Index`: `emitIndexedStore()` → cálculo de índice + store
   - `Member`: `movl %ecx, offset(%rax)` (vía `leaBinding` o `loadBinding`)
   - `Deref`: `popq %rbx; movq %rax, (%rbx)`

Nodos que soportan l-value:

| Nodo                            | LValKind | Descripción                  |
|---------------------------------|----------|------------------------------|
| `IdentifierNode`                | `Id`     | Variable simple              |
| `IndexNode`                     | `Index`  | Arreglo unidimensional       |
| `IndexNode` (anidado)           | `Index`  | Multidimensional vía collect |
| `MemberAccessNode` (`.member`)  | `Member` | Campo de struct              |
| `ArrowAccessNode` (`->member`)  | `Member` | Campo vía puntero            |
| `UnaryOpNode` (DEREF)           | `Deref`  | `*ptr`                       |

### 4.4 Memoria

El AST usa **propiedad exclusiva** via raw pointers. `Program` es el nodo
raíz y es responsable de liberar todo el árbol: cada destructor elimina
sus hijos recursivamente.

---

## 5. Operadores

### 5.1 Operadores binarios

| Símbolo | Enum              | Precedencia | Asociatividad | Descripción                |
|---------|-------------------|-------------|---------------|----------------------------|
| `=`     | `ASSIGN`          | 1           | derecha       | Asignación                 |
| `\|\|`  | `LOG_OR`          | 2           | izquierda     | OR lógico                  |
| `&&`    | `LOG_AND`         | 3           | izquierda     | AND lógico                 |
| `==`    | `EQ`              | 4           | izquierda     | Igualdad                   |
| `!=`    | `NE`              | 4           | izquierda     | Desigualdad                |
| `<`     | `LT`              | 5           | izquierda     | Menor que                  |
| `>`     | `GT`              | 5           | izquierda     | Mayor que                  |
| `<=`    | `LE`              | 5           | izquierda     | Menor o igual              |
| `>=`    | `GE`              | 5           | izquierda     | Mayor o igual              |
| `+`     | `ADD`             | 6           | izquierda     | Suma                       |
| `-`     | `SUB`             | 6           | izquierda     | Resta                      |
| `*`     | `MUL`             | 7           | izquierda     | Multiplicación             |
| `/`     | `DIV`             | 7           | izquierda     | División entera            |
| `%`     | `MOD`             | 7           | izquierda     | Módulo                     |
| `**`    | `POW`             | 8           | derecha       | Potencia (exponenciación)  |

### 5.2 Operadores unarios

| Símbolo | Enum          | Descripción                        |
|---------|---------------|------------------------------------|
| `-`     | `MINUS`       | Negación aritmética                |
| `!`     | `LOG_NOT`     | Negación lógica                    |
| `&`     | `ADDR`        | Dirección de variable              |
| `*`     | `DEREF`       | Desreferencia de puntero           |
| `++x`   | `PRE_INC`     | Pre-incremento                     |
| `--x`   | `PRE_DEC`     | Pre-decremento                     |
| `x++`   | `POST_INC`    | Post-incremento                    |
| `x--`   | `POST_DEC`    | Post-decremento                    |

No hay operadores de asignación compuesta (`+=`, `-=`, etc.).
