# C-- — Especificación del Lenguaje

Subset de C con punteros, templates, lambdas y memoria dinámica.
Compilado a x86-64 (AT&T/GAS) vía recursive descent + triple dispatch.

---

## 1. Gramática

```
program     = decl*

// declaraciones
decl        = fundec | vardec | struct_decl | templ_decl

fundec      = type ID "(" param_list ")" body
vardec      = type ID arrsuf [ "=" expr ] ";"
struct_decl = "struct" ID "{" vardec* "}" ";"

// tipos
type        = btype "*"*
btype       = "void" | "int" | "char" | "float" | "double" | "bool" | "auto"
            | stype
stype       = "struct" ID
arrsuf      = ("[" [ expr ] "]")*

// parametros
param_list  = param ("," param)* | ε
param       = type ID arrsuf

// statements
stmt        = body | expr_stmt | sel_stmt | iter_stmt | jump_stmt | free_stmt | vardec

body        = "{" stmt* "}"
expr_stmt   = [ expr ] ";"

sel_stmt    = if_stmt | switch_stmt
if_stmt     = "if" "(" expr ")" stmt [ "else" stmt ]
switch_stmt = "switch" "(" expr ")" "{" { case_clause | default_clause } "}"
case_clause = "case" expr ":" stmt*
default_clause = "default" ":" stmt*

iter_stmt   = while_stmt | do_stmt | for_stmt
while_stmt  = "while" "(" expr ")" stmt
do_stmt     = "do" stmt "while" "(" expr ")" ";"
for_stmt    = "for" "(" [ init ] ";" [ cond ] ";" [ inc ] ")" stmt
init        = expr | vardec
cond        = expr
inc         = expr

jump_stmt   = break_stmt | cont_stmt | ret_stmt
break_stmt  = "break" ";"
cont_stmt   = "continue" ";"
ret_stmt    = "return" [ expr ] ";"
free_stmt   = "free" "(" expr ")" ";"

// expresiones
expr        = assign_expr ("," assign_expr)*
assign_expr = cond_expr | unary_expr assign_op assign_expr
assign_op   = "=" | "+=" | "-=" | "*=" | "/="
cond_expr   = lor_expr | lor_expr "?" expr ":" cond_expr
lor_expr    = land_expr ("||" land_expr)*
land_expr   = eq_expr ("&&" eq_expr)*
eq_expr     = rel_expr { ("==" | "!=") rel_expr }
rel_expr    = add_expr { ("<" | ">" | "<=" | ">=") add_expr }
add_expr    = mul_expr { ("+" | "-") mul_expr }
mul_expr    = pow_expr { ("*" | "/" | "%") pow_expr }
pow_expr    = cast_expr [ "**" pow_expr ]
cast_expr   = unary_expr | "(" type ")" cast_expr
unary_expr  = post_expr | "++" unary_expr | "--" unary_expr | unary_op cast_expr
unary_op    = "&" | "*" | "-" | "!"
post_expr   = prim_expr { "[" expr "]" | "(" [ arg_list ] ")"
                        | "." ID | "->" ID | "++" | "--" }
arg_list    = assign_expr ("," assign_expr)*
prim_expr   = ID | const | "(" expr ")" | "malloc" "(" expr ")"
            | "sizeof" "(" type ")" | lambda_expr

// constantes
const       = intc | floatc | charc | boolc | str
intc        = NUM
floatc      = FNUM
charc       = "'" CHAR "'"
boolc       = "true" | "false"
str         = '"' CHAR* '"'

// lambdas
lambda_expr = "[" [ cap_list ] "]" "(" param_list ")" "->" type body
cap_list    = cap ("," cap)*
cap         = "=" | "&" | ID | "&" ID

// templates
templ_decl  = "template" "<" tparam_list ">" ( fundec | struct_decl )
tparam_list = "typename" ID ("," "typename" ID)*

// tokens lexicos
ID          = letter { letter | digit | "_" }
NUM         = digit digit*
FNUM        = digit digit* "." digit digit*
              [ ("e" | "E") [ "+" | "-" ] digit digit* ]
CHAR        = printable_ascii | escape_sequence
escape_sequence = "\\" ( "'" | '"' | "\\" | "n" | "t" | "r" | "0" )
letter      = "A".."Z" | "a".."z"
digit       = "0".."9"
```

---

## 2. Semántica

### 2.1 Sistema de tipos

| Tipo      | Palabra | Tamaño | Observaciones                           |
|-----------|---------|--------|-----------------------------------------|
| void      | `void`  | —      | Solo como retorno de función            |
| int       | `int`   | 4      | Entero con signo                        |
| char      | `char`  | 1      | Tratado como int en aritmética          |
| float     | `float` | 4      | Compatible con double en operaciones    |
| double    | `double`| 8      | Equivalente semántico a float           |
| bool      | `bool`  | 1      | `true` = 1.0, `false` = 0.0             |
| auto      | `auto`  | —      | Inferido del inicializador              |
| puntero   | `T*`    | 8      | `&x`, `*p`, `p->m`                     |
| struct    | `struct`| ∑      | Declaración: `struct Nombre { ... };`   |

### 2.2 Scoping

| Nivel     | Entra con                                          | Contiene                      |
|-----------|----------------------------------------------------|-------------------------------|
| Global    | inicio del programa                                | funciones, structs, globales  |
| Función   | `fundec`                             | parámetros + cuerpo           |
| Bloque    | `{` de `body`, `if`, `while`, `for`, `switch` | variables locales  |

- Búsqueda: del scope más interno al más externo (shadowing permitido).
- No se permite redeclarar variable en el mismo scope.
- Funciones y structs en scope global; no se permite redefinirlos.

### 2.3 Reglas de tipado

| Constructo                 | Regla                                                |
|----------------------------|------------------------------------------------------|
| Asignación `=` `+=` etc.   | Tipos compatibles; `int → float` es promovido        |
| Condición `if` `while` `for` `?:` | Debe ser `bool`                              |
| `&&` `\|\|` `!`            | Operandos `bool`; resultado `bool`                   |
| `==` `!=` `<` `>` `<=` `>=` | Operandos `int` o `float`; resultado `bool`          |
| `+` `-` `*` `/` `%` `**`   | `int` o `float`; mixto → `float`                     |
| `[]` indexación            | Base: arreglo o puntero; índice: `int` o `char`      |
| `.` acceso miembro          | Objeto debe ser struct; miembro debe existir         |
| `->` acceso flecha          | Operando debe ser puntero a struct                   |
| Llamada a función          | Aridad y tipos coinciden con firma declarada         |
| `return`                   | Tipo debe coincidir con tipo de retorno              |
| `break` / `continue`       | Solo dentro de `while`, `for`, `do-while` o `switch` |

### 2.4 Conversiones

- **Promoción automática**: `int → float` en aritmética mixta y asignaciones.
- **Promoción ternaria**: si una rama es `float` y otra `int`, resultado `float`.
- **Cast explícito**: `(tipo) expr` convierte al tipo indicado.
- **`sizeof`**: retorna `int` con el tamaño en bytes del tipo.

### 2.5 Manejo de errores

| Fase       | Estrategia                                                       |
|------------|------------------------------------------------------------------|
| Léxico     | Token `ERR` en caracteres no reconocidos → aborta compilación    |
| Sintáctico | `sync_error()` lanza excepción con token esperado vs encontrado  |
| Semántico  | Errores acumulados en `TypeChecker::errors`; si ≥1 → `exit(1)`   |

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
    ├─► PrintVisitor ──► pretty-print del AST (debug)
    ├─► EVALVisitor ──► interpretación directa (prototipado)
    ├─► TypeChecker ──► verificación semántica (errores acumulativos)
    └─► CodeGenVisitor ──► código ensamblador x86-64 (AT&T/GAS)
```

El AST usa **triple dispatch**: cada nodo implementa `accept(Visitor*)`,
`accept(TypeVisitor*)` y `accept(CodeGenVisitor*)`. La jerarquía de nodos es
`Exp` (expresiones) y `Stm` (sentencias), con `VarDecl`, `FunDecl`, `StructDecl`,
`Program`, `TemplateDecl` como nodos de declaración independientes.
