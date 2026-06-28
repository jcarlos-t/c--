# Code Generation Guidelines (x86-64)

Basado en el approach usado en `base_lab/Labactual/visitor.cpp` (`GenCodeVisitor`).

## 1. Arquitectura

```
class GenCodeVisitor : public CodeGenVisitor {
protected:
    ostream &out;                     // flujo de salida (.s)
    int labelcont = 0;               // contador para labels únicas
    int offset = -8;                 // offset actual en el stack frame
    unordered_map<string, int> memoria;        // variable local → %rbp offset
    unordered_map<string, bool> memoriaGlobal; // variable → está en .data?
    string nombreFuncion;
    string currentBreakLabel;                 // label de break (switch/while)
    string currentContinueLabel;              // label de continue (while/for)
    bool dentroDeFuncion = false;
    unordered_map<string, int> funFrameSize;  // de TypeChecker: nombre → slot count

public:
    GenCodeVisitor(ostream &o);
    void generar(Program *p);
    // ... override de todos los visit() y computeAddress() ...
};
```

### Flujo de compilación

```
main.cpp:
  GenCodeVisitor codegen(outfile);
  codegen.generar(ast);

generar():
  1. TypeChecker tc; tc.typecheck(program)   // análisis semántico + frame sizes
  2. funFrameSize = tc.funFrameSize           // copiar info de frames
  3. program->accept(this)                    // generar código
```

### Salida assembly (AT&T)

```asm
.data
print_fmt: .string "%ld \n"
globalVar1: .quad 0

.text
.globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $N, %rsp              # N = frameSize * 8

    # ... cuerpo de la función ...

.end_main:
    leave
    ret

.section .note.GNU-stack,"",@progbits
```

## 2. Stack Frame Layout

```
                     Direcciones altas
       +----------------------------+
       | argumentos (stack)         |
       +----------------------------+
       | dirección de retorno       |
       +----------------------------+
  %rbp | %rbp guardado              |
       +----------------------------+
       | param1          (-8)       |
       +----------------------------+
       | param2          (-16)      |
       +----------------------------+
       | local1          (-24)      |
       +----------------------------+
       | local2          (-32)      |
       +----------------------------+
  %rsp | espacio reservado          |
       +----------------------------+
                     Direcciones bajas
```

## 3. Parámetros y registros (System V AMD64 ABI)

| Posición | Registro |
|----------|----------|
| arg 1 | `%rdi` |
| arg 2 | `%rsi` |
| arg 3 | `%rdx` |
| arg 4 | `%rcx` |
| arg 5 | `%r8` |
| arg 6 | `%r9` |
| resto | stack |

### Convenciones de registros

| Registro | Uso |
|----------|-----|
| `%rax` | Resultado de expresiones (rvalue) |
| `%rbx` | Dirección efectiva de lvalue |
| `%r10` | Comparaciones en switch |
| `%rdi`, `%rsi`, etc. | Argumentos de llamadas |
| `%rbp` | Base pointer (frame) |
| `%rsp` | Stack pointer |

## 4. Patrones de generación

### 4.1 Expresiones aritméticas (binarias)

```cpp
// left + right
visit(left)        → %rax
pushq %rax
visit(right)       → %rax
movq %rax, %rcx
popq %rax
addq %rcx, %rax
```

### 4.2 Asignación simple (`=`)

```cpp
// x = expr
visit(expr)                  → %rax
target->computeAddress(this) → %rbx  (dirección de x)
movq %rax, (%rbx)
```

### 4.3 Asignación compuesta (`+=`)

```cpp
// x += expr
target->computeAddress(this) → %rbx  (dirección de x)
movq (%rbx), %rax                    (cargar valor actual)
pushq %rax
visit(expr)                   → %rax
movq %rax, %rcx
popq %rax
addq %rcx, %rax
movq %rax, (%rbx)                    (almacenar resultado)
```

### 4.4 Llamada a función

```cpp
// f(a, b, c)
visit(a) → %rax
movq %rax, %rdi
visit(b) → %rax
movq %rax, %rsi
visit(c) → %rax
movq %rax, %rdx
call f
```

### 4.5 Control de flujo: If-else

```asm
    visit(condition)        → %rax
    cmpq $0, %rax
    je else_L
    # then branch
    jmp endif_L
else_L:
    # else branch (si existe)
endif_L:
```

### 4.6 Control de flujo: While

```asm
while_L:
    visit(condition)        → %rax
    cmpq $0, %rax
    je endwhile_L
    # body
    jmp while_L
endwhile_L:
```

### 4.7 Control de flujo: Do-While

```asm
dowhile_L:
    # body
    visit(condition)        → %rax
    cmpq $0, %rax
    jne dowhile_L           # loop si condition != 0
```

### 4.8 Control de flujo: Switch

```asm
    visit(expr)             → %rax
    movq %rax, %r10
    # case values...
    movq $1, %rax
    cmpq %rax, %r10
    je case_L_1
    # ... más cases ...
    jmp endswitch_L         # o default_L
case_L_1:
    # body case 1
    jmp endswitch_L
    # ...
endswitch_L:
```

### 4.9 Break / Continue

- `break`: `jmp <currentBreakLabel>` (label de fin del loop/switch)
- `continue`: `jmp <currentContinueLabel>` (label de condición del loop)

### 4.10 Return

```cpp
visit(expr) → %rax
jmp .end_funcName
```

El epílogo de función (`leave; ret`) se emite en `.end_funcName:`.

### 4.11 Print

```asm
visit(expr) → %rax
movq %rax, %rsi
leaq print_fmt(%rip), %rdi
xorq %rax, %rax
call printf@PLT
```

### 4.12 Malloc

```asm
visit(size_expr) → %rax
# tamaño en bytes (ej: 8 * count para array de 8 bytes)
movq %rax, %rdi
call malloc@PLT
# resultado (puntero) queda en %rax
```

## 5. Manejo de tipos y tamaños

El `TypeChecker` resuelve los tipos de cada expresión. El `CodeGenVisitor` necesita consultar el tipo para:

| Tipo | Tamaño | Instrucción de store |
|------|--------|---------------------|
| `char`, `bool` | 1 byte | `movb %al, (%rbx)` |
| `int`, `float` | 4 bytes | `movl %eax, (%rbx)` |
| `double` | 8 bytes | `movsd %xmm0, (%rbx)` / registro XMM |
| `pointer` | 8 bytes | `movq %rax, (%rbx)` |
| `struct` | variable | `mov` miembro por miembro |

Usar `Exp::resolved_type` (campo opcional Type* que el TypeChecker llena al visitar cada nodo) para determinar el tamaño y el tipo de registro.

## 6. Frame Size Computation (TypeChecker)

El TypeChecker actual debe extenderse para llevar la cuenta de cuántos slots (variables locales + parámetros) necesita cada función. Esto se logra con:

```cpp
// En TypeChecker (o un visitor separado):
unordered_map<string, int> funFrameSize;

// En visit(FunDecl*):
//  - Registrar nombre de función
//  - Contar parámetros: slots = params.size()
//  - Abrir scope, visitar body, cerrar scope
//  - funFrameSize[name] = slots + locales

// En visit(VarDecl*):
//  - Por cada variable declarada: slots++

// En visit(Body*):
//  - Abrir scope, visitar, cerrar scope (no incrementa slots porque
//    VarDecl ya los cuenta; pero los scopes anidados necesitan reset
//    de contador local si se quiere optimizar como en base_lab)
```

El `CodeGenVisitor` usa `funFrameSize[nombre]` para la instrucción:

```asm
subq $funFrameSize[nombre] * 8, %rsp
```

## 7. Variables globales

En `visit(Program*)`:
1. Recorrer `p->globals` → emitir en `.data` como `nombre: .quad 0`
2. Registrar nombres en `memoriaGlobal`
3. Visitar funciones

En `visit(VarDecl*)`:
- Si `dentroDeFuncion == false` → registrar en `memoriaGlobal` (no emitir código, ya se emitió en Program)
- Si `dentroDeFuncion == true` → asignar offset: `memoria[nombre] = offset; offset -= 8`

## 8. Labels únicas

Usar `labelcont` para generar nombres únicos:

```cpp
string label = "while_" + to_string(labelcont++);
string endlabel = "endwhile_" + to_string(labelcont++);
```

Para break/continue, guardar el label correspondiente antes de visitar el cuerpo:

```cpp
string oldBreak = currentBreakLabel;
string oldContinue = currentContinueLabel;
currentBreakLabel = endLabel;    // adonde salta break
currentContinueLabel = condLabel; // adonde salta continue
body->accept(this);
currentBreakLabel = oldBreak;
currentContinueLabel = oldContinue;
```

## 9. Referencias

- `base_lab/Labactual/visitor.cpp` — implementación de referencia (GenCodeVisitor, 819 líneas)
- `docs/lenguaje.md` (sección 4) — documentación del AST y los visitors
- System V AMD64 ABI — convención de llamada
