# Guía de Implementación: Características de C--

Esta guía explica cómo se implementaron las siguientes características en el compilador C--, usando ejemplos del código fuente y justificando las elecciones de diseño.

## Contenido

1. [Arquitectura de tipos: TypeNode vs Type*](#1-arquitectura-de-tipos-typenode-vs-type)
2. [Punteros explícitos (*, &, ->)](#2-punteros-explícitos---)
3. [Promoción/conversión automática de tipos](#3-promociónconversión-automática-de-tipos)
4. [float y double](#4-float-y-double)
5. [Matrices (arreglos multidimensionales)](#5-matrices-arreglos-multidimensionales)
6. [const](#6-const)
7. [void](#7-void)

---

## 1. Arquitectura de tipos: TypeNode vs Type*

### Descripción general

El compilador C-- utiliza **dos jerarquías de tipos** separadas para evitar casteo innecesario en el TypeChecker y GenCode:

1. **TypeNode (ast.h):** Nodos del AST que representan tipos en el código fuente
   - `PrimitiveTypeNode`: int, char, float, double, bool, void, long
   - `PointerTypeNode`: T* (punteros)
   - `StructTypeNode`: struct Nombre

2. **Type* (semantic_types.h):** Tipos semánticos resueltos después del análisis
   - `Type`: Base para tipos primitivos (con discriminante `ttype`)
   - `PointerType`: Punteros con tipo base resuelto
   - `ArrayType`: Arreglos con dimensiones y tipo base
   - `StructType`: Structs con tabla de miembros

**Flujo de conversión:**
```
Parser → TypeNode (AST) → TypeChecker::type_from_ast() → Type* (semántico)
                                                    ↓
                                            Exp::resolvedType
                                            VarDecl::resolvedType
```

### Implementación en el AST

**ast.h - TypeNode y sus subclases:**
```cpp
// Clase base para nodos de tipo en el AST
class TypeNode {
public:
    Location loc;
    bool isConst = false;
    virtual ~TypeNode() {}
    virtual void accept(Visitor* visitor) = 0;
};

// Tipo primitivo: int, char, float, double, bool, void, long
class PrimitiveTypeNode : public TypeNode {
public:
    enum Prim { VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, LONG };
    Prim prim;
    bool isUnsigned = false;
    // ...
};

// Tipo puntero: T*
class PointerTypeNode : public TypeNode {
public:
    TypeNode* base;  // Tipo base (otro TypeNode)
    PointerTypeNode(TypeNode* b);
    // ...
};

// Tipo struct por nombre
class StructTypeNode : public TypeNode {
public:
    string name;
    StructTypeNode(const string& n);
    // ...
};
```

**ast.h - Exp y VarDecl con resolvedType:**
```cpp
// Expresión con tipo semántico resuelto
class Exp {
public:
    Type* resolvedType = nullptr;  // Tipo semántico (Type*)
    bool isConstant = false;
    double constantValue = 0.0;
    // ...
};

// Declaración de variable con tipo resuelto
class VarDecl : public Stm {
public:
    TypeNode* type;         // Tipo del AST (sintaxis)
    Type* resolvedType;      // Tipo semántico (Type*)
    int offset = 0;         // Offset en stack frame
    int memSize = 0;        // Tamaño en bytes
    // ...
};
```

### Implementación en semantic_types.h

**semantic_types.h - Jerarquía de tipos semánticos:**
```cpp
// Tipo base con discriminante
class Type {
public:
    enum TType { NOTYPE, VOID, INT, CHAR, BOOL, FLOAT, DOUBLE, LONG, POINTER, ARRAY, STRUCT };
    TType ttype;           // Discriminante: qué subclase es
    bool isUnsigned = false;
    bool isConst = false;

    virtual bool match(Type* t) const {
        return this->ttype == t->ttype && this->isUnsigned == t->isUnsigned;
    }

    virtual int size() const {
        switch (ttype) {
            case VOID:   return 0;
            case INT:    return 4;
            case CHAR:   return 1;
            case BOOL:   return 1;
            case FLOAT:  return 4;
            case DOUBLE: return 8;
            case LONG:   return 8;
            case POINTER: return 8;
            default:     return 8;
        }
    }
};

// Puntero semántico
class PointerType : public Type {
public:
    Type* base;  // Tipo apuntado (Type*, no TypeNode)
    PointerType(Type* b) : Type(POINTER), base(b) {}

    bool match(Type* t) const override {
        if (t->ttype != POINTER) return false;
        PointerType* pt = (PointerType*)t;
        return base->match(pt->base);
    }

    string str() const override {
        return base->str() + "*";
    }
};

// Arreglo semántico (puede ser multidimensional)
class ArrayType : public Type {
public:
    Type* base;
    int length;  // -1 si tamaño desconocido

    ArrayType(Type* b, int l = -1) : Type(ARRAY), base(b), length(l) {}

    bool match(Type* t) const override {
        if (t->ttype != ARRAY) return false;
        ArrayType* at = (ArrayType*)t;
        return base->match(at->base);  // Ignora length
    }

    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8;  // Arreglo sin tamaño → como puntero
    }
};

// Struct semántico
class StructType : public Type {
public:
    string name;
    unordered_map<string, Type*> members;  // nombre → Type*

    StructType(const string& n) : Type(STRUCT), name(n) {}

    bool match(Type* t) const override {
        if (t->ttype != STRUCT) return false;
        StructType* st = (StructType*)t;
        return name == st->name;  // Equivalencia nominal
    }

    int size() const override {
        int total = 0;
        for (auto& pair : members) {
            total += pair.second->size();
        }
        return total;
    }
};
```

### Implementación en TypeChecker

**TypeChecker.cpp - type_from_ast (conversión TypeNode → Type*):**
```cpp
Type* TypeChecker::type_from_ast(TypeNode* t) {
    // 1. Tipos primitivos → singletons
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(t)) {
        switch (pt->prim) {
            case PrimitiveTypeNode::VOID:   res = voidType; break;
            case PrimitiveTypeNode::INT:    res = intType; break;
            case PrimitiveTypeNode::CHAR:   res = charType; break;
            case PrimitiveTypeNode::FLOAT:  res = floatType; break;
            case PrimitiveTypeNode::DOUBLE: res = doubleType; break;
            case PrimitiveTypeNode::BOOL:   res = boolType; break;
            case PrimitiveTypeNode::LONG:   res = longType; break;
        }
        // Clonar si tiene modificadores (unsigned/const)
        if (res && (pt->isUnsigned || pt->isConst)) {
            Type* clone = new Type(res->ttype);
            clone->isUnsigned = pt->isUnsigned;
            clone->isConst = pt->isConst;
            typeCache.push_back(clone);
            res = clone;
        }
        return res;
    }

    // 2. Punteros → PointerType recursivo
    if (auto* pt = dynamic_cast<PointerTypeNode*>(t)) {
        Type* base = type_from_ast(pt->base);  // Recursión
        PointerType* ptr = new PointerType(base);
        ptr->isConst = pt->isConst;
        typeCache.push_back(ptr);
        return ptr;
    }

    // 3. Structs → StructType por nombre
    if (auto* st = dynamic_cast<StructTypeNode*>(t)) {
        auto it = struct_types.find(st->name);
        if (it != struct_types.end()) {
            res = it->second;
            if (st->isConst) {
                StructType* clone = new StructType(st->name);
                clone->members = ((StructType*)res)->members;
                clone->isConst = true;
                typeCache.push_back(clone);
                res = clone;
            }
            return res;
        }
        error("struct '" + st->name + "' no declarado.");
        return intType;
    }

    return intType;  // Fallback
}
```

**TypeChecker.cpp - Asignación de resolvedType:**
```cpp
// En visit(VarDecl):
Type* t = type_from_ast(v->type);  // TypeNode → Type*

// Wrap en ArrayType si tiene dimensiones
for (int idx = (int)v->array_sizes.size() - 1; idx >= 0; idx--) {
    int dim = -1;
    if (auto* il = dynamic_cast<IntegerLiteralNode*>(v->array_sizes[idx]))
        dim = (int)il->value;
    ArrayType* at = new ArrayType(t, dim);
    typeCache.push_back(at);
    t = at;
}

v->resolvedType = t;  // Guardar Type* en el nodo
v->memSize = t->size();
```

### Implementación en GenCodeVisitor

**Gencode.cpp - Uso de resolvedType (sin casteo de TypeNode):**
```cpp
// Cargar variable: usa resolvedType para determinar instrucción
void GenCodeVisitor::loadBinding(VarDecl* vd) {
    int size = vd->memSize > 0 ? vd->memSize : 8;
    Type* type = vd->resolvedType;  // Type*, no TypeNode

    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movsd " << bindingMem(vd) << ", %xmm7\n";
            out << "  movq %xmm7, %rax\n";
        } else {  // FLOAT
            out << "  movss " << bindingMem(vd) << ", %xmm7\n";
            out << "  movd %xmm7, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }
    out << "  " << loadInstr(size) << " " << bindingMem(vd) << ", %rax\n";
}

// Discriminar tipo usando ttype (no dynamic_cast de TypeNode)
if (type->ttype == Type::ARRAY) {
    ArrayType* at = (ArrayType*)type;  // Casteo de Type* a ArrayType*
    // ...
} else if (type->ttype == Type::POINTER) {
    PointerType* pt = (PointerType*)type;  // Casteo de Type* a PointerType*
    // ...
}
```

### Ventajas de esta arquitectura

1. **Separación de responsabilidades:**
   - `TypeNode`: Representación sintáctica (lo que escribe el usuario)
   - `Type*`: Representación semántica (lo que entiende el compilador)

2. **Evita casteo repetitivo:**
   - TypeChecker convierte una vez: `TypeNode → Type*`
   - GenCode usa directamente `Type*` sin necesidad de `dynamic_cast<TypeNode*>`

3. **Singletons para primitivos:**
   - `intType`, `charType`, etc. se crean una vez en el constructor
   - Ahorra memoria y simplifica comparaciones

4. **Cache de tipos compuestos:**
   - `typeCache` almacena PointerType, ArrayType, StructType creados dinámicamente
   - El destructor de TypeChecker los libera automáticamente

5. **Información de tamaño en Type*:**
   - `Type::size()` calcula el tamaño en bytes
   - GenCode usa esto para elegir `movb`/`movl`/`movq`

### Ciclo de vida de los tipos

```
1. Constructor TypeChecker:
   - Crea singletons (intType, charType, floatType, etc.)

2. type_from_ast(TypeNode*):
   - Primitivos → retorna singleton (o clon con modificadores)
   - Compuestos → new PointerType/ArrayType/StructType → typeCache

3. Durante typecheck:
   - resolvedType se asigna a Exp, VarDecl, etc.
   - Se usa para verificación de tipos (match(), check_assign())

4. Destructor TypeChecker:
   - delete todos los elementos de typeCache
   - delete todos los struct_types
   - delete singletons
```

### Ejemplo completo

```cpp
// Código fuente:
int x = 5;
int* p = &x;
int arr[3][2];

// AST (Parser):
VarDecl { type: PrimitiveTypeNode(INT), name: "x", initializer: IntegerLiteralNode(5) }
VarDecl { type: PointerTypeNode(PrimitiveTypeNode(INT)), name: "p", initializer: UnaryOpNode(ADDR, IdentifierNode("x")) }
VarDecl { type: PrimitiveTypeNode(INT), name: "arr", array_sizes: [3, 2] }

// TypeChecker (type_from_ast):
x.resolvedType = intType (singleton)
p.resolvedType = PointerType(intType) (nuevo en typeCache)
arr.resolvedType = ArrayType(ArrayType(intType, 2), 3) (nuevos en typeCache)

// GenCode (usa resolvedType):
loadBinding(x) → type->ttype == INT → movslq
loadBinding(p) → type->ttype == POINTER → movq
loadBinding(arr[1][0]) → type->ttype == ARRAY → cálculo de índice lineal
```

### Justificación de diseño

- **Clean separation:** Sintaxis vs semántica claramente separadas
- **Performance:** Una conversión TypeNode→Type*, luego solo uso de Type*
- **Type safety:** Casteo solo entre subclases de Type* (ArrayType*, PointerType*, etc.)
- **Memory management:** typeCache centraliza la gestión de memoria de tipos dinámicos
- **Extensibilidad:** Fácil agregar nuevos tipos semánticos (ej. FunctionType)

---

## 2. Punteros explícitos (*, &, ->)

### Descripción general

Los punteros permiten manipular direcciones de memoria directamente. C-- soporta:

- `&` para obtener la dirección de una variable
- `*` para desreferenciar un puntero
- `->` para acceder a miembros de structs a través de punteros

### Implementación en el Scanner

Primero, el Scanner (lexer) reconoce estos tokens:

**Token.h (implícito en Scanner):**
- `*` reconocido como token operador
- `&` reconocido como token operador
- `->` reconocido como token operador

### Implementación en el Parser

El Parser usa recursión descendente para construir el AST. Los punteros se representan en tres nodos principales:

**ast.h - Nodos del AST para punteros:**
```cpp
// 1. Tipo puntero: int*
class PointerTypeNode : public TypeNode {
public:
    TypeNode* base;
    PointerTypeNode(TypeNode* b);
    // ...
};

// 2. Operador unario * (desreferencia) y & (dirección)
enum class UnaryOp {
    ADDR,    // &x
    DEREF,   // *p
    // ... otros unarios
};

class UnaryOpNode : public Exp {
public:
    UnaryOp op;
    Exp* operand;
    // ...
};

// 3. Acceso -> a struct
class ArrowAccessNode : public Exp {
public:
    Exp* pointer;
    string member;
    ArrowAccessNode(Exp* p, const string& m);
    // ...
};
```

**Parser.cpp (parse_unary y parse_postfix):**
- `parse_unary()` reconoce `&` y `*` como operadores unarios y crea `UnaryOpNode` con `ADDR` o `DEREF`
- `parse_postfix()` reconoce `->` y crea `ArrowAccessNode`

### Implementación en el TypeChecker

**semantic_types.h - PointerType:**
```cpp
class PointerType : public Type {
public:
    Type* base;  // tipo al que apunta
    PointerType(Type* b) : Type(POINTER), base(b) {}

    // Igualdad de punteros: int* == int* (mismo base)
    bool match(Type* t) const override {
        if (t->ttype != POINTER) return false;
        PointerType* pt = (PointerType*)t;
        return base->match(pt->base);
    }

    string str() const override {
        return base->str() + "*";
    }
};
```

**TypeChecker.cpp - visit(UnaryOpNode):**
```cpp
case UnaryOp::ADDR: {
    // &x: devuelve PointerType(tipo_de_x)
    PointerType* ptr = new PointerType(t);
    typeCache.push_back(ptr);
    resultType = ptr;
    break;
}
case UnaryOp::DEREF:
    // *p: devuelve el tipo base del puntero
    if (t->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)t;
        resultType = pt->base;
    } else {
        error("operando de * debe ser puntero.");
    }
    break;
```

### Implementación en el GenCodeVisitor

Para la generación de código x86-64:

**Gencode.cpp - Operador & (ADDR):**
```cpp
// &x → leaq x(%rbp), %rax  (carga la dirección efectiva)
void GenCodeVisitor::leaBinding(VarDecl* vd) {
    out << "  leaq " << bindingMem(vd) << ", %rax\n";
}
```

**Gencode.cpp - Operador * (DEREF):**
```cpp
// Para *p:
// 1. Cargar p en %rax
// 2. Cargar el valor desde (%rax)
// Ejemplo: *p = 5 → movq p(%rbp), %rbx; movl $5, (%rbx)
```

**Gencode.cpp - Operador -> (ArrowAccessNode):**
```cpp
// p->x es equivalente a (*p).x
// 1. Obtener la dirección del struct (*p)
// 2. Sumar el offset del miembro x
// 3. Cargar/almacenar desde esa dirección
```

### Ejemplo de uso (test11_pointers.cnn):

```cpp
int x;
int* p;
x = 100;
p = &x;
*p = 200;  // modifica x a través de p
printf(x);  // 200

// Puntero a struct
struct Point pt;
struct Point* ptPtr;
ptPtr = &pt;
ptPtr->x = 10;  // equivalente a (*ptPtr).x = 10
```

### Justificación de diseño

- **Sintaxis similar a C:** Familiar para programadores C, reduciendo la curva de aprendizaje
- **PointerType separado:** Permite manejar punteros de cualquier tipo base (int*, char*, etc.)
- **ArrowAccessNode como sintaxis sugar:** Simplifica la escritura de `(*p).x` como `p->x`

---

## 2. Promoción/conversión automática de tipos

### Descripción general

C-- promueve tipos automáticamente según reglas de promoción aritmética, similar a C:

- **Aritmética:** Si algún operando es `double` → `double`; si no, si alguno es `float` → `float`; si no → `int`
- **Asignación:** Conversión implícita entre tipos compatibles (int ↔ float, int ↔ char, etc.)

### Implementación en el TypeChecker

**TypeChecker.cpp - Funciones auxiliares:**
```cpp
// true para int, char, long (tipos enteros)
bool is_integral_type(Type* t) {
    return t->ttype == Type::INT || t->ttype == Type::CHAR || t->ttype == Type::LONG;
}

// true para tipos aritméticos (int, char, float, double)
bool is_arithmetic_type(Type* t) {
    return is_integral_type(t) || t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE;
}
```

**TypeChecker.cpp - check_assign (asignación):**
```cpp
bool TypeChecker::check_assign(Type* target, Type* value) {
    // 1. Tipos exactos
    if (target->ttype == value->ttype && target->isUnsigned == value->isUnsigned) 
        return true;
    
    // 2. Cualquier puntero ↔ cualquier puntero (coerción laxa)
    if (target->ttype == Type::POINTER && value->ttype == Type::POINTER) 
        return true;
    
    // 3. char ↔ int ↔ long
    if (target->ttype == Type::CHAR && (value->ttype == Type::INT || value->ttype == Type::LONG)) 
        return true;
    if (target->ttype == Type::INT && (value->ttype == Type::CHAR || value->ttype == Type::LONG)) 
        return true;
    if (target->ttype == Type::LONG && is_integral_type(value)) 
        return true;
    
    // 4. int/char/long → float/double
    if ((target->ttype == Type::FLOAT || target->ttype == Type::DOUBLE) && is_integral_type(value)) 
        return true;
    
    // 5. float → double
    if (target->ttype == Type::DOUBLE && value->ttype == Type::FLOAT) 
        return true;
    
    // 6. double → float (narrowing, como en C)
    if (target->ttype == Type::FLOAT && value->ttype == Type::DOUBLE) 
        return true;
    
    return false;
}
```

**TypeChecker.cpp - visit(BinaryOpNode) (promoción aritmética):**
```cpp
// Operaciones aritméticas:
if (is_arithmetic_type(left) && is_arithmetic_type(right)) {
    // Si alguno es double → resultado double
    if (left->ttype == Type::DOUBLE || right->ttype == Type::DOUBLE)
        resultType = doubleType;
    // Si alguno es float → resultado float
    else if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
        resultType = floatType;
    // Si no → int (char promueve a int)
    else
        resultType = intType;
}
```

### Ejemplo de uso (test18_type_promotion.cnn):

```cpp
float y;
int x = 5;
y = x + 0.5;  // x (int) se promueve a float → 5.0 + 0.5 = 5.5

int a = 10;
float b;
b = a;  // int → float (10.0)

bool ok;
ok = (2 + 1.0) == 3.0;  // 2 → double, suma → 3.0
```

### Justificación de diseño

- **Reglas de C:** Familiar para programadores, predecibles
- **Promoción de char a int:** Evita problemas de desbordamiento en operaciones
- **Narrowing permitido en asignación:** Como en C, permite expresividad aunque puede perder precisión
- **Coerción de punteros laxa:** Simplifica manejo de memoria dinámica (void* → int*)

---

## 3. float y double

### Descripción general

C-- soporta:
- `float`: 32 bits (precisión simple)
- `double`: 64 bits (precisión doble)

Ambos siguen el estándar IEEE 754 y usan registros XMM de x86-64.

### Implementación en el Scanner

**Scanner.cpp - Tokens para literales de punto flotante:**
```cpp
// Literales como 3.14, 0.5, 2.0e-3
// Generan FNUM token (literal de punto flotante)
Token* Scanner::number() {
    // Reconoce números con '.' o 'e'/'E' como float/double
}
```

### Implementación en el AST

**ast.h - FloatLiteralNode y tipos:**
```cpp
// Literal de punto flotante (almacenado como double)
class FloatLiteralNode : public Exp {
public:
    double value;
    FloatLiteralNode(double v);
    // ...
};

// Tipos primitivos
class PrimitiveTypeNode : public TypeNode {
public:
    enum Prim { VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, LONG };
    Prim prim;
    // ...
};
```

### Implementación en el TypeChecker

**semantic_types.h - Tipos float/double:**
```cpp
class Type {
public:
    enum TType { NOTYPE, VOID, INT, CHAR, BOOL, FLOAT, DOUBLE, LONG, POINTER, ARRAY, STRUCT };
    TType ttype;
    
    int size() const {
        switch (ttype) {
            case FLOAT:  return 4;  // 32 bits
            case DOUBLE: return 8;  // 64 bits
            // ... otros
        }
    }
};
```

**TypeChecker.cpp - Constructor (singletons para tipos):**
```cpp
TypeChecker::TypeChecker() {
    floatType = new Type(Type::FLOAT);    // 4 bytes
    doubleType = new Type(Type::DOUBLE);  // 8 bytes
    // ...
}
```

### Implementación en el GenCodeVisitor

**Gencode.cpp - Funciones auxiliares para float/double:**
```cpp
// Detectar si es un tipo de punto flotante
static bool isFloatSemanticType(Type* t) {
    return t && (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE);
}

// Cargar float/double en %rax (vía registros XMM)
void GenCodeVisitor::loadBinding(VarDecl* vd) {
    Type* type = vd->resolvedType;
    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movsd " << bindingMem(vd) << ", %xmm7\n";
            out << "  movq %xmm7, %rax\n";
        } else {  // FLOAT
            out << "  movss " << bindingMem(vd) << ", %xmm7\n";
            out << "  movd %xmm7, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }
    // ... carga enteros
}

// Almacenar float/double desde %rax
void GenCodeVisitor::storeBinding(VarDecl* vd) {
    Type* type = vd->resolvedType;
    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movq %rax, %xmm7\n";
            out << "  movsd %xmm7, " << bindingMem(vd) << "\n";
        } else {  // FLOAT
            out << "  movd %eax, %xmm7\n";
            out << "  movss %xmm7, " << bindingMem(vd) << "\n";
        }
        return;
    }
    // ... almacenar enteros
}
```

**Gencode.cpp - Formatos de printf en .data:**
```cpp
void GenCodeVisitor::generate(Program *p) {
    out << ".data\n";
    out << "print_fmt: .string \"%ld\\n\"\n";       // int/bool/ptr
    out << "print_fmt_float: .string \"%f\\n\"\n";  // float/double
    // ...
}
```

### Ejemplo de uso (test20_float_double.cnn):

```cpp
float a = 2.5;
float b = 1.5;
float result = a + b;  // 4.0

double x = 10.0;
double y = 3.0;
double z = x / y;  // ~3.3333333
```

### Justificación de diseño

- **IEEE 754:** Estándar de facto para punto flotante
- **Registros XMM:** x86-64 standard para operaciones de punto flotante
- **Literales como double:** Mayor precisión durante la compilación
- **Convención %rax para resultados:** Todas las expresiones dejan su resultado en %rax, incluso float/double (movsd/movq)

---

## 4. Matrices (arreglos multidimensionales)

### Descripción general

Las matrices multidimensionales en C-- son arreglos de arreglos, almacenados contiguamente en memoria (fila mayor, row-major).

Ejemplo: `int m[2][3]` → 2 filas × 3 columnas = 6 elementos contiguos

### Implementación en el TypeChecker

**semantic_types.h - ArrayType:**
```cpp
class ArrayType : public Type {
public:
    Type* base;
    int length;  // -1 si tamaño desconocido
    
    ArrayType(Type* b, int l = -1) : Type(ARRAY), base(b), length(l) {}
    
    // Igualdad: int[5] == int[3] (mismo base, ignora length)
    bool match(Type* t) const override {
        if (t->ttype != ARRAY) return false;
        ArrayType* at = (ArrayType*)t;
        return base->match(at->base);
    }
    
    // Tamaño total: base->size() * length
    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8;  // arreglo sin tamaño → tratado como puntero
    }
};
```

**TypeChecker.cpp - visit(VarDecl) (creación de arreglos):**
```cpp
// Para int m[2][3] (array_sizes = [2, 3]):
for (int idx = (int)vd->array_sizes.size() - 1; idx >= 0; idx--) {
    // Empezar por la dimensión más interna!
    int dim = -1;
    if (auto* il = dynamic_cast<IntegerLiteralNode*>(vd->array_sizes[idx]))
        dim = (int)il->value;
    ArrayType* at = new ArrayType(t, dim);  // t empieza siendo int
    typeCache.push_back(at);
    t = at;  // t pasa a ser int[3], luego int[2][3]
}
vd->resolvedType = t;  // ArrayType(ArrayType(IntType, 3), 2)
```

### Implementación en el GenCodeVisitor

**Gencode.cpp - arrayDimsFor (extraer dimensiones):**
```cpp
// Para int m[2][3] → ArrayType(ArrayType(IntType, 3), 2)
// → arrayDimsFor(m) = [2, 3]
static vector<int> arrayDimsFor(VarDecl* vd) {
    vector<int> dims;
    Type* t = vd->resolvedType;
    while (t && t->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)t;
        if (at->length > 0) dims.push_back(at->length);
        t = at->base;
    }
    return dims;
}
```

**Gencode.cpp - Acceso a arreglos (IndexNode):**
```cpp
// Para m[i][j]:
// 1. Cálculo de offset: (i * ncols + j) * sizeof(int)
// 2. Dirección: base + offset
// 3. Cargar/almacenar desde esa dirección
```

### Ejemplo de uso (test17_multidim_arrays.cnn):

```cpp
int m[2][3];
m[0][0] = 1; m[0][1] = 2; m[0][2] = 3;
m[1][0] = 4; m[1][1] = 5; m[1][2] = 6;

int sum = m[0][0] + m[1][2];  // 1 + 6 = 7
```

### Layout en memoria

```
m[0][0]  m[0][1]  m[0][2]  m[1][0]  m[1][1]  m[1][2]
┌────────┬────────┬────────┬────────┬────────┬────────┐
│  1     │  2     │  3     │  4     │  5     │  6     │
└────────┴────────┴────────┴────────┴────────┴────────┘
```

### Justificación de diseño

- **Arreglos de arreglos:** Compatible con C, simplifica la implementación
- **Row-major:** Same as C, optimiza locality de caché (accesos consecutivos a filas)
- **ArrayType anidado:** Representación clara de dimensiones múltiples
- **match() ignora length:** Permite pasar int[5] a función que espera int[]

---

## 5. const

### Descripción general

El modificador `const` declara variables de solo lectura que no se pueden modificar después de la inicialización.

### Implementación en el AST

**ast.h - VarDecl y TypeNode con const:**
```cpp
class VarDecl : public Stm {
public:
    bool isConst;  // true si la variable es const
    // ...
};

class TypeNode : public Exp {
public:
    bool isConst;  // modificador const en el tipo (const int)
    // ...
};
```

### Implementación en el Parser

**Parser.cpp - parse_type() reconoce `const`:**
```cpp
TypeNode* Parser::parse_type() {
    bool isConst = false;
    if (match(CONST)) {
        isConst = true;
    }
    // ... parsear el tipo base
    type->isConst = isConst;
    return type;
}
```

### Implementación en el TypeChecker

**semantic_types.h - Type con isConst:**
```cpp
class Type {
public:
    bool isConst;  // flag para const
    // ...
};
```

**TypeChecker.cpp - check_assign y visit(AssignmentNode):**
```cpp
void TypeChecker::visit(AssignmentNode* e) {
    // ...
    Type* targetType = e->target->resolvedType;
    if (targetType->isConst) {
        error("no se puede asignar a una variable declarada como const.");
    }
    // ...
}
```

### Ejemplo de uso (test23_const_void.cnn):

```cpp
const int a = 5;  // ok: inicializado
int b = a;        // ok: leer const
// a = 10;        // error: no se puede modificar const

const long long ll = 1000000;
const char c = 'Z';
const unsigned u = 255;
```

### Justificación de diseño

- **const como flag booleano:** Simple de implementar
- **Verificación en asignación:** Captura intentos de modificar const en tiempo de compilación
- **Inicialización obligatoria:** Como en C, garantiza que la variable const tenga un valor
- **Propagación de const en tipos:** const int* y int* const son manejables con TypeNode::isConst

---

## 6. void

### Descripción general

`void` es un tipo que representa la ausencia de valor. Se usa principalmente como:
- Tipo de retorno de funciones sin retorno
- Tipo base de punteros genéricos (void*)

### Implementación en el TypeChecker

**semantic_types.h - Tipo VOID:**
```cpp
class Type {
public:
    enum TType { NOTYPE, VOID, INT, ... };
    // ...
    int size() const {
        case VOID: return 0;  // void no tiene tamaño
    }
};
```

**TypeChecker.cpp - visit(FunDecl):**
```cpp
void TypeChecker::visit(FunDecl* f) {
    Type* returnType = type_from_ast(f->return_type);
    // ... verificaciones de retorno
}
```

**TypeChecker.cpp - visit(ReturnStmt):**
```cpp
void TypeChecker::visit(ReturnStmt* e) {
    if (retornodefuncion->match(voidType) && e->expr) {
        error("función void no debe retornar valor.");
        return;
    }
    if (!retornodefuncion->match(voidType) && !e->expr) {
        error("función no-void debe retornar un valor.");
        return;
    }
    // ...
}
```

**TypeChecker.cpp - visit(VarDecl):**
```cpp
void TypeChecker::visit(VarDecl* vd) {
    Type* t = type_from_ast(vd->type);
    if (t->match(voidType)) {
        error("no se puede declarar variable de tipo void.");
        return;
    }
    // ...
}
```

### Ejemplo de uso (test23_const_void.cnn):

```cpp
// Función void no retorna valor
void print_three() {
    printf(3);
}

int main() {
    print_three();  // llamada a función void
    
    // void*: puntero genérico (soportado via PointerType con base void)
    void* p = malloc(4);
    int* q = p;  // coerción de void* a int* (como en C)
    
    return 0;
}
```

### Justificación de diseño

- **void como tipo singleton:** Un solo objeto representando void
- **size() = 0:** Lógico, void no ocupa espacio
- **Restricciones de uso:** No variables void, funciones void no retornan valor
- **void* como puntero genérico:** Compatible con malloc()/free() y C

---

## Resumen de Arquitectura

Todas estas características se integran en el pipeline del compilador:

1. **Scanner** → Tokeniza const, void, *, &, ->, literales de punto flotante
2. **Parser** → Construye AST con TypeNode (PointerTypeNode, UnaryOpNode, ArrowAccessNode, FloatLiteralNode, etc.)
3. **TypeChecker** → Convierte TypeNode → Type* (type_from_ast), verifica tipos, resuelve PointerType/ArrayType, verifica const, realiza promoción
4. **GenCodeVisitor** → Emite código x86-64 usando Type* (resolvedType), registros XMM para float/double, operaciones de punteros, accesos a arreglos

---

## Referencias al código fuente

- **ast.h**: Definición de nodos del AST (PointerTypeNode, UnaryOpNode, ArrowAccessNode, etc.)
- **semantic_types.h**: Type, PointerType, ArrayType con lógica de igualdad y tamaños
- **TypeChecker.cpp**: Verificación de tipos, promoción, check_assign
- **Gencode.cpp**: Generación de código x86-64, manejo de float/double en registros XMM
- **tests/integracion/**: Tests para cada característica (test11, test17, test18, test20, test23)
