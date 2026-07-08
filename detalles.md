# Detalles de Implementación: TypeChecker y Gencode

Este documento muestra —con código extraído directamente del compilador— cómo se gestionan cuatro características clave en las fases de **TypeChecker** (`TypeChecker.cpp`) y **Code Generation** (`Gencode.cpp`).

---

## 1. Punteros Explícitos (`*`, `&`, `->`)

### TypeChecker

#### Address-of (`&expr`) — `TypeChecker.cpp:877`

```cpp
case UnaryOp::ADDR:
{
    PointerType* ptr = new PointerType(t);  // envuelve el tipo del operando
    typeCache.push_back(ptr);               // preserva el tipo dinámico
    resultType = ptr;
}
break;
```

Si `expr` es `int`, el resultado es `int*`. El `PointerType` se construye envolviendo al tipo base y se guarda en `typeCache` para que no se fugue memoria.

#### Dereference (`*expr`) — `TypeChecker.cpp:884`

```cpp
case UnaryOp::DEREF:
    if (t->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)t;
        resultType = pt->base;             // desempaqueta: T* → T
    } else {
        error("operando de * debe ser puntero.");
        resultType = intType;
    }
    break;
```

Valida que el operando sea efectivamente un puntero; si lo es, entrega el tipo base (el tipo apuntado). Si no, error semántico.

#### Flecha (`ptr->member`) — `TypeChecker.cpp:1029`

```cpp
void TypeChecker::visit(ArrowAccessNode* e) {
    e->pointer->accept(this);
    Type* ptrType = e->pointer->resolvedType;
    if (ptrType->ttype != Type::POINTER) {
        error("acceso '->' requiere puntero (es " + ptrType->str() + ").");
        e->resolvedType = intType;
        return;
    }
    PointerType* pt = (PointerType*)ptrType;
    if (pt->base->ttype != Type::STRUCT) {
        error("acceso '->' requiere puntero a struct.");
        e->resolvedType = intType;
        return;
    }
    StructType* st = (StructType*)pt->base;
    auto it = st->members.find(e->member);
    if (it == st->members.end()) {
        error("el struct '" + st->name + "' no tiene miembro '" + e->member + "'.");
        e->resolvedType = intType;
        return;
    }
    e->resolvedType = it->second;  // tipo del campo
}
```

Tres validaciones en cascada: (1) que sea puntero, (2) que apunte a un struct, (3) que el miembro exista. El `resolvedType` final es el tipo del campo miembro.

#### Aritmética de punteros — `TypeChecker.cpp:773`

```cpp
case BinaryOp::ADD: case BinaryOp::SUB:
    if ((left->ttype == Type::POINTER && is_integral_type(right)) ||
        (is_integral_type(left) && right->ttype == Type::POINTER && e->op == BinaryOp::ADD)) {
        resultType = left->ttype == Type::POINTER ? left : right;  // ptr ± int → ptr
    } else if (left->ttype == Type::POINTER && right->ttype == Type::POINTER && e->op == BinaryOp::SUB) {
        resultType = intType;  // ptr - ptr → diferencia (entero)
    }
```

`ptr + int` preserva el tipo puntero. `ptr - ptr` da un entero (la diferencia en número de elementos).

---

### Gencode

#### Address-of — `computeAddress(IdNode)` — `Gencode.cpp:2105`

```cpp
void GenCodeVisitor::computeAddress(IdNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;   // modo l-value: captura info
        lvalTarget->name = e->name;
        lvalTarget->binding = e->binding;
        return;
    }
    leaBinding(e->binding);                 // modo r-value: leaq en %rax
}
```

Cuando se usa como l-value (para asignación), solo captura la variable en el `LVal`. En r-value (`&var`), emite `leaq offset(%rbp), %rax`.

#### Dereference — `visit(UnaryOpNode)` — `Gencode.cpp:1166, 1265`

```cpp
// Modo l-value: capturar para escritura
if (lvalTarget && e->op == UnaryOp::DEREF) {
    lvalTarget->kind = LValKind::Deref;
    lvalTarget->storeSize = e->resolvedType ? e->resolvedType->size() : 8;
    lvalTarget = nullptr;
    e->operand->accept(this);
    out << "  pushq %rax\n";   // guarda dirección del puntero
    return;
}

// Modo r-value: cargar valor
case UnaryOp::DEREF: {
    int derefSize = e->resolvedType ? e->resolvedType->size() : 8;
    bool derefUnsigned = e->resolvedType && e->resolvedType->isUnsigned;
    out << "  " << loadInstr(derefSize, derefUnsigned) << " (%rax), %rax\n";
    break;
}
```

En modo l-value (asignación a `*p = x`), se guarda la dirección del puntero en el stack para que `storeTarget` escriba ahí. En modo r-value (lectura de `*p`), se carga el valor desde la dirección apuntada.

#### Aritmética de punteros — `Gencode.cpp:989`

```cpp
if (leftIsPtr) {
    e->left->accept(this);
    out << "  pushq %rax\n";           // guarda dirección base
    e->right->accept(this);            // carga el entero
    PointerType* pt = static_cast<PointerType*>(leftType);
    int elemSize = pt->base ? pt->base->size() : 8;
    if (elemSize != 1) {
        out << "  movq $" << elemSize << ", %rcx\n";
        out << "  imulq %rcx, %rax\n"; // escala: índice * tamaño
    }
    out << "  popq %rcx\n";
    out << "  addq %rcx, %rax\n";     // base + desplazamiento
}
```

La aritmética de punteros escala el índice por `elemSize` (tamaño del tipo apuntado) antes de sumar a la base.

#### Flecha — `computeAddress(ArrowAccessNode)` — `Gencode.cpp:2141`

```cpp
void GenCodeVisitor::computeAddress(ArrowAccessNode *e) {
    if (auto *id = dynamic_cast<IdNode*>(e->pointer)) {
        loadBinding(id->binding);       // carga el valor del puntero
        // suma offset del campo dentro del struct
        out << "  addq $" << structFieldOffset[...][e->member] << ", %rax\n";
    }
}
```

Carga el puntero (la dirección del struct) y le suma el offset del miembro, dejando en `%rax` la dirección efectiva del campo.

#### Protocolo l-value — `storeTarget` para Deref — `Gencode.cpp:540`

```cpp
case LValKind::Deref: {
    int storeSize = lv.storeSize;
    string r = (storeSize == 1) ? "%al" : (storeSize == 4) ? "%eax" : "%rax";
    out << "  popq %rcx\n";            // recupera dirección del puntero
    out << "  " << storeInstr(storeSize) << " " << r << ", (%rcx)\n";
    break;
}
```

Para `*p = expr`, se evalúa `expr` → `%rax`, se recupera la dirección del puntero del stack, y se escribe el valor en esa dirección.

---

## 2. Promoción/Conversión Automática de Tipos

### TypeChecker

#### Jerarquía de promoción en operaciones binarias — `TypeChecker.cpp:781`

```cpp
} else if (is_arithmetic_type(left) && is_arithmetic_type(right)) {
    // Promoción automática al tipo más grande
    if (left->ttype == Type::DOUBLE || right->ttype == Type::DOUBLE)
        resultType = doubleType;
    else if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
        resultType = floatType;
    else if (left->ttype == Type::LONG || right->ttype == Type::LONG) {
        bool uns = (left->ttype == Type::LONG && left->isUnsigned) ||
                   (right->ttype == Type::LONG && right->isUnsigned);
        resultType = uns ? ulongType : longType;
    } else if (left->isUnsigned || right->isUnsigned)
        resultType = uintType;
    else
        resultType = intType;
}
```

La jerarquía es `double > float > long > unsigned > int`. Si alguien suma un `int` con un `double`, el resultado es `double`. Si suma `float` con `long`, el resultado es `float`.

#### Tabla de conversiones en asignación — `TypeChecker.cpp:173`

```cpp
bool TypeChecker::check_assign(Type* target, Type* value) {
    if (target->ttype == value->ttype && target->isUnsigned == value->isUnsigned) return true;
    if (target->ttype == Type::POINTER && value->ttype == Type::POINTER) return true;
    if (target->ttype == Type::CHAR && (value->ttype == Type::INT || value->ttype == Type::LONG)) return true;
    if (target->ttype == Type::INT && (value->ttype == Type::CHAR || value->ttype == Type::LONG)) return true;
    if (target->ttype == Type::LONG && is_integral_type(value)) return true;
    if ((target->ttype == Type::FLOAT || target->ttype == Type::DOUBLE) && is_integral_type(value)) return true;
    if (target->ttype == Type::DOUBLE && value->ttype == Type::FLOAT) return true;
    if (target->ttype == Type::FLOAT && value->ttype == Type::DOUBLE) return true;
    if (is_integral_type(target) && (value->ttype == Type::FLOAT || value->ttype == Type::DOUBLE)) return true;
    if (target->ttype == Type::BOOL && is_integral_type(value)) return true;
    if (is_integral_type(target) && value->ttype == Type::BOOL) return true;
    return false;
}
```

Cada línea es un caso de conversión implícita permitida. Si ninguna regla aplica, la asignación es ilegal y el TypeChecker reporta error.

---

### Gencode

#### Conversión en asignación — `Gencode.cpp:1293`

```cpp
Type* tgtType = nullptr;
if (target.kind == LValKind::Id && target.binding)
    tgtType = target.binding->resolvedType;

if (tgtType && tgtType->ttype == Type::DOUBLE) {
    if (valType && valType->ttype == Type::FLOAT) {
        out << "  movd %eax, %xmm7\n";
        out << "  cvtss2sd %xmm7, %xmm7\n";   // float → double
        out << "  movq %xmm7, %rax\n";
    } else if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
        out << "  cvtsi2sd %rax, %xmm7\n";     // int/char → double
        out << "  movq %xmm7, %rax\n";
    }
} else if (tgtType && tgtType->ttype == Type::FLOAT) {
    if (valType && valType->ttype == Type::DOUBLE) {
        out << "  movq %rax, %xmm7\n";
        out << "  cvtsd2ss %xmm7, %xmm7\n";   // double → float
        out << "  movd %xmm7, %eax\n";
        out << "  movslq %eax, %rax\n";
    } else if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
        out << "  cvtsi2ss %eax, %xmm7\n";     // int/char → float
        out << "  movd %xmm7, %eax\n";
        out << "  movslq %eax, %rax\n";
    }
} else if (tgtType && (tgtType->ttype == Type::INT || Type::CHAR || Type::LONG)) {
    if (valType && valType->ttype == Type::FLOAT) {
        out << "  movd %eax, %xmm7\n";
        out << "  cvtss2si %xmm7, %eax\n";     // float → int
    } else if (valType && valType->ttype == Type::DOUBLE) {
        out << "  movq %rax, %xmm7\n";
        out << "  cvtsd2si %xmm7, %eax\n";     // double → int
    }
}
```

Cada conversión entre tipos numéricos se traduce a la instrucción SSE específica. El patrón es siempre: cargar a registro SSE, convertir, y pasar el resultado a `%rax` para el `storeTarget`.

#### Conversión en operaciones binarias — `Gencode.cpp:893`

```cpp
if (useFloat) {
    e->left->accept(this);
    if (useDouble) {
        if (!isFloatSemanticType(leftType))
            out << "  cvtsi2sd %rax, %xmm0\n";  // entero → double
        else if (leftType->ttype == Type::FLOAT) {
            out << "  movd %eax, %xmm0\n";
            out << "  cvtss2sd %xmm0, %xmm0\n";  // float → double
        } else
            out << "  movq %rax, %xmm0\n";       // double directo
    }
    // ... mismo para right → %xmm1 ...
    // ... operación SSE (addsd, subsd, etc.) ...
}
```

Ambos operandos se convierten al tipo más ancho antes de operar. Si la operación es `int + double`, el `int` se convierte con `cvtsi2sd`. Si es `float + double`, el `float` se convierte con `cvtss2sd`.

#### Conversión en inicialización de variables — `Gencode.cpp:1967`

```cpp
if (d->resolvedType && d->resolvedType->ttype == Type::DOUBLE) {
    Type* initType = d->initializer->resolvedType;
    if (initType && initType->ttype == Type::FLOAT) {
        out << "  movd %eax, %xmm7\n";
        out << "  cvtss2sd %xmm7, %xmm7\n";      // float → double
        out << "  movq %xmm7, %rax\n";
    } else if (!initType || initType->ttype == Type::INT || Type::CHAR) {
        out << "  cvtsi2sd %rax, %xmm7\n";        // int/char → double
    }
}
```

Idéntico patrón al de asignación: el inicializador se evalúa, se convierte al tipo destino si es necesario, y se almacena.

#### Conversión float→double en printf — `Gencode.cpp:1435`

```cpp
if (argType && argType->ttype == Type::FLOAT) {
    out << "  movd %eax, " << xmmRegs[xmmIdx] << "\n";
    out << "  cvtss2sd " << xmmRegs[xmmIdx] << ", " << xmmRegs[xmmIdx] << "\n";
}
```

La ABI variádica de System V exige que todos los flotantes se pasen como `double` en registros `%xmm`. Por eso C-- convierte `float` → `double` antes de cada llamada a `printf`.

---

## 3. `float` / `double`

### TypeChecker

#### Tipos singleton — `TypeChecker.cpp:68`

```cpp
TypeChecker::TypeChecker() {
    floatType = new Type(Type::FLOAT);    // singleton reutilizable
    doubleType = new Type(Type::DOUBLE);  // singleton reutilizable
}
```

`floatType` y `doubleType` son instancias únicas de `Type` que se reusan en todo el typechecking para no crear objetos redundantes.

#### Resolución de literales flotantes — `TypeChecker.cpp:1110`

```cpp
void TypeChecker::visit(FloatLiteralNode* e) {
    e->resolvedType = (e->literalSuffix == Token::LiteralSuffix::SUF_F)
                      ? floatType : doubleType;
}
```

`3.14f` (sufijo `F`) → `floatType`. `3.14` (sin sufijo) → `doubleType`.

#### Negación unaria en flotantes — `TypeChecker.cpp:851`

```cpp
case UnaryOp::MINUS:
    if (is_arithmetic_type(t)) {
        resultType = is_integral_type(t) ? intType : t;  // float → float, double → double
    }
```

A diferencia de los enteros (que promueven a `int`), `-float` preserva `float` y `-double` preserva `double`.

---

### Gencode

#### Helper de detección — `Gencode.cpp:54`

```cpp
static bool isFloatSemanticType(Type* t) {
    return t && (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE);
}
```

Usado en toda la generación de código para decidir si usar instrucciones SSE o enteras.

#### Carga de variables float/double — `Gencode.cpp:278`

```cpp
void GenCodeVisitor::loadBinding(VarDecl* vd) {
    Type* type = vd->resolvedType;
    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movsd " << bindingMem(vd) << ", %xmm7\n";
            out << "  movq %xmm7, %rax\n";
        } else {
            out << "  movss " << bindingMem(vd) << ", %xmm7\n";
            out << "  movd %xmm7, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }
    // ... enteros ...
}
```
```
float  → movss (4 bytes) → %xmm7 → movd a %eax → sign-extend a %rax
double → movsd (8 bytes) → %xmm7 → movq directo a %rax
```

#### Almacenamiento de variables float/double — `Gencode.cpp:297`

```cpp
void GenCodeVisitor::storeBinding(VarDecl* vd) {
    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movq %rax, %xmm7\n";
            out << "  movsd %xmm7, " << bindingMem(vd) << "\n";
        } else {
            out << "  movd %eax, %xmm7\n";
            out << "  movss %xmm7, " << bindingMem(vd) << "\n";
        }
        return;
    }
}
```

Patrón inverso a la carga: `%rax` → registro SSE → `movsd`/`movss` a memoria.

#### Operaciones aritméticas SSE — `Gencode.cpp:924`

```cpp
switch (e->op) {
case BinaryOp::ADD:
    out << (useDouble ? "  addsd %xmm1, %xmm0\n" : "  addss %xmm1, %xmm0\n");
    break;
case BinaryOp::SUB:
    out << (useDouble ? "  subsd %xmm1, %xmm0\n" : "  subss %xmm1, %xmm0\n");
    break;
case BinaryOp::MUL:
    out << (useDouble ? "  mulsd %xmm1, %xmm0\n" : "  mulss %xmm1, %xmm0\n");
    break;
case BinaryOp::DIV:
    out << (useDouble ? "  divsd %xmm1, %xmm0\n" : "  divss %xmm1, %xmm0\n");
    break;
case BinaryOp::EQ:
    out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
    out << "  movq $0, %rax\n";
    out << "  sete %al\n";
    out << "  movzbq %al, %rax\n";
    return;
// ... LT, GT, LE, GE (setb, seta, setbe, setae) ...
}
```

Cada operación aritmética tiene su instrucción SSE: `addss`/`addsd`, `subss`/`subsd`, etc. Las comparaciones usan `ucomiss`/`ucomisd` y luego `sete`/`setne`/`setb`/etc. para extraer la bandera a `%rax`.

#### Negación unaria flotante — `Gencode.cpp:1208`

```cpp
case UnaryOp::MINUS:
    if (e->resolvedType && isFloatSemanticType(e->resolvedType)) {
        if (e->resolvedType->ttype == Type::DOUBLE) {
            out << "  movq %rax, %xmm0\n";
            out << "  movabsq $0x8000000000000000, %rcx\n";
            out << "  movq %rcx, %xmm7\n";
            out << "  xorpd %xmm7, %xmm0\n";   // flip bit de signo (double)
            out << "  movq %xmm0, %rax\n";
        } else {
            out << "  movd %eax, %xmm0\n";
            out << "  movl $0x80000000, %ecx\n";
            out << "  movd %ecx, %xmm7\n";
            out << "  xorps %xmm7, %xmm0\n";   // flip bit de signo (float)
            out << "  movd %xmm0, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
    }
```

En vez de restar de cero, la negación flotante hace XOR del bit de signo (el MSB del formato IEEE-754): `0x8000000000000000` para double, `0x80000000` para float.

#### Literales flotantes — `Gencode.cpp:776`

```cpp
void GenCodeVisitor::visit(FloatLiteralNode *e) {
    if (e->resolvedType && e->resolvedType->ttype == Type::DOUBLE) {
        union { double d; unsigned long long i; } dc;
        dc.d = e->value;
        out << "  movq $0x" << hex << dc.i << dec << ", %rax\n";
    } else {
        union { float f; unsigned int i; } fc;
        fc.f = (float)e->value;
        out << "  movl $0x" << hex << fc.i << dec << ", %eax\n";
        out << "  movslq %eax, %rax\n";
    }
}
```

Usa un `union` para obtener los bits IEEE-754 del literal y los carga como inmediato. Para double: `movq` con 64 bits. Para float: `movl` con 32 bits + sign-extend.

#### Inicialización global de flotantes — `Gencode.cpp:83`

```cpp
if (g->resolvedType->ttype == Type::DOUBLE) {
    union { double d; unsigned long long i; } dc;
    dc.d = g->initializer->constantValue;
    out << g->name << ": .quad 0x" << hex << dc.i << dec << "\n";
} else {
    union { float f; unsigned int i; } fc;
    fc.f = (float)g->initializer->constantValue;
    out << g->name << ": .long 0x" << hex << fc.i << dec << "\n";
}
```

Similar a los literales, pero en la sección `.data` del assembly: `.quad` para double (8 bytes), `.long` para float (4 bytes).

#### Direct store peephole para float — `Gencode.cpp:733`

```cpp
if (tt->ttype == Type::FLOAT) {
    union { float f; unsigned int i; } fc;
    fc.f = (float)intVal;
    out << "  movl $0x" << hex << floatBits << dec << ", " << mem << "\n";
    return true;
}
if (tt->ttype == Type::DOUBLE) {
    return false;  // no cabe como inmediato de 64 bits
}
```

Optimización: si el destino es `float` y el valor es constante, escribe directamente el inmediato IEEE-754 de 32 bits en memoria, evitando el viaje por `%rax`. Double se salta porque el inmediato de 64 bits no cabe en una sola instrucción x86-64.

---

## 4. Matrices (Arreglos Multidimensionales)

### TypeChecker

#### Construcción del tipo Array — `TypeChecker.cpp:322`

```cpp
for (int idx = (int)v->array_sizes.size() - 1; idx >= 0; idx--) {
    int dim = -1;
    if (auto* il = dynamic_cast<NumberLiteralNode*>(v->array_sizes[idx]))
        dim = (int)il->value;
    ArrayType* at = new ArrayType(t, dim);   // envuelve el tipo actual
    typeCache.push_back(at);
    t = at;
}
v->resolvedType = t;
```

Para `int m[2][3]`, `array_sizes = [2, 3]`. El loop en orden inverso construye:
```
paso idx=1: ArrayType(int, 3)
paso idx=0: ArrayType(ArrayType(int, 3), 2)
```
El resultado anida la última dimensión como la más interna.

#### Indexación — `TypeChecker.cpp:984`

```cpp
void TypeChecker::visit(IndexNode* e) {
    e->base->accept(this); Type* base = e->base->resolvedType;
    e->index->accept(this); Type* index = e->index->resolvedType;
    if (!is_switch_index_type(index)) {
        error("índice de arreglo debe ser int o char.");
    }
    if (base->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)base;
        e->resolvedType = at->base;     // pela una dimensión
        return;
    }
    if (base->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)base;
        e->resolvedType = pt->base;     // acceso por puntero
        return;
    }
}
```

Cada `IndexNode` pela una capa de `ArrayType`. Para `m[i][j]`, el primer índice reduce `ArrayType(ArrayType(int,3),2)` → `ArrayType(int,3)`, y el segundo reduce a `int`.

#### Array-to-pointer decay — `TypeChecker.cpp:1067`

```cpp
if (!isLvalContext && vd->resolvedType->ttype == Type::ARRAY) {
    ArrayType* at = (ArrayType*)vd->resolvedType;
    PointerType* ptrType = new PointerType(at->base);
    typeCache.push_back(ptrType);
    e->resolvedType = ptrType;  // decae a puntero al primer elemento
}
```

En contexto rvalue (ej. pasar un arreglo a una función), el arreglo decae a `PointerType(tipo_base)`.

---

### Gencode

#### Extracción de dimensiones — `Gencode.cpp:231, 242`

```cpp
static vector<int> arrayDimsForType(Type* t) {
    vector<int> dims;
    while (t && t->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)t;
        if (at->length > 0) dims.push_back(at->length);
        t = at->base;
    }
    return dims;
}

static vector<int> arrayDimsFor(VarDecl* vd) {
    if (!vd || !vd->resolvedType) return {};
    return arrayDimsForType(vd->resolvedType);
}
```

Recorre el árbol de `ArrayType` anidado extrayendo cada `length`. Para `int[2][3]` → `{2, 3}`.

#### Tamaño del elemento base — `Gencode.cpp:216`

```cpp
int GenCodeVisitor::array_elem_size(VarDecl* vd) const {
    if (!vd || !vd->resolvedType) return 8;
    Type* t = vd->resolvedType;
    if (t->ttype == Type::ARRAY) {
        Type* base = t;
        while (base && base->ttype == Type::ARRAY)
            base = ((ArrayType*)base)->base;
        if (base) return base->size();    // int → 4, char → 1, double → 8
    }
    if (t->ttype == Type::POINTER && ((PointerType*)t)->base)
        return ((PointerType*)t)->base->size();
    return 8;
}
```

Desanida todas las capas de `ArrayType` hasta llegar al tipo elemental y retorna su `size()`.

#### Recolección de índices anidados — `Gencode.cpp:259`

```cpp
static void collectIndices(Exp* e, vector<Exp*>& indices, Exp*& base) {
    Exp* b = e;
    while (auto* inner = dynamic_cast<IndexNode*>(b)) {
        indices.push_back(inner->index);
        b = inner->base;
    }
    reverse(indices.begin(), indices.end());  // orden original: i, j, k
    base = b;
}
```

Para `a[i][j][k]`, recorre la cadena de `IndexNode`s y recolecta `indices = [i, j, k]`, `base = a`.

#### Cálculo de dirección multidimensional — `Gencode.cpp:327`

```cpp
void GenCodeVisitor::emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices) {
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);

    if (dims.size() > 1) {
        leaBinding(vd);                       // dirección base → %r8
        out << "  movq %rax, %r8\n";
        out << "  movq $0, %r10\n";           // acumulador de offset lineal
        for (size_t d = 0; d < indices.size(); d++) {
            indices[d]->accept(this);          // índice → %rax
            int stride = 1;
            for (size_t s = d + 1; s < dims.size(); s++)
                stride *= dims[s];             // stride = producto de dims siguientes
            if (stride != 1) {
                out << "  movq $" << stride << ", %rcx\n";
                out << "  imulq %rcx, %rax\n";  // índice × stride
            }
            out << "  addq %rax, %r10\n";       // acumula
        }
        out << "  movq %r8, %rax\n";
        // escala por elemSize
        if (elemSize == 4) out << "  leaq (%rax,%r10,4), %rax\n";
        else { /* imulq + addq para otros tamaños */ }
        return;
    }
    // caso 1D: direccionamiento indexado directo
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    leaBinding(vd);
    out << "  leaq (%rax,%rdi,4), %rax\n";  // base + índice*4
}
```
```
Ejemplo: int m[2][3], acceder m[i][j]
  stride[0] = 3, stride[1] = 1
  offset_linear = i*3 + j
  dirección = base + offset_linear * 4
```

Para 1D usa direccionamiento indexado directo de x86-64 (`(%rax,%rdi,4)`). Para N-dimensiones calcula el offset lineal como `Σ (índice_d × stride_d)` y luego escala por `elemSize`.

#### Carga indexada — `Gencode.cpp:384`

```cpp
void GenCodeVisitor::emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices) {
    if (dims.size() > 1) {
        emitIndexedAddress(vd, indices);               // dirección → %rax
        if (indices.size() < dims.size()) return;     // sub-arreglo: dejar dirección
        if (elemSize == 4) out << "  movslq (%rax), %rax\n";
        else out << "  movq (%rax), %rax\n";
        return;
    }
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    leaBinding(vd);
    if (elemSize == 4)
        out << "  movslq (%rax,%rdi,4), %rax\n";      // carga: base + índice*4
}
```

Combina el cálculo de dirección con una carga desde memoria `(%rax)`. Para sub-arreglos (cuando hay menos índices que dimensiones), deja la dirección en `%rax`.

#### Almacenamiento indexado — `Gencode.cpp:427`

```cpp
void GenCodeVisitor::emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices,
                                      const string& valueReg) {
    if (dims.size() > 1) {
        out << "  movq %rax, %r9\n";           // preserva valor
        emitIndexedAddress(vd, indices);       // dirección → %rax
        out << "  movq %rax, %rbx\n";
        out << "  " << storeInstr(elemSize) << " %r9d, (%rbx)\n";  // escribe
        return;
    }
    // 1D: lea + store indexado
    out << "  movq %rax, %rcx\n";
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    leaBinding(vd);
    out << "  movl %ecx, (%rax,%rdi,4)\n";
}
```

Patrón inverso a la carga: preserva el valor a escribir, calcula la dirección y emite el store.

#### Inicialización con lista — `Gencode.cpp:2007`

```cpp
if (!d->init_list.empty()) {
    int elemSize = array_elem_size(d);
    for (size_t i = 0; i < d->init_list.size(); i++) {
        d->init_list[i]->accept(this);          // cada inicializador → %rax
        int off = d->offset + (int)i * elemSize;
        if (isFloatSemanticType(elemType)) {
            // movsd / movss para float/double
        } else {
            out << "  " << storeInstr(size) << " " << reg << ", " << off << "(%rbp)\n";
        }
    }
}
```

Para `int m[2][3] = {1,2,3,4,5,6}`, cada elemento se almacena secuencialmente en el stack usando `offset + i × elemSize`.
