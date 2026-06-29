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
expr        = assign_expr
assign_expr = unary_expr assign_op assign_expr
assign_op   = "="
lor_expr    = land_expr ("||" land_expr)*
land_expr   = eq_expr ("&&" eq_expr)*
eq_expr     = rel_expr { ("==" | "!=") rel_expr }
rel_expr    = add_expr { ("<" | ">" | "<=" | ">=") add_expr }
add_expr    = mul_expr { ("+" | "-") mul_expr }
mul_expr    = pow_expr { ("*" | "/" | "%") pow_expr }
pow_expr    = unary_expr [ "**" pow_expr ]
unary_expr  = post_expr | "++" unary_expr | "--" unary_expr | unary_op unary_expr
unary_op    = "&" | "*" | "-" | "!"
post_expr   = prim_expr { "[" expr "]" | "(" [ arg_list ] ")"
                        | "." ID | "->" ID | "++" | "--" }
arg_list    = assign_expr ("," assign_expr)*
prim_expr   = ID | const | "(" expr ")" | "malloc" "(" expr ")"
            | "sizeof" "(" type ")" | "printf" "(" arg_list ")" | lambda_expr

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
| Asignación `=`            | Tipos compatibles; `int → float` es promovido        |
| Condición `if` `while` `for` | Debe ser `bool`                              |
| `&&` `\|\|` `!`            | Operandos `bool`; resultado `bool`                   |
| `==` `!=` `<` `>` `<=` `>=` | Operandos `int` o `float`; resultado `bool`          |
| `+` `-` `*` `/` `%` `**`   | `int` o `float`; mixto → `float`                     |
| `[]` indexación            | Base: arreglo o puntero; índice: `int` o `char`      |
| `.` acceso miembro          | Objeto debe ser struct; miembro debe existir         |
| `->` acceso flecha          | Operando debe ser puntero a struct                   |
| Llamada a función          | Aridad y tipos coinciden con firma declarada         |
| `return`                   | Tipo debe coincidir con tipo de retorno              |
| `break` / `continue`       | Solo dentro de `while`, `for`, `do-while` o `switch` |
| `malloc` / `free`          | `malloc` retorna `void*`; `free` recibe `void*`      |

### 2.4 Conversiones

- **Promoción automática**: `int → float` en aritmética mixta y asignaciones.
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
    ├─► EVALVisitor ──► interpretación directa (prototipado)
    ├─► TypeChecker ──► verificación semántica (errores acumulativos)
    └─► GenCodeVisitor ──► código ensamblador x86-64 (AT&T/GAS)
```

---

## 4. Estructura del AST

### 4.1 Jerarquía de nodos

El AST usa **triple dispatch**: cada nodo implementa tres métodos `accept`:

| Jerarquía | Clase Base | Retorno Exp | Retorno Stm | Propósito |
|-----------|-----------|-------------|-------------|-----------|
| `Visitor` | `Visitor` | `double` | `int` | Interpretación (`EVALVisitor`) |
| `TypeVisitor` | `TypeVisitor` | `Type*` | `void` | Type checking (`TypeChecker`) |
| `CodeGenVisitor` | `CodeGenVisitor` | `void` | `void` | Code generation x86-64 (`GenCodeVisitor`) |

Las clases base del AST son:

- **`Exp`** — expresión (retorna `double` en EVAL, `Type*` en TypeChecker)
- **`Stm`** — sentencia (retorna `int` en EVAL, `void` en TypeChecker)
- **Nodos independientes** — `VarDecl`, `FunDecl`, `StructDecl`, `Program`, `TemplateDecl`

### 4.2 Árbol de relaciones

```
Program
├── FunDecl
│   ├── TypeNode (return_type: PrimitiveTypeNode / PointerTypeNode / StructTypeNode / ...)
│   ├── VarDecl* params
│   └── Body
│       └── Stm*
│
├── VarDecl (globals)
│   ├── TypeNode (type)
│   ├── array_sizes[] (Expression)
│   └── initializer (optional Expression)
│
├── StructDecl
│   ├── name
│   └── VarDecl* members
│
└── TemplateDecl
    ├── params[] (strings: nombres de parámetros de tipo)
    └── FunDecl / StructDecl (patrón)

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
├── BinaryOpNode ── left, right (ADD, SUB, MUL, DIV, MOD, EQ, NE, LT, GT, LE, GE, LOG_AND, LOG_OR, POW)
├── UnaryOpNode ── operand (ADDR, DEREF, MINUS, LOG_NOT, PRE_INC, PRE_DEC, POST_INC, POST_DEC)
├── AssignmentNode ── target, value (ASSIGN)
├── FcallNode ── callee, args[], template_args[]
├── IndexNode ── base, index
├── MemberAccessNode ── object, member
├── ArrowAccessNode ── pointer, member
├── MallocNode ── size
├── SizeOfNode ── target_type
├── PrintfNode ── args[]
├── LambdaExprNode ── captures[], params[], return_type, body
├── CaptureNode ── mode (BY_VALUE / BY_REF), name
├── IdentifierNode ── name
├── IntegerLiteralNode ── value
├── FloatLiteralNode ── value
├── BoolLiteralNode ── value
├── CharLiteralNode ── value
├── StringLiteralNode ── value
└── TypeNode (base para tipos)
    ├── PrimitiveTypeNode ── prim (VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, AUTO)
    ├── PointerTypeNode ── base (TypeNode)
    ├── StructTypeNode ── name
    ├── NamedTypeNode ── name (placeholder para tipos no resueltos: params de template)
    └── TemplateTypeNode ── name, type_args[] (instanciación concreta: vector<int>)
```

La identificación de tipos de nodos en tiempo de ejecución se hace mediante
`dynamic_cast` (no se usa `enum class NodeKind`).

### 4.3 Lvalue handling (`computeAddress`)

Para generación de código, la asignación requiere distinguir entre:

- **Rvalue** (`visit`) — evaluar una expresión, dejar el valor en `%rax`
- **Lvalue** (`computeAddress`) — calcular la dirección efectiva de un lvalue y dejarla en `%rbx` para almacenar

El método `computeAddress` está declarado en `Exp` con una implementación
default que lanza error ("not an lvalue"). Los siguientes nodos lo overriddean:

| Nodo | Descripción |
|------|-------------|
| `UnaryOpNode` (solo DEREF) | Dirección del apuntado (`*ptr`) |
| `IdentifierNode` | Dirección de variable (`x`) |
| `IndexNode` | Dirección de elemento de array (`arr[i]`) |
| `MemberAccessNode` | Dirección de miembro de struct (`s.m`) |
| `ArrowAccessNode` | Dirección de miembro vía puntero (`p->m`) |

Patrón de uso en asignación simple (`=`):

```
visit(RHS)             → valor en %rax
target->computeAddress(this) → dirección en %rbx
emit "movq %rax, (%rbx)"     → almacenar
```

### 4.4 Memoria

El AST usa **propiedad exclusiva** via raw pointers. `Program` es el nodo
raíz y es responsable de liberar todo el árbol: cada destructor elimina
sus hijos recursivamente.
