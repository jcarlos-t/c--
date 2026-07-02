# Compilador C--: Flujos de Implementación

Este documento explica cómo funcionan las características principales del compilador C--, con énfasis en los flujos de `TypeChecker` (análisis semántico) y `GenCodeVisitor` (generación de código ensamblador).

---

## 1. Tipos Básicos

### 1.1 `char`

#### TypeChecker
- Representación semántica: `charType` (singleton de `Type` con `ttype = CHAR`)
- Tamaño: 1 byte
- Funcionamiento: `char` se promueve automáticamente a `int` en expresiones aritméticas (como en C estándar)

**Flujo de resolución de tipo:**
```cpp
// Código fuente: char c = 'A';
// AST: VarDecl{ type: PrimitiveTypeNode(CHAR), name: "c" }

Type* TypeChecker::type_from_ast(Exp* t) {
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(t)) {
        if (pt->prim == PrimitiveTypeNode::CHAR) {
            return charType;  // singleton precreado
        }
    }
}
```

**Ejemplo de validación:**
```c
char c1 = 'A';     // ok
char c2 = 65;      // ok (entero → char, truncamiento implícito)
char c3 = 3.14;    // error (no hay conversión float → char implícita)
```

#### GenCodeVisitor
- Carga/almacenamiento: Usa `movb` para byte
- En expresiones: Se extiende a entero de 32 bits (`movsbl` para signo, `movzbl` para cero)

**Ejemplo de generación de código:**
```asm
; char c = 'A';
movb $65, -8(%rbp)     ; almacenar 'A' (ASCII 65) en stack frame

; int x = c;            ; char → int
movsbl -8(%rbp), %eax  ; extender signo de byte a dword
movl %eax, -16(%rbp)   ; almacenar en x
```

---

### 1.2 `float` y `double`

#### TypeChecker
- Representaciones: `floatType` (4 bytes) y `doubleType` (8 bytes)
- Conversiones:
  - `int/char/long` → `float` o `double` (promoción implícita)
  - `float` → `double` (promoción implícita)

**Flujo de validación en operaciones binarias:**
```cpp
void TypeChecker::visit(BinaryOpNode* e) {
    e->left->accept(this);
    e->right->accept(this);
    Type* lt = e->left->resolvedType;
    Type* rt = e->right->resolvedType;
    
    // Promoción entre tipos numéricos
    if (is_arithmetic_type(lt) && is_arithmetic_type(rt)) {
        // float + double → double
        if (lt->ttype == DOUBLE || rt->ttype == DOUBLE) {
            e->resolvedType = doubleType;
        }
        // float + [int/char/long] → float
        else if (lt->ttype == FLOAT || rt->ttype == FLOAT) {
            e->resolvedType = floatType;
        }
    }
}
```

**Ejemplo:**
```c
float f = 3.14;      // ok
double d = f;        // ok (float → double implícito)
double d2 = 100;     // ok (int → double implícito)
```

#### GenCodeVisitor
- Registros: Usa registros SSE (`%xmm0`, `%xmm1`, etc.)
- Instrucciones:
  - `movss` (float simple)
  - `movsd` (double precisión)
  - `addss`, `subss`, `mulss`, `divss` (para floats)
  - `addsd`, `subsd`, `mulsd`, `divsd` (para doubles)

**Ejemplo de generación de código:**
```asm
; float f = 3.14;
movss .LC0(%rip), %xmm0  ; cargar literal float
movss %xmm0, -8(%rbp)    ; almacenar en f

; double d = 2.718;
movsd .LC1(%rip), %xmm0  ; cargar literal double
movsd %xmm0, -16(%rbp)   ; almacenar en d

; float suma = f + 5.0;
movss -8(%rbp), %xmm0    ; cargar f
cvtss2sd %xmm0, %xmm0    ; convertir float a double
movsd .LC2(%rip), %xmm1  ; cargar 5.0
addsd %xmm1, %xmm0       ; sumar
cvtsd2ss %xmm0, %xmm0    ; convertir double a float
movss %xmm0, -24(%rbp)   ; almacenar en suma
```

---

### 1.3 `long long`

#### TypeChecker
- Representación: `longType` (tamaño: 8 bytes)
- Promociones: `char` → `int` → `long`
- Conversiones: Puede convertir de/para `float` y `double`

**Flujo en type_from_ast:**
```cpp
// Código fuente: long long ll = 100LL;
// AST: PrimitiveTypeNode(LONG)

// Parser reconoce "long long" como LONG
PrimitiveTypeNode* Parser::parse_basic_type() {
    bool sawLong = false;
    while (check(Token::LONG)) {
        sawLong = true;
        advance();
    }
    if (sawLong) {
        return new PrimitiveTypeNode(PrimitiveTypeNode::LONG);
    }
}
```

**Ejemplo:**
```c
long long ll1 = 42;          // ok (int → long)
long long ll2 = 3.14;        // error (no hay float → long implícita)
```

#### GenCodeVisitor
- Registros de 64 bits: `%rax`, `%rbx`, `%rcx`, `%rdx`, etc.
- Tamaño: 8 bytes
- Instrucciones: `movq` (mover quadword), `addq`, `subq`, `imulq`, `idivq`

**Ejemplo:**
```asm
; long long ll = 123456789;
movabsq $123456789, %rax  ; cargar valor de 64 bits
movq %rax, -16(%rbp)      ; almacenar en ll

; long long suma = ll + 100;
movq -16(%rbp), %rax      ; cargar ll
addq $100, %rax           ; sumar
movq %rax, -24(%rbp)      ; almacenar en suma
```

---

### 1.4 `void`

#### TypeChecker
- Representación: `voidType` (no tiene tamaño definido)
- Restricciones:
  - No se puede declarar una variable de tipo `void`
  - Solo se usa como tipo de retorno de funciones

**Validación en visit(VarDecl):**
```cpp
void TypeChecker::visit(VarDecl* v) {
    Type* t = type_from_ast(v->type);
    if (t->match(voidType)) {
        error("variable no puede ser de tipo void.");
        return;
    }
    v->resolvedType = t;
}
```

**Validación en visit(FunDecl):**
```cpp
void TypeChecker::visit(FunDecl* f) {
    Type* rt = type_from_ast(f->return_type);
    retornodefuncion = rt;  // puede ser void
    
    // Verificar que no haya parámetros de tipo void
    for (auto p : f->params) {
        Type* pt = type_from_ast(p->type);
        if (pt->match(voidType)) {
            error("parámetro no puede ser de tipo void.");
        }
    }
}
```

**Ejemplo:**
```c
void func() {}           // ok
void bad(int x) {}       // ok (parámetro int, no void)
void bad() {
    void x;              // error: variable void
}
```

#### GenCodeVisitor
- Si una función retorna `void`, el `return` no tiene valor asociado
- No se genera código para mover valor a `%eax`

**Ejemplo:**
```asm
; void saludar() { printf("Hola\n"); }
saludar:
    pushq %rbp
    movq %rsp, %rbp
    movl $.LC0, %edi    ; "Hola\n"
    call printf
    nop
    popq %rbp
    ret                 ; no hay mov a %eax
```

---

## 2. Modificadores de Tipo

### 2.1 `const`

#### TypeChecker
- Funcionamiento: Marca una variable como de solo lectura
- Restricción: No se puede asignar a una variable `const` después de su declaración

**Flujo de implementación:**
1. `Parser` detecta `const` y lo marca en `TypeNode` y `VarDecl`
2. `TypeChecker` copia la bandera `isConst` al `Type*` semántico
3. En la asignación, verifica que el tipo de destino no sea `const`

**Validación en visit(AssignmentNode):**
```cpp
void TypeChecker::visit(AssignmentNode* e) {
    isLvalContext = true;
    e->target->accept(this);
    Type* targetType = e->target->resolvedType;
    isLvalContext = false;
    
    // Verificar que no sea const
    if (targetType->isConst) {
        error("no se puede asignar a una variable declarada como const.");
    }
    
    // Continuar con la validación normal
    e->value->accept(this);
    if (!check_assign(targetType, e->value->resolvedType)) {
        error("tipos incompatibles en asignación.");
    }
}
```

**Ejemplo:**
```c
const int x = 5;        // ok (inicialización permitida)
int y = x;              // ok (lectura permitida)
x = 10;                 // error: no se puede asignar a const
```

#### GenCodeVisitor
- `const` es solo un check de tiempo de compilación
- No genera código diferenciado en ensamblador
- Las variables `const` se tratan igual que las no-const en memoria

---

### 2.2 `unsigned`

#### TypeChecker
- Representación: `Type` con la bandera `isUnsigned = true`
- Funcionamiento: Indica que el entero no tiene signo
- Diferencia principal: Comparaciones y división/multiplicación usan instrucciones diferentes

**Flujo de type_from_ast con unsigned:**
```cpp
Type* TypeChecker::type_from_ast(Exp* t) {
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(t)) {
        Type* base = nullptr;
        switch (pt->prim) {
            case PrimitiveTypeNode::INT: base = intType; break;
            case PrimitiveTypeNode::LONG: base = longType; break;
            // ...
        }
        
        if (base && pt->isUnsigned) {
            // Clonar el tipo para establecer la bandera
            Type* clone = new Type(base->ttype);
            clone->isUnsigned = true;
            typeCache.push_back(clone);
            return clone;
        }
        
        return base;
    }
}
```

**Ejemplo:**
```c
unsigned int x = 100;   // ok
unsigned long y = x;    // ok
```

#### GenCodeVisitor
- Para operaciones aritméticas:
  - `mul` (unsigned multiply) en lugar de `imul`
  - `div` (unsigned divide) en lugar de `idiv`
- Para comparaciones:
  - `setb` (below), `seta` (above), `setbe`, `setae` (unsigned)
  - En lugar de `setl`, `setg`, `setle`, `setge` (signed)

**Ejemplo de código generado:**
```asm
; unsigned int a = 10, b = 20;
; if (a < b) { ... }

movl -8(%rbp), %eax     ; cargar a
cmpl -16(%rbp), %eax    ; comparar con b
setb %al                ; setear si es menor (unsigned)
movzbl %al, %eax
testl %eax, %eax
je .L1
; ... then branch
.L1:
; ... else branch
```

---

## 3. Estructuras de Control

### 3.1 `for`

#### TypeChecker
- Sintaxis: `for (init; cond; incr) body`
- Todos los componentes son opcionales
- Funcionamiento:
  1. El componente `init` puede ser una declaración de variable (con scope propio del for)
  2. Se crea un nuevo scope para el cuerpo del for
  3. La condición debe ser convertible a `bool`

**Flujo en visit(ForStmt):**
```cpp
void TypeChecker::visit(ForStmt* f) {
    env.add_level();      // scope para variable de init
    varEnv.add_level();
    
    // Procesar init
    if (f->init) f->init->accept(this);
    
    // Procesar condición (si existe)
    if (f->cond) {
        f->cond->accept(this);
        if (!is_integral_type(f->cond->resolvedType)) {
            error("condición de for debe ser de tipo entero.");
        }
    }
    
    // Procesar incremento (si existe)
    if (f->incr) f->incr->accept(this);
    
    // Procesar body (scope propio)
    env.add_level();
    varEnv.add_level();
    if (f->body) f->body->accept(this);
    varEnv.remove_level();
    env.remove_level();
    
    varEnv.remove_level();
    env.remove_level();
}
```

**Ejemplo:**
```c
for (int i = 0; i < 10; i++) {  // ok
    printf("%d", i);
}
```

#### GenCodeVisitor
- Etiquetas generadas:
  - `for_cond`: Verificar la condición
  - `for_body`: Cuerpo del ciclo
  - `for_incr`: Incremento
  - `for_end`: Fin del ciclo

**Flujo de generación:**
1. Generar init
2. Saltar a `for_cond`
3. En `for_cond`: Evaluar condición, si falsa saltar a `for_end`
4. En `for_body`: Ejecutar cuerpo
5. Saltar a `for_incr`
6. En `for_incr`: Ejecutar incremento
7. Saltar a `for_cond`
8. En `for_end`: Continuar

**Ejemplo de código generado:**
```asm
; for (int i = 0; i < 10; i++) { printf("%d", i); }
    movl $0, -8(%rbp)      ; init: i = 0
    jmp for_cond
for_body:
    movl -8(%rbp), %edi    ; cargar i para printf
    call printf
for_incr:
    incl -8(%rbp)          ; i++
for_cond:
    cmpl $9, -8(%rbp)      ; i < 10
    jle for_body           ; si menor o igual, ir a body
for_end:
    ; fin del for
```

---

## 4. Cadenas y Arreglos

### 4.1 Strings (Basados en `char[]`)

#### TypeChecker
- Las cadenas literales (`"hola"`) se tratan como `char*`
- Almacenamiento: Se guardan en la sección `.rodata` (solo lectura)
- No hay tipo `string` como tal; es solo un puntero a `char`

**Flujo de manejo de literales de cadena:**
```cpp
// Parser: reconoce tokens STRING y crea un StringLiteralNode
Exp* Parser::parse_primary() {
    if (check(Token::STRING)) {
        return new StringLiteralNode(current->text);
    }
}

// TypeChecker: el tipo de una cadena literal es char*
void TypeChecker::visit(StringLiteralNode* e) {
    e->resolvedType = new PointerType(charType);
    typeCache.push_back(e->resolvedType);
}
```

**Ejemplo:**
```c
char* str = "Hola mundo";  // ok: "Hola mundo" es char*
printf("%s", str);         // ok: %s espera char*
```

#### GenCodeVisitor
- Literales de cadena se colocan en la sección `.rodata`
- Cada literal se asocia a una etiqueta única

**Generación de literales en la sección de datos:**
```asm
.section .rodata
.LC0:
    .string "Hola mundo"
.LC1:
    .string "%d"
```

**Uso en la sección de texto:**
```asm
; char* str = "Hola mundo";
movq $.LC0, %rax          ; cargar dirección de la cadena
movq %rax, -8(%rbp)       ; almacenar en str

; printf("%s", str);
movq -8(%rbp), %rsi       ; str es segundo argumento (rsi)
movl $.LC1, %edi          ; "%s" es primer argumento (edi)
movl $0, %eax
call printf
```

---

### 4.2 Arreglos

#### TypeChecker
- Tipos de arreglo: `ArrayType{base: Type*, length: int}`
- Declaración: `int arr[5];`
- Multi-dimensional: `int mat[3][4];` (arreglo de arreglos)
- En expresiones: Un arreglo se convierte implícitamente a puntero a su primer elemento (decaimiento)

**Flujo en visit(VarDecl):**
```cpp
void TypeChecker::visit(VarDecl* v) {
    Type* t = type_from_ast(v->type);
    
    // Aplicar dimensiones de arreglos
    for (auto size_exp : v->array_sizes) {
        int len = -1;
        if (auto* il = dynamic_cast<IntegerLiteralNode*>(size_exp)) {
            len = (int)il->value;
        }
        ArrayType* at = new ArrayType(t, len);
        typeCache.push_back(at);
        t = at;
    }
    
    v->resolvedType = t;
}
```

**Ejemplo:**
```c
int arr[5];           // ArrayType{base: intType, length: 5}
int mat[3][4];        // ArrayType{base: ArrayType{intType, 4}, length: 3}
int x = arr[2];       // ok: arr decae a int*, arr[2] es int
```

#### GenCodeVisitor
- Aritmética de punteros para acceso por índice: `base + index * size_of_base`
- Para arreglos multidimensionales: `base + (i*N + j) * size`

**Ejemplo de acceso a arreglo unidimensional:**
```asm
; int arr[5]; int x = arr[2];
    ; arr está en -24(%rbp)
    movl -24(%rbp, %rcx, 4), %eax  ; base + (index * 4)
    movl %eax, -4(%rbp)            ; x = arr[2]
```

**Ejemplo de acceso a arreglo multidimensional:**
```asm
; int mat[3][4]; int y = mat[i][j];
    ; i está en %rax, j en %rcx
    imulq $4, %rax, %rdx      ; i * 4
    addq %rcx, %rdx           ; i*4 + j
    movl -48(%rbp, %rdx, 4), %eax  ; mat[i][j]
    movl %eax, -4(%rbp)       ; y = mat[i][j]
```

---

## 5. Punteros

### 5.1 Operadores explícitos (`*`, `&`, `->`)

#### TypeChecker

**Operador de dirección (`&`):**
- Toma un l-value y devuelve un puntero a ese tipo
- Flujo: `Type* → PointerType*`

```cpp
void TypeChecker::visit(AddrOfNode* e) {
    isLvalContext = true;
    e->exp->accept(this);
    isLvalContext = false;
    Type* t = e->exp->resolvedType;
    e->resolvedType = new PointerType(t);
    typeCache.push_back(e->resolvedType);
}
```

**Operador de desreferencia (`*`):**
- Toma un puntero y devuelve el tipo al que apunta
- Resultado es un l-value
- Flujo: `PointerType* → Type*`

```cpp
void TypeChecker::visit(DerefNode* e) {
    e->exp->accept(this);
    Type* t = e->exp->resolvedType;
    if (auto* pt = dynamic_cast<PointerType*>(t)) {
        e->resolvedType = pt->base;
    } else {
        error("no se puede desreferenciar un tipo que no sea puntero.");
        e->resolvedType = intType;
    }
}
```

**Operador de acceso a miembro por puntero (`->`):**
- `p->m` es equivalente a `(*p).m`
- Flujo: `PointerType{StructType{...}} → Type*`

```cpp
void TypeChecker::visit(MemberAccessNode* e) {
    e->object->accept(this);
    Type* objType = e->object->resolvedType;
    
    // Verificar si es un puntero a struct
    if (auto* pt = dynamic_cast<PointerType*>(objType)) {
        if (auto* st = dynamic_cast<StructType*>(pt->base)) {
            // Buscar miembro en el struct
            auto it = st->members.find(e->member);
            if (it != st->members.end()) {
                e->resolvedType = it->second;
                return;
            }
        }
    }
    error("no existe el miembro '" + e->member + "'.");
}
```

**Ejemplo:**
```c
int x = 5;
int* p = &x;            // ok: &x es int*
int y = *p;             // ok: *p es int, y = 5
*p = 10;                // ok: *p es l-value, x = 10

struct Point { int x; int y; };
struct Point pt = {1, 2};
struct Point* pp = &pt;
int px = pp->x;         // ok: px = 1
pp->y = 20;             // ok: pt.y = 20
```

#### GenCodeVisitor

**Operador de dirección (`&`):**
```asm
; int x = 5; int* p = &x;
leaq -8(%rbp), %rax    ; cargar dirección de x (lea = load effective address)
movq %rax, -16(%rbp)   ; p = &x
```

**Operador de desreferencia (`*`):**
```asm
; int y = *p;
movq -16(%rbp), %rax   ; cargar p
movl (%rax), %eax      ; desreferenciar: *p
movl %eax, -24(%rbp)   ; y = *p

; *p = 10;
movq -16(%rbp), %rax   ; cargar p
movl $10, (%rax)       ; *p = 10
```

**Operador de acceso a miembro por puntero (`->`):**
```asm
; int px = pp->x;  (x está en offset 0 del struct)
movq -24(%rbp), %rax   ; cargar pp
movl (%rax), %eax      ; *(pp + 0) → pp->x
movl %eax, -28(%rbp)   ; px = pp->x

; pp->y = 20;  (y está en offset 4)
movq -24(%rbp), %rax   ; cargar pp
movl $20, 4(%rax)      ; *(pp + 4) = 20 → pp->y = 20
```

---

## 6. Memoria Dinámica

### 6.1 `malloc` y `free`

#### TypeChecker
- `malloc` y `free` se tratan como funciones predefinidas
- No hay necesidad de declararlas explícitamente (como en C)
- Verificación de llamadas:
  - `malloc(size_t)`: espera un entero, retorna `void*`
  - `free(void*)`: espera un puntero, no retorna nada

**Registro de funciones predefinidas:**
```cpp
TypeChecker::TypeChecker() {
    // Registrar malloc
    Type* voidStarType = new PointerType(voidType);
    typeCache.push_back(voidStarType);
    
    FuncInfo mallocInfo;
    mallocInfo.returnType = voidStarType;
    mallocInfo.paramTypes.push_back(intType);  // asumiendo size_t es int
    functions["malloc"] = mallocInfo;
    
    // Registrar free
    FuncInfo freeInfo;
    freeInfo.returnType = voidType;
    freeInfo.paramTypes.push_back(voidStarType);
    functions["free"] = freeInfo;
}
```

**Ejemplo:**
```c
int* arr = (int*)malloc(10 * sizeof(int));  // ok
free(arr);                                  // ok
```

#### GenCodeVisitor
- `malloc` y `free` se llaman como funciones externas
- El resultado de `malloc` se convierte (implícitamente o explícitamente) al tipo de puntero deseado

**Ejemplo de código generado:**
```asm
; int* arr = (int*)malloc(40);
movl $40, %edi         ; tamaño: 10 * 4 bytes
call malloc            ; retorna void* en %rax
movq %rax, -8(%rbp)    ; arr = malloc(40)

; free(arr);
movq -8(%rbp), %rdi    ; arr como argumento
call free
```

---

## 7. Promoción y Conversión Automática de Tipos

### 7.1 Reglas de Conversión

#### TypeChecker

La función `check_assign` verifica si un tipo se puede convertir a otro implícitamente:

```cpp
bool TypeChecker::check_assign(Type* target, Type* value) {
    // 1. Coincidencia exacta
    if (target->match(value)) return true;
    
    // 2. Cualquier puntero a cualquier puntero (coerción laxista)
    if (target->ttype == Type::POINTER && value->ttype == Type::POINTER) {
        return true;
    }
    
    // 3. Conversiones entre enteros (char ↔ int ↔ long)
    if (target->ttype == Type::CHAR && is_integral_type(value)) return true;
    if (target->ttype == Type::INT && is_integral_type(value)) return true;
    if (target->ttype == Type::LONG && is_integral_type(value)) return true;
    if (is_integral_type(target) && value->ttype == Type::CHAR) return true;
    if (is_integral_type(target) && value->ttype == Type::INT) return true;
    if (is_integral_type(target) && value->ttype == Type::LONG) return true;
    
    // 4. Entero → float/double
    if ((target->ttype == Type::FLOAT || target->ttype == Type::DOUBLE) && 
        is_integral_type(value)) {
        return true;
    }
    
    // 5. Float → double
    if (target->ttype == Type::DOUBLE && value->ttype == Type::FLOAT) {
        return true;
    }
    
    // 6. Entero → bool
    if (target->ttype == Type::BOOL && is_integral_type(value)) {
        return true;
    }
    
    // 7. Bool → entero
    if (is_integral_type(target) && value->ttype == Type::BOOL) {
        return true;
    }
    
    return false;
}
```

**Ejemplos de conversiones permitidas:**
```c
char c = 'A';
int i = c;           // ok (char → int)
long l = i;          // ok (int → long)
float f = l;         // ok (long → float)
double d = f;        // ok (float → double)
bool b = i;          // ok (int → bool)
```

### 7.2 Generación de Código para Conversiones

#### GenCodeVisitor

**Entero → Entero más grande (extensión de signo):**
```asm
; char → int
movsbl -8(%rbp), %eax  ; byte → dword, extender signo

; int → long
cltq                   ; sign-extender %eax a %rax
```

**Entero → Float/Double:**
```asm
; int → float
cvtsi2ssl -8(%rbp), %xmm0

; long → double
cvtsi2sdq -16(%rbp), %xmm0
```

**Float → Double:**
```asm
cvtss2sd %xmm0, %xmm1  ; float → double
```

**Ejemplo completo con múltiples conversiones:**
```c
int i = 42;
float f = i;           // int → float
double d = f;          // float → double
long l = d;            // error: no hay conversión double → long implícita
```

---

## 8. Resumen de Flujos Principales

### 8.1 Análisis Semántico (TypeChecker)

1. **Programa completo:**
   - Registrar firmas de funciones
   - Procesar declaraciones globales
   - Procesar structs
   - Typecheck de cada función

2. **Variable:**
   - Resolver tipo (type_from_ast)
   - Aplicar dimensiones de arreglos
   - Registrar en environment de scope
   - Verificar inicialización

3. **Expresión:**
   - Resolver tipos de operandos
   - Verificar operadores
   - Aplicar promociones
   - Determinar tipo de resultado

### 8.2 Generación de Código (GenCodeVisitor)

1. **Secciones:**
   - `.rodata`: Literales de cadena
   - `.text`: Código ejecutable

2. **Prologue de función:**
   ```asm
   pushq %rbp
   movq %rsp, %rbp
   subq $FRAME_SIZE, %rsp
   ```

3. **Epílogo de función:**
   ```asm
   leave
   ret
   ```

4. **Expresiones:**
   - Resultado en `%eax` (enteros 32 bits)
   - Resultado en `%rax` (enteros 64 bits, punteros)
   - Resultado en `%xmm0` (floats y doubles)

---

## 9. Tabla de Instrucciones por Tipo

| Tipo     | Tamaño | Mover      | Sumar      | Multiplicar | Dividir    | Comparar   |
|----------|--------|------------|------------|-------------|------------|------------|
| `char`   | 1      | `movb`     | `addb`     | —           | —          | `cmpb`     |
| `int`    | 4      | `movl`     | `addl`     | `imull`     | `idivl`    | `cmpl`     |
| `unsigned int` | 4 | `movl` | `addl` | `mull` | `divl` | `cmpl` |
| `long`   | 8      | `movq`     | `addq`     | `imulq`     | `idivq`    | `cmpq`     |
| `unsigned long` | 8 | `movq` | `addq` | `mulq` | `divq` | `cmpq` |
| `float`  | 4      | `movss`    | `addss`    | `mulss`     | `divss`    | `ucomiss`  |
| `double` | 8      | `movsd`    | `addsd`    | `mulsd`     | `divsd`    | `ucomisd`  |
| Puntero  | 8      | `movq`     | `addq`     | —           | —          | `cmpq`     |

| Condición (Signed) | Condición (Unsigned) | Instrucción |
|---------------------|-----------------------|-------------|
| Equal               | Equal                 | `sete`      |
| Not Equal           | Not Equal             | `setne`     |
| Less Than           | Below                 | `setl` / `setb` |
| Greater Than        | Above                 | `setg` / `seta` |
| Less or Equal       | Below or Equal        | `setle` / `setbe` |
| Greater or Equal    | Above or Equal        | `setge` / `setae` |
