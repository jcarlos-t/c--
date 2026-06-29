# Soporte para Tipos - Versión 0

## Resumen de Cambios

Esta implementación agrega soporte para diferentes tamaños de tipos (int, bool, char, float, punteros) con optimización de memoria mediante bin packing en el stack frame.

---

## 1. semantic_types.h

### Cambio: Renombrar campo `size` a `length` en ArrayType

**Motivo:** Conflicto de nombres entre el campo de datos `size` y el método `size()` que calcula el tamaño en bytes.

**Antes:**
```cpp
class ArrayType : public Type {
public:
    Type* base;
    int size; // -1 si tamaño desconocido
    ArrayType(Type* b, int s = -1) : Type(ARRAY), base(b), size(s) {}
    
    int size() const override {
        if (size >= 0) return base->size() * size;
        return 8;
    }
};
```

**Después:**
```cpp
class ArrayType : public Type {
public:
    Type* base;
    int length; // -1 si tamaño desconocido
    ArrayType(Type* b, int l = -1) : Type(ARRAY), base(b), length(l) {}
    
    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8;
    }
};
```

### Adición: Método `size()` en clases Type

Se agregó el método virtual `size()` a todas las clases de tipos semánticos:

```cpp
class Type {
public:
    virtual int size() const {
        switch (ttype) {
            case VOID:   return 0;
            case INT:    return 4;
            case BOOL:   return 1;
            case FLOAT:  return 4;
            case POINTER: return 8;
            default:     return 8;
        }
    }
};

class ArrayType : public Type {
public:
    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8;
    }
};

class StructType : public Type {
public:
    int size() const override {
        int total = 0;
        for (auto& pair : members) {
            total += pair.second->size();
        }
        return total;
    }
};
```

---

## 2. ast.h

### Cambio: Agregar campos a VarDecl

**Motivo:** Almacenar información de tipo y offset calculados por TypeChecker.

**Antes:**
```cpp
class VarDecl : public Stm {
public:
    Exp* type;
    string name;
    vector<Exp*> array_sizes;
    Exp* initializer;
    
    VarDecl(Exp* t, const string& n);
    // ...
};
```

**Después:**
```cpp
class VarDecl : public Stm {
public:
    Exp* type;
    string name;
    vector<Exp*> array_sizes;
    Exp* initializer;
    
    // Calculados por TypeChecker
    Type* resolvedType = nullptr;  // tipo semántico resuelto
    int offset = 0;      // offset desde %rbp (negativo)
    int memSize = 0;     // tamaño en bytes
    
    VarDecl(Exp* t, const string& n);
    // ...
};
```

### Cambio: Agregar campo frameSize a FunDecl

**Motivo:** Almacenar el tamaño total del stack frame calculado por TypeChecker.

**Antes:**
```cpp
class FunDecl {
public:
    Location loc;
    Exp* return_type;
    string name;
    vector<VarDecl*> params;
    Body* body;
    bool is_template;
    
    // ...
};
```

**Después:**
```cpp
class FunDecl {
public:
    Location loc;
    Exp* return_type;
    string name;
    vector<VarDecl*> params;
    Body* body;
    bool is_template;
    
    // Calculados por TypeChecker
    int frameSize = 0;   // tamaño total del frame
    
    // ...
};
```

### Cambio: Agregar campos a StructDecl

**Motivo:** Almacenar offsets de miembros y tamaño total del struct.

**Antes:**
```cpp
class StructDecl {
public:
    Location loc;
    string name;
    vector<VarDecl*> members;
    
    // ...
};
```

**Después:**
```cpp
class StructDecl {
public:
    Location loc;
    string name;
    vector<VarDecl*> members;
    
    // Calculados por TypeChecker
    unordered_map<string, int> memberOffsets;  // offset de cada miembro
    int totalSize = 0;                          // tamaño total del struct
    
    // ...
};
```

---

## 3. visitor.h

### Cambio: Agregar métodos y campos a TypeChecker

**Motivo:** Implementar bin packing para optimizar el uso de memoria.

**Antes:**
```cpp
class TypeChecker : public TypeVisitor {
private:
    // ... campos existentes ...
    int loopDepth;
    int switchDepth;
    vector<string> errors;
    bool hasError;
    
    // ... métodos ...
};
```

**Después:**
```cpp
class TypeChecker : public TypeVisitor {
private:
    // ... campos existentes ...
    int loopDepth;
    int switchDepth;
    vector<string> errors;
    bool hasError;
    int currentOffset = 0;  // offset actual para variables locales
    
    // ... métodos existentes ...
    
    // Bin packing: recolectar variables y calcular offsets optimizados
    void collectVars(Stm* stmt, vector<VarDecl*>& vars);
    void assignOffsetsWithBinPacking(vector<VarDecl*>& vars, int startOffset);
};
```

### Cambio: Agregar métodos y campos a GenCodeVisitor

**Motivo:** Soporte para instrucciones de diferentes tamaños según el tipo.

**Antes:**
```cpp
class GenCodeVisitor : public CodeGenVisitor {
private:
    ostream &out;
    LVal *lvalTarget = nullptr;
    LVal captureLVal(Exp *e);
    void storeTarget(const LVal &lv);
    string varMem(const string& name);
    void loadVar(const string& name);
    void storeVar(const string& name);
    void leaVar(const string& name);
    
    unordered_map<string, int> memoria;
    unordered_map<string, bool> memoriaGlobal;
    // ... otros campos ...
};
```

**Después:**
```cpp
class GenCodeVisitor : public CodeGenVisitor {
private:
    ostream &out;
    LVal *lvalTarget = nullptr;
    LVal captureLVal(Exp *e);
    void storeTarget(const LVal &lv);
    string varMem(const string& name);
    void loadVar(const string& name);
    void storeVar(const string& name);
    void leaVar(const string& name);
    
    // Helpers para instrucciones según tamaño
    string instrSuffix(int size);
    string loadInstr(int size);   // movzbq, movslq, movq
    string storeInstr(int size);  // movb, movl, movq
    
    unordered_map<string, int> memoria;
    unordered_map<string, bool> memoriaGlobal;
    unordered_map<string, int> variableSizes;  // tamaño de cada variable
    // ... otros campos ...
};
```

---

## 4. TypeChecker.cpp

### Cambio: Implementar bin packing en visit(FunDecl*)

**Motivo:** Optimizar el uso de memoria agrupando variables por tamaño.

**Implementación:**

```cpp
void TypeChecker::visit(FunDecl* f) {
    env.add_level();
    
    // Recolectar parámetros
    vector<VarDecl*> params;
    for (auto p : f->params) {
        Type* t = type_from_ast(p->type);
        if (t->match(voidType))
            error("parámetro no puede ser de tipo void.");
        p->memSize = t->size();
        p->resolvedType = t;
        params.push_back(p);
        env.add_var(p->name, t);
    }
    
    // Asignar offsets a parámetros con bin packing
    assignOffsetsWithBinPacking(params, 0);
    int paramSize = 0;
    for (auto p : params) {
        int end = p->offset + p->memSize;
        if (end > paramSize) paramSize = end;
    }
    
    // Recolectar todas las variables locales del body
    vector<VarDecl*> localVars;
    collectVars(f->body, localVars);
    
    // Verificar tipos de variables locales
    for (auto v : localVars) {
        v->accept(this);
    }
    
    // Asignar offsets a variables locales con bin packing
    assignOffsetsWithBinPacking(localVars, paramSize);
    
    // Calcular tamaño total usado
    int totalSize = paramSize;
    for (auto v : localVars) {
        int end = v->offset + v->memSize;
        if (end > totalSize) totalSize = end;
    }
    
    // Convertir offsets a negativos (stack frame)
    for (auto p : params) {
        p->offset = -(p->offset + 8);
    }
    for (auto v : localVars) {
        v->offset = -(v->offset + 8);
    }
    
    retornodefuncion = type_from_ast(f->return_type);
    f->body->accept(this);
    
    // ... validación de retorno ...
    
    // Calcular frame size total (alineado a 16 bytes)
    f->frameSize = (totalSize + 15) & ~15;
    
    env.remove_level();
}
```

### Adición: Método collectVars()

**Motivo:** Recolectar recursivamente todas las variables declaradas en un statement.

```cpp
void TypeChecker::collectVars(Stm* stmt, vector<VarDecl*>& vars) {
    if (!stmt) return;
    
    if (auto* v = dynamic_cast<VarDecl*>(stmt)) {
        vars.push_back(v);
    }
    
    if (auto* body = dynamic_cast<Body*>(stmt)) {
        for (auto s : body->stmts) {
            collectVars(s, vars);
        }
    }
    
    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        collectVars(ifStmt->then_branch, vars);
        if (ifStmt->else_branch) collectVars(ifStmt->else_branch, vars);
    }
    
    if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        collectVars(whileStmt->body, vars);
    }
    
    if (auto* forStmt = dynamic_cast<ForStmt*>(stmt)) {
        if (forStmt->init) collectVars(forStmt->init, vars);
        collectVars(forStmt->body, vars);
    }
    
    if (auto* doWhileStmt = dynamic_cast<DoWhileStmt*>(stmt)) {
        collectVars(doWhileStmt->body, vars);
    }
    
    if (auto* switchStmt = dynamic_cast<SwitchStmt*>(stmt)) {
        for (auto cc : switchStmt->cases) {
            for (auto s : cc->body) {
                collectVars(s, vars);
            }
        }
        for (auto s : switchStmt->default_body) {
            collectVars(s, vars);
        }
    }
}
```

### Adición: Método assignOffsetsWithBinPacking()

**Motivo:** Asignar offsets optimizados agrupando variables por tamaño en slots de 8 bytes.

```cpp
void TypeChecker::assignOffsetsWithBinPacking(vector<VarDecl*>& vars, int startOffset) {
    if (vars.empty()) return;
    
    // Ordenar por tamaño descendente (8, 4, 1)
    sort(vars.begin(), vars.end(), [](VarDecl* a, VarDecl* b) {
        return a->memSize > b->memSize;
    });
    
    // Estructura para trackear slots
    struct Slot {
        int used = 0;  // bytes usados en este slot
    };
    vector<Slot> slots;
    slots.push_back(Slot());  // primer slot
    
    // Asignar cada variable al primer slot donde quepa
    for (auto v : vars) {
        int size = v->memSize;
        bool placed = false;
        
        for (size_t i = 0; i < slots.size(); i++) {
            // Verificar alineación
            int align = size;
            if (align > 8) align = 8;
            
            // Calcular offset dentro del slot con alineación
            int offsetInSlot = slots[i].used;
            if (offsetInSlot % align != 0) {
                offsetInSlot += align - (offsetInSlot % align);
            }
            
            // Si cabe en este slot
            if (offsetInSlot + size <= 8) {
                v->offset = startOffset + i * 8 + offsetInSlot;
                slots[i].used = offsetInSlot + size;
                placed = true;
                break;
            }
        }
        
        // Si no cabe en ningún slot, crear uno nuevo
        if (!placed) {
            Slot newSlot;
            v->offset = startOffset + slots.size() * 8;
            newSlot.used = size;
            slots.push_back(newSlot);
        }
    }
}
```

### Cambio: Simplificar visit(VarDecl*)

**Motivo:** El offset ya se calcula con bin packing en visit(FunDecl*).

**Antes:**
```cpp
void TypeChecker::visit(VarDecl* v) {
    Type* t = type_from_ast(v->type);
    // ... validaciones ...
    
    v->memSize = t->size();
    currentOffset += v->memSize;
    v->offset = currentOffset;
    
    env.add_var(v->name, t);
    // ... verificación de inicializador ...
}
```

**Después:**
```cpp
void TypeChecker::visit(VarDecl* v) {
    // Si ya tiene resolvedType, fue procesado por visit(FunDecl*)
    if (v->resolvedType != nullptr) {
        // Solo verificar inicializador
        if (v->initializer) {
            Type* initType = v->initializer->accept(this);
            if (!check_assign(v->resolvedType, initType)) {
                error("tipos incompatibles en inicialización de '" + v->name + "'.");
            }
        }
        return;
    }
    
    Type* t = type_from_ast(v->type);
    // ... validaciones ...
    
    v->resolvedType = t;
    v->memSize = t->size();
    
    env.add_var(v->name, t);
    // ... verificación de inicializador ...
}
```

### Cambio: Actualizar visit(StructDecl*)

**Motivo:** Calcular offsets de miembros y tamaño total del struct.

**Antes:**
```cpp
void TypeChecker::visit(StructDecl* s) {
    StructType* st = new StructType(s->name);
    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        st->members[m->name] = mt;
    }
    struct_types[s->name] = st;
}
```

**Después:**
```cpp
void TypeChecker::visit(StructDecl* s) {
    StructType* st = new StructType(s->name);
    int offset = 0;
    
    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        st->members[m->name] = mt;
        s->memberOffsets[m->name] = offset;
        offset += mt->size();
    }
    
    s->totalSize = offset;
    struct_types[s->name] = st;
}
```

---

## 5. Gencode.cpp

### Adición: Helpers para instrucciones según tamaño

**Motivo:** Generar instrucciones de movimiento del tamaño correcto según el tipo.

```cpp
// Helper: sufijo de instrucción según tamaño
string GenCodeVisitor::instrSuffix(int size) {
    switch (size) {
        case 1: return "b";
        case 4: return "l";
        case 8: return "q";
        default: return "q";
    }
}

// Helper: instrucción de carga según tamaño
string GenCodeVisitor::loadInstr(int size) {
    switch (size) {
        case 1: return "movzbq";  // zero-extend 1 byte -> 8 bytes
        case 4: return "movslq";  // sign-extend 4 bytes -> 8 bytes
        case 8: return "movq";
        default: return "movq";
    }
}

// Helper: instrucción de almacenamiento según tamaño
string GenCodeVisitor::storeInstr(int size) {
    switch (size) {
        case 1: return "movb";
        case 4: return "movl";
        case 8: return "movq";
        default: return "movq";
    }
}
```

### Cambio: Actualizar loadVar() y storeVar()

**Motivo:** Usar instrucciones del tamaño correcto según el tipo de variable.

**Antes:**
```cpp
void GenCodeVisitor::loadVar(const string& name) {
    out << "  movq " << varMem(name) << ", %rax\n";
}

void GenCodeVisitor::storeVar(const string& name) {
    out << "  movq %rax, " << varMem(name) << "\n";
}
```

**Después:**
```cpp
void GenCodeVisitor::loadVar(const string& name) {
    int size = 8;  // default
    if (variableSizes.count(name)) {
        size = variableSizes[name];
    }
    out << "  " << loadInstr(size) << " " << varMem(name) << ", %rax\n";
}

void GenCodeVisitor::storeVar(const string& name) {
    int size = 8;  // default
    if (variableSizes.count(name)) {
        size = variableSizes[name];
    }
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
    out << "  " << storeInstr(size) << " " << reg << ", " << varMem(name) << "\n";
}
```

### Cambio: Actualizar visit(FunDecl*)

**Motivo:** Usar offsets negativos calculados por TypeChecker y guardar tamaños de variables.

**Antes:**
```cpp
void GenCodeVisitor::visit(FunDecl *d) {
    inFunction = true;
    memoria.clear();
    funcName = d->name;
    int paramCount = (int)d->params.size();
    offset = 0;
    
    // ... calcular frameSize ...
    
    for (int i = 0; i < paramCount && i < 6; i++) {
        offset -= 8;
        memoria[d->params[i]->name] = offset;
        out << "  movq " << argRegs[i] << ", " << offset << "(%rbp)\n";
    }
    
    // ... generar código ...
}
```

**Después:**
```cpp
void GenCodeVisitor::visit(FunDecl *d) {
    inFunction = true;
    memoria.clear();
    variableSizes.clear();
    funcName = d->name;
    
    int frameSize = d->frameSize;  // calculado por TypeChecker
    
    out << "\n.globl " << d->name << "\n";
    out << d->name << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";
    
    const vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    for (size_t i = 0; i < d->params.size() && i < 6; i++) {
        memoria[d->params[i]->name] = d->params[i]->offset;  // ya es negativo
        variableSizes[d->params[i]->name] = d->params[i]->memSize;
        
        int size = d->params[i]->memSize;
        string destReg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
        out << "  " << storeInstr(size) << " " << destReg << ", " 
            << d->params[i]->offset << "(%rbp)\n";
    }
    
    d->body->accept(this);
    
    out << ".end_" << d->name << ":\n";
    out << "  leave\n";
    out << "  ret\n";
    inFunction = false;
}
```

### Cambio: Actualizar visit(VarDecl*)

**Motivo:** Usar offset negativo calculado por TypeChecker y guardar tamaño de variable.

**Antes:**
```cpp
void GenCodeVisitor::visit(VarDecl *d) {
    if (!inFunction) {
        memoriaGlobal[d->name] = true;
        return;
    }
    
    offset -= 8;
    memoria[d->name] = offset;
    
    // ... resto del código ...
}
```

**Después:**
```cpp
void GenCodeVisitor::visit(VarDecl *d) {
    if (!inFunction) {
        memoriaGlobal[d->name] = true;
        return;
    }
    
    memoria[d->name] = d->offset;  // ya es negativo
    variableSizes[d->name] = d->memSize;
    
    // ... resto del código ...
}
```

### Cambio: Actualizar operaciones aritméticas en visit(BinaryOpNode*)

**Motivo:** Preparar para usar sufijos de instrucción según tamaño (actualmente usa "q" por defecto).

**Nota:** Las operaciones aritméticas actualmente usan sufijo "q" (8 bytes) por defecto. En el futuro se puede optimizar para usar el tamaño correcto según los tipos de los operandos.

---

## 6. Bin Packing - Ejemplo de Funcionamiento

### Ejemplo con variables de diferentes tamaños

Supongamos una función con las siguientes variables:
- `int a` (4 bytes)
- `bool b` (1 byte)
- `int c` (4 bytes)
- `char d` (1 byte)
- `int* e` (8 bytes)

### Sin bin packing (antes):
```
Offset 0-3:   a (4 bytes)
Offset 4-7:   b (1 byte) + padding (3 bytes)
Offset 8-11:  c (4 bytes)
Offset 12-15: d (1 byte) + padding (3 bytes)
Offset 16-23: e (8 bytes)
Total: 24 bytes
```

### Con bin packing (después):
```
Slot 0 (8 bytes): [a (4 bytes)][b (1 byte)][c (3 bytes de 4)]
Slot 1 (8 bytes): [c (1 byte restante)][d (1 byte)][padding (6 bytes)]
Slot 2 (8 bytes): [e (8 bytes)]
Total: 24 bytes (3 slots)
```

**Mejor agrupación:**
```
Slot 0 (8 bytes): [e (8 bytes)]
Slot 1 (8 bytes): [a (4 bytes)][c (4 bytes)]
Slot 2 (8 bytes): [b (1 byte)][d (1 byte)][padding (6 bytes)]
Total: 24 bytes (3 slots)
```

El bin packing ordena las variables por tamaño descendente y las agrupa para minimizar el desperdicio de memoria.

---

## 7. Consideraciones Importantes

### Alineación del Stack

- El System V AMD64 ABI requiere que `%rsp` esté alineado a 16 bytes antes de cada `call`
- El frame size se calcula como `(totalSize + 15) & ~15` para garantizar esta alineación
- Los offsets se convierten a negativos: `offset = -(offset + 8)` para evitar sobrescribir el `%rbp` guardado en `0(%rbp)`

### Extensiones de Signo

- `movzbq`: carga 1 byte y extiende con zeros a 8 bytes (para bool/char unsigned)
- `movslq`: carga 4 bytes y extiende con signo a 8 bytes (para int)
- `movq`: carga 8 bytes (para punteros y long)

### Operaciones Aritméticas

Actualmente las operaciones aritméticas usan sufijo "q" (8 bytes) por defecto. Esto funciona correctamente porque:
1. Los valores se cargan con sign-extend (`movslq`) o zero-extend (`movzbq`)
2. Las operaciones en 8 bytes producen el resultado correcto
3. El resultado se almacena truncando al tamaño correcto (`movb`, `movl`, `movq`)

En el futuro se puede optimizar para usar operaciones del tamaño correcto (`addl`, `addb`, etc.) si es necesario.

---

## 8. Archivos Modificados

1. `semantic_types.h` - Renombrar `size` a `length` en ArrayType, agregar método `size()`
2. `ast.h` - Agregar campos `resolvedType`, `offset`, `memSize` a VarDecl; `frameSize` a FunDecl; `memberOffsets`, `totalSize` a StructDecl
3. `visitor.h` - Agregar métodos y campos a TypeChecker y GenCodeVisitor
4. `TypeChecker.cpp` - Implementar bin packing, calcular offsets y tamaños
5. `Gencode.cpp` - Soporte para instrucciones de diferentes tamaños, usar offsets negativos

---

## 9. Estado Actual

- ✅ Bin packing implementado
- ✅ Soporte para diferentes tamaños de tipos (1, 4, 8 bytes)
- ✅ Instrucciones de carga/almacenamiento según tamaño
- ✅ Offsets negativos para evitar sobrescribir `%rbp`
- ✅ Alineación de stack a 16 bytes
- ⚠️ Operaciones aritméticas usan 8 bytes por defecto (funciona pero no óptimo)
- ⚠️ Lambdas mantienen el sistema antiguo (8 bytes por variable)

---

## 10. Próximos Pasos (Opcional)

1. Optimizar operaciones aritméticas para usar el tamaño correcto según los operandos
2. Implementar bin packing también para lambdas
3. Agregar padding en structs para alineación estricta de miembros
4. Soporte para promociones de tipo en operaciones mixtas (int + float)
