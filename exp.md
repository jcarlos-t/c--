# Gestión de Tipos Básicos y Templates en C--

Este documento explica los flujos que sigue el compilador C-- para la gestión de tipos básicos y templates, con ejemplos concretos.

---

## 1. Gestión de Tipos Básicos

### 1.1 Jerarquía de Tipos Semánticos

El compilador distingue entre dos representaciones de tipos:

- **TypeNode (AST)**: Representa tipos en el código fuente (sintaxis)
- **Type* (semántico)**: Representa tipos resueltos tras análisis semántico

La jerarquía de tipos semánticos está definida en `semantic_types.h`:

```
Type (base)
├── INT, CHAR, BOOL, FLOAT, DOUBLE, VOID (primitivos)
├── PointerType (punteros T*)
├── ArrayType (arreglos T[n])
└── StructType (structs con nombre)
```

### 1.2 Flujo de Resolución de Tipos Primitivos

**Paso 1: Creación de Singletons en el Constructor**

```cpp
// TypeChecker::TypeChecker()
intType = new Type(Type::INT);
charType = new Type(Type::CHAR);
floatType = new Type(Type::FLOAT);
doubleType = new Type(Type::DOUBLE);
boolType = new Type(Type::BOOL);
voidType = new Type(Type::VOID);
```

Estos objetos se reutilizan durante todo el typechecking (no se crean nuevos para cada uso).

**Paso 2: Conversión de TypeNode a Type* (type_from_ast)**

```cpp
Type* TypeChecker::type_from_ast(Exp* t) {
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(t)) {
        switch (pt->prim) {
            case PrimitiveTypeNode::INT:    return intType;
            case PrimitiveTypeNode::CHAR:   return charType;
            case PrimitiveTypeNode::FLOAT:  return floatType;
            case PrimitiveTypeNode::DOUBLE: return doubleType;
            case PrimitiveTypeNode::BOOL:   return boolType;
            case PrimitiveTypeNode::VOID:   return voidType;
        }
    }
    // ... otros casos
}
```

**Ejemplo:**

```c
// Código fuente
int x = 5;
float y = 3.14;
```

```cpp
// AST generado
VarDecl {
    type: PrimitiveTypeNode(INT),
    name: "x",
    initializer: IntegerLiteralNode(5)
}

VarDecl {
    type: PrimitiveTypeNode(FLOAT),
    name: "y",
    initializer: FloatLiteralNode(3.14)
}

// Después de type_from_ast()
x->resolvedType = intType    // Type{ttype: INT}
y->resolvedType = floatType  // Type{ttype: FLOAT}
```

### 1.3 Flujo de Resolución de Punteros

**Paso 1: Detección de PointerTypeNode**

```cpp
if (auto* pt = dynamic_cast<PointerTypeNode*>(t)) {
    Type* base = type_from_ast(pt->base);  // recursivo
    PointerType* ptr = new PointerType(base);
    typeCache.push_back(ptr);  // guardar para limpieza
    return ptr;
}
```

**Paso 2: Creación de PointerType con Tipo Base**

```cpp
class PointerType : public Type {
public:
    Type* base;  // tipo apuntado
    PointerType(Type* b) : Type(POINTER), base(b) {}
    
    bool match(Type* t) const override {
        if (t->ttype != POINTER) return false;
        PointerType* pt = (PointerType*)t;
        return base->match(pt->base);  // comparación recursiva
    }
};
```

**Ejemplo:**

```c
// Código fuente
int* p;
char** pp;
void* generic;
```

```cpp
// AST
PointerTypeNode(PrimitiveTypeNode(INT))     // int*
PointerTypeNode(PointerTypeNode(PrimitiveTypeNode(CHAR)))  // char**
PointerTypeNode(PrimitiveTypeNode(VOID))    // void*

// Después de type_from_ast()
p->resolvedType = PointerType{base: intType}
pp->resolvedType = PointerType{base: PointerType{base: charType}}
generic->resolvedType = PointerType{base: voidType}
```

### 1.4 Flujo de Resolución de Arreglos

**Paso 1: Detección de Dimensiones en VarDecl**

```cpp
// En visit(VarDecl)
for (auto s : v->array_sizes) {
    int dim = -1;
    if (auto* il = dynamic_cast<IntegerLiteralNode*>(s))
        dim = (int)il->value;
    ArrayType* at = new ArrayType(t, dim);
    typeCache.push_back(at);
    t = at;  // anidar para multidimensional
}
```

**Paso 2: Creación de ArrayType**

```cpp
class ArrayType : public Type {
public:
    Type* base;
    int length;  // -1 si desconocido (int a[])
    
    bool match(Type* t) const override {
        if (t->ttype != ARRAY) return false;
        ArrayType* at = (ArrayType*)t;
        return base->match(at->base);  // ignora length
    }
    
    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8;  // tratado como puntero
    }
};
```

**Ejemplo:**

```c
// Código fuente
int arr[10];
int matrix[2][3];
int flexible[];
```

```cpp
// AST
VarDecl {
    type: PrimitiveTypeNode(INT),
    name: "arr",
    array_sizes: [IntegerLiteralNode(10)]
}

VarDecl {
    type: PrimitiveTypeNode(INT),
    name: "matrix",
    array_sizes: [IntegerLiteralNode(2), IntegerLiteralNode(3)]
}

// Después de type_from_ast()
arr->resolvedType = ArrayType{base: intType, length: 10}
matrix->resolvedType = ArrayType{
    base: ArrayType{base: intType, length: 3},
    length: 2
}
flexible->resolvedType = ArrayType{base: intType, length: -1}
```

### 1.5 Flujo de Resolución de Structs

**Paso 1: Registro de StructDecl**

```cpp
void TypeChecker::visit(StructDecl* s) {
    StructType* st = new StructType(s->name);
    int offset = 0;
    
    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        st->members[m->name] = mt;
        s->memberOffsets[m->name] = offset;
        s->memberSizes[m->name] = mt->size();
        offset += mt->size();
    }
    
    s->totalSize = offset;
    struct_types[s->name] = st;  // registrar globalmente
}
```

**Paso 2: Uso de StructType**

```cpp
class StructType : public Type {
public:
    string name;
    unordered_map<string, Type*> members;
    
    bool match(Type* t) const override {
        if (t->ttype != STRUCT) return false;
        StructType* st = (StructType*)t;
        return name == st->name;  // equivalencia nominal
    }
};
```

**Ejemplo:**

```c
// Código fuente
struct Point {
    int x;
    float y;
};

struct Point p;
```

```cpp
// AST
StructDecl {
    name: "Point",
    members: [
        VarDecl{type: PrimitiveTypeNode(INT), name: "x"},
        VarDecl{type: PrimitiveTypeNode(FLOAT), name: "y"}
    ]
}

StructTypeNode("Point")

// Después de visit(StructDecl)
struct_types["Point"] = StructType{
    name: "Point",
    members: {"x": intType, "y": floatType}
}

// Después de type_from_ast() para StructTypeNode
p->resolvedType = struct_types["Point"]
```

---

## 2. Gestión de Templates

### 2.1 Declaración de Templates

Los templates se declaran con la sintaxis:

```c
template<typename T>
struct Par {
    T first;
    T second;
};

template<typename T>
T identidad(T x) {
    return x;
}
```

**AST Generado:**

```cpp
TemplateDecl {
    params: ["T"],
    struct_decl: StructDecl {
        name: "Par",
        members: [
            VarDecl{type: NamedTypeNode("T"), name: "first"},
            VarDecl{type: NamedTypeNode("T"), name: "second"}
        ]
    }
}

TemplateDecl {
    params: ["T"],
    func: FunDecl {
        return_type: NamedTypeNode("T"),
        name: "identidad",
        params: [VarDecl{type: NamedTypeNode("T"), name: "x"}]
    }
}
```

**Registro:**

```cpp
void TypeChecker::visit(TemplateDecl* t) {
    template_decls[t->func ? t->func->name : t->struct_decl->name] = t;
}
```

### 2.2 Instanciación de Struct Templates

**Flujo Completo:**

**Paso 1: Detección de TemplateTypeNode**

```cpp
if (auto* tt = dynamic_cast<TemplateTypeNode*>(t)) {
    return instantiate_template(tt->name, tt->type_args);
}
```

**Paso 2: Generación de Nombre Mangleado**

```cpp
string mangleTemplateName(const string& base, const vector<TypeNode*>& args) {
    string result = base + "<";
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) result += ",";
        if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(args[i])) {
            switch (pt->prim) {
                case PrimitiveTypeNode::INT: result += "int"; break;
                case PrimitiveTypeNode::FLOAT: result += "float"; break;
                // ...
            }
        }
    }
    result += ">";
    return result;
}
```

**Paso 3: Verificación de Caché**

```cpp
StructType* TypeChecker::instantiate_template(const string& name, const vector<TypeNode*>& args) {
    string concrete_name = mangleTemplateName(name, args);
    
    // Si ya existe, reusar
    auto cit = struct_types.find(concrete_name);
    if (cit != struct_types.end()) return cit->second;
    
    // ... continuar con instanciación
}
```

**Paso 4: Construcción de Mapa de Sustitución**

```cpp
unordered_map<string, Type*> subs;
for (size_t i = 0; i < tdecl->params.size() && i < args.size(); i++) {
    subs[tdecl->params[i]] = type_from_ast(args[i]);
}
// Ejemplo: subs["T"] = intType
```

**Paso 5: Sustitución Recursiva de Tipos**

```cpp
function<Type*(Exp*)> substitute = [&](Exp* ast_type) -> Type* {
    // Caso 1: NamedTypeNode → buscar en mapa de sustitución
    if (auto* idt = dynamic_cast<NamedTypeNode*>(ast_type)) {
        auto sit = subs.find(idt->name);
        if (sit != subs.end()) return sit->second;
        error("tipo de template '" + idt->name + "' no reconocido.");
        return intType;
    }
    
    // Caso 2: PointerTypeNode → sustituir base recursivamente
    if (auto* ptr = dynamic_cast<PointerTypeNode*>(ast_type)) {
        Type* base = substitute(ptr->base);
        PointerType* result = new PointerType(base);
        typeCache.push_back(result);
        return result;
    }
    
    // Caso 3: PrimitiveTypeNode → convertir directamente
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(ast_type)) {
        return type_from_ast(pt);
    }
    
    return type_from_ast(ast_type);
};
```

**Paso 6: Creación de StructType Concreto**

```cpp
StructType* st = new StructType(concrete_name);
for (auto member : orig->members) {
    Type* mt = substitute(member->type);
    // Wrap en ArrayType si tiene dimensiones
    for (size_t d = 0; d < member->array_sizes.size(); d++) {
        ArrayType* at = new ArrayType(mt, -1);
        typeCache.push_back(at);
        mt = at;
    }
    st->members[member->name] = mt;
}
struct_types[concrete_name] = st;
return st;
```

**Ejemplo Completo:**

```c
// Código fuente
template<typename T>
struct Par {
    T first;
    T second;
};

Par<int> p;
Par<float> q;
```

```cpp
// Paso 1: TemplateTypeNode detectado
TemplateTypeNode {
    name: "Par",
    type_args: [PrimitiveTypeNode(INT)]
}

// Paso 2: Nombre mangleado
concrete_name = "Par<int>"

// Paso 3: Caché miss (primera instanciación)

// Paso 4: Mapa de sustitución
subs = {"T": intType}

// Paso 5: Sustitución en miembros
member "first": NamedTypeNode("T") → intType
member "second": NamedTypeNode("T") → intType

// Paso 6: StructType creado
struct_types["Par<int>"] = StructType{
    name: "Par<int>",
    members: {"first": intType, "second": intType}
}

// Resultado
p->resolvedType = struct_types["Par<int>"]

// Segunda instanciación (Par<float>)
concrete_name = "Par<float>"
subs = {"T": floatType}
struct_types["Par<float>"] = StructType{
    name: "Par<float>",
    members: {"first": floatType, "second": floatType}
}
```

### 2.3 Instanciación de Function Templates

**Flujo Completo:**

**Paso 1: Detección de Llamada con Argumentos Template**

```cpp
// En visit(FcallNode)
if (!fcall->template_args.empty()) {
    // Buscar template de función
    auto it = template_decls.find(callee_name);
    if (it != template_decls.end() && it->second->is_function) {
        fd = instantiate_function(it->second, fcall->template_args);
    }
}
```

**Paso 2: Generación de Key y Verificación de Caché**

```cpp
FunDecl* TypeChecker::instantiate_function(TemplateDecl* tdecl, const vector<TypeNode*>& args) {
    FunDecl* orig = tdecl->func;
    string key = mangleTemplateName(orig->name, args);
    
    auto cached = instantiated_function_cache.find(key);
    if (cached != instantiated_function_cache.end())
        return cached->first;  // reusar instancia existente
    
    // ... continuar
}
```

**Paso 3: Construcción de Mapa de Sustitución**

```cpp
unordered_map<string, Type*> subs;
for (size_t i = 0; i < tdecl->params.size() && i < args.size(); i++)
    subs[tdecl->params[i]] = type_from_ast(args[i]);
```

**Paso 4: Sustitución en AST**

```cpp
function<Exp*(Exp*)> subst_node = [&](Exp* node) -> Exp* {
    if (auto* nt = dynamic_cast<NamedTypeNode*>(node)) {
        auto it = subs.find(nt->name);
        if (it != subs.end()) return semantic_to_type_node(it->second);
    }
    if (auto* pt = dynamic_cast<PointerTypeNode*>(node)) {
        return new PointerTypeNode(dynamic_cast<TypeNode*>(subst_node(pt->base)));
    }
    return node;
};
```

**Paso 5: Creación de FunDecl Concreto**

```cpp
FunDecl* fd = new FunDecl(subst_node(orig->return_type), orig->name, orig->body);
for (auto p : orig->params)
    fd->params.push_back(new VarDecl(subst_node(p->type), p->name));

instantiated_function_cache[key] = fd;
if (program) program->instantiated_functions.push_back(fd);
return fd;
```

**Ejemplo Completo:**

```c
// Código fuente
template<typename T>
T identidad(T x) {
    return x;
}

int a = identidad<int>(42);
float b = identidad<float>(3.14);
```

```cpp
// Primera llamada: identidad<int>(42)
// Paso 1: FcallNode con template_args
FcallNode {
    callee: IdentifierNode("identidad"),
    args: [IntegerLiteralNode(42)],
    template_args: [PrimitiveTypeNode(INT)]
}

// Paso 2: Key = "identidad<int>", caché miss

// Paso 3: Mapa de sustitución
subs = {"T": intType}

// Paso 4: Sustitución en firma
return_type: NamedTypeNode("T") → PrimitiveTypeNode(INT)
param type: NamedTypeNode("T") → PrimitiveTypeNode(INT)

// Paso 5: FunDecl creado
FunDecl {
    return_type: PrimitiveTypeNode(INT),
    name: "identidad",
    params: [VarDecl{type: PrimitiveTypeNode(INT), name: "x"}],
    body: (original, sin cambios en body)
}

instantiated_function_cache["identidad<int>"] = fd

// Segunda llamada: identidad<float>(3.14)
// Key = "identidad<float>", caché miss
// Nueva instancia creada con floatType
```

### 2.4 Ejemplo con Múltiples Parámetros de Template

```c
// Código fuente
template<typename T, typename U>
struct Par {
    T first;
    U second;
};

Par<int, float> p;
```

```cpp
// TemplateTypeNode
TemplateTypeNode {
    name: "Par",
    type_args: [PrimitiveTypeNode(INT), PrimitiveTypeNode(FLOAT)]
}

// Nombre mangleado
concrete_name = "Par<int,float>"

// Mapa de sustitución
subs = {"T": intType, "U": floatType}

// Sustitución en miembros
member "first": NamedTypeNode("T") → intType
member "second": NamedTypeNode("U") → floatType

// StructType resultante
struct_types["Par<int,float>"] = StructType{
    name: "Par<int,float>",
    members: {"first": intType, "second": floatType}
}
```

### 2.5 Ejemplo con Punteros en Templates

```c
// Código fuente
template<typename T>
struct Box {
    T* value;
};

Box<int> b;
```

```cpp
// TemplateDecl
TemplateDecl {
    params: ["T"],
    struct_decl: StructDecl {
        name: "Box",
        members: [
            VarDecl{
                type: PointerTypeNode(NamedTypeNode("T")),
                name: "value"
            }
        ]
    }
}

// Instanciación con int
// Mapa de sustitución
subs = {"T": intType}

// Sustitución recursiva
PointerTypeNode(NamedTypeNode("T"))
  → substitute(NamedTypeNode("T")) = intType
  → PointerTypeNode(PrimitiveTypeNode(INT))
  → type_from_ast() = PointerType{base: intType}

// StructType resultante
struct_types["Box<int>"] = StructType{
    name: "Box<int>",
    members: {"value": PointerType{base: intType}}
}
```

---

## 3. Gestión de Variables y Manejo de Scope

### 3.1 Sistema de Environments

El compilador utiliza dos tablas de símbolos implementadas con la clase `Environment<T>`:

- **env**: `Environment<Type*>` — nombre de variable → tipo semántico
- **varEnv**: `Environment<VarDecl*>` — nombre → nodo VarDecl del AST (binding)

Ambas implementan el patrón de "ribs" (costillas): cada nivel de scope es un `unordered_map` independiente. La búsqueda va del scope más interno al más externo (shadowing permitido).

```cpp
template <typename T>
class Environment {
private:
    vector<unordered_map<string, T>> ribs;  // ribs[0] = más externo, ribs.back() = actual
    
public:
    void add_level();              // abre scope (push)
    void add_var(const string& var, const T& value);  // define en scope actual
    bool remove_level();           // cierra scope (pop)
    bool check_current(const string& x);  // ¿existe en scope actual?
    bool check(const string& x);   // ¿existe en algún scope visible?
    bool lookup(const string& x, T& v);  // busca y asigna valor
};
```

### 3.2 Flujo de Gestión de Scope Global

**Paso 1: Apertura de Scope Global**

```cpp
void TypeChecker::visit(Program* p) {
    program = p;
    for (auto f : p->functions) add_function(f);
    
    env.add_level();      // ribs = [{}]
    varEnv.add_level();   // ribs = [{}]
    
    // ... procesar globals, structs, templates, funciones ...
    
    varEnv.remove_level();
    env.remove_level();
}
```

**Paso 2: Registro de Variables Globales**

```cpp
for (auto g : p->globals) g->accept(this);
```

Cada variable global llama a `visit(VarDecl)`, que registra en ambos environments.

**Ejemplo:**

```c
// Código fuente
int global_x = 10;
float global_y = 3.14;

int main() {
    return 0;
}
```

```cpp
// Después de visit(Program)
env (Type*):
  ribs[0]: {"global_x": intType, "global_y": floatType}

varEnv (VarDecl*):
  ribs[0]: {"global_x": VarDecl*{name:"global_x", ...}, 
            "global_y": VarDecl*{name:"global_y", ...}}
```

### 3.3 Flujo de Gestión de Scope de Función

**Paso 1: Apertura de Scope de Función**

```cpp
void TypeChecker::visit(FunDecl* f) {
    env.add_level();      // ribs = [{global}, {}]
    varEnv.add_level();   // ribs = [{global}, {}]
    functionDepth++;
    
    retornodefuncion = type_from_ast(f->return_type);
    
    // ... procesar parámetros y locales ...
    
    varEnv.remove_level();
    env.remove_level();
    functionDepth--;
}
```

**Paso 2: Registro de Parámetros**

```cpp
for (auto p : f->params) {
    Type* t = type_from_ast(p->type);
    p->memSize = t->size();
    p->resolvedType = t;
    params.push_back(p);
    initialized_vars.insert(p);
}

// Asignar offsets
int paramSize = assignOffsets(params, 0);

// Agregar al environment
for (auto p : params) {
    bind_var_decl(p);
}
```

**Paso 3: bind_var_decl — Registro en Ambos Environments**

```cpp
void TypeChecker::bind_var_decl(VarDecl* v) {
    env.add_var(v->name, v->resolvedType);      // env: nombre → Type*
    varEnv.add_var(v->name, v);                 // varEnv: nombre → VarDecl*
}
```

**Ejemplo:**

```c
// Código fuente
int suma(int a, int b) {
    return a + b;
}
```

```cpp
// Después de visit(FunDecl)
env (Type*):
  ribs[0]: {"global_x": intType, "global_y": floatType}
  ribs[1]: {"a": intType, "b": intType}

varEnv (VarDecl*):
  ribs[0]: {"global_x": VarDecl*, "global_y": VarDecl*}
  ribs[1]: {"a": VarDecl*{offset:-8, ...}, 
            "b": VarDecl*{offset:-16, ...}}
```

### 3.4 Flujo de Gestión de Scope de Bloque

**Paso 1: Apertura de Scope en Body**

```cpp
void TypeChecker::visit(Body* b) {
    env.add_level();      // nuevo scope para bloque
    varEnv.add_level();
    
    for (auto s : b->stmts) {
        s->accept(this);
    }
    
    varEnv.remove_level();
    env.remove_level();
}
```

**Paso 2: Registro de Variables Locales**

```cpp
void TypeChecker::visit(VarDecl* v) {
    // ... resolver tipo ...
    
    if (env.check_current(v->name)) {
        error("variable '" + v->name + "' ya declarada en este ámbito.");
        return;
    }
    
    v->resolvedType = t;
    v->memSize = t->size();
    
    bind_var_decl(v);  // registrar en env y varEnv
}
```

**Ejemplo con Bloques Anidados:**

```c
// Código fuente
int main() {
    int x = 10;
    {
        int y = 20;
        x = x + y;
    }
    return x;
}
```

```cpp
// Estado de environments durante typechecking

// Al entrar a main()
env:
  ribs[0]: {globals...}
  ribs[1]: {"x": intType}

varEnv:
  ribs[0]: {globals...}
  ribs[1]: {"x": VarDecl*{offset:-8, ...}}

// Al entrar al bloque interno {}
env:
  ribs[0]: {globals...}
  ribs[1]: {"x": intType}
  ribs[2]: {"y": intType}

varEnv:
  ribs[0]: {globals...}
  ribs[1]: {"x": VarDecl*}
  ribs[2]: {"y": VarDecl*{offset:-16, ...}}

// Al salir del bloque interno (remove_level)
env:
  ribs[0]: {globals...}
  ribs[1]: {"x": intType}

varEnv:
  ribs[0]: {globals...}
  ribs[1]: {"x": VarDecl*}
```

### 3.5 Resolución de Identificadores

**Paso 1: Búsqueda en varEnv**

```cpp
void TypeChecker::visit(IdentifierNode* e) {
    VarDecl* binding = nullptr;
    if (varEnv.lookup(e->name, binding)) {
        e->binding = binding;           // enlazar con VarDecl
        e->resolvedType = binding->resolvedType;
    } else {
        error("variable '" + e->name + "' no declarada.");
        e->resolvedType = intType;
    }
}
```

**Paso 2: Búsqueda en Environment (search_rib)**

```cpp
int search_rib(const string& var) const {
    // Busca desde el rib más interno hacia afuera
    for (int idx = static_cast<int>(ribs.size()) - 1; idx >= 0; --idx) {
        auto it = ribs[idx].find(var);
        if (it != ribs[idx].end())
            return idx;  // encontrado en este nivel
    }
    return -1;  // no encontrado
}
```

**Ejemplo de Resolución:**

```c
// Código fuente
int x = 10;
void test() {
    int x = 20;  // shadowing
    {
        int y = 30;
        printf("%d", x);  // ¿cuál x?
    }
}
```

```cpp
// Al resolver "x" dentro del bloque interno
search_rib("x") en varEnv:
  idx=2 (bloque interno): "x" no encontrado
  idx=1 (función): "x" encontrado → VarDecl*{offset:-8, value:20}
  
Resultado: e->binding apunta al x local (20), no al global (10)
```

### 3.6 Shadowing (Ocultamiento de Variables)

C-- permite shadowing: una variable en un scope interno puede tener el mismo nombre que una variable en un scope externo.

**Ejemplo:**

```c
// Código fuente
int x = 100;  // global

void ejemplo() {
    int x = 10;  // shadowing del global
    {
        int x = 5;  // shadowing del local
        printf("%d", x);  // imprime 5
    }
    printf("%d", x);  // imprime 10
}
```

```cpp
// Estado de environments
// En el bloque más interno
env:
  ribs[0]: {"x": intType}  // global
  ribs[1]: {"x": intType}  // función
  ribs[2]: {"x": intType}  // bloque interno

// Búsqueda de "x" desde ribs[2]:
search_rib("x") → idx=2 (encontrado en scope actual)
// Retorna el x del bloque interno (valor 5)
```

**Detección de Redeclaración:**

```cpp
void TypeChecker::visit(VarDecl* v) {
    if (env.check_current(v->name)) {
        error("variable '" + v->name + "' ya declarada en este ámbito.");
        return;
    }
    // ... continuar con registro ...
}
```

`check_current` solo verifica el scope actual (ribs.back()), permitiendo shadowing pero prohibiendo redeclaración en el mismo bloque.

### 3.7 Ejemplo Completo con Múltiples Scopes

```c
// Código fuente
int global = 1;

int main() {
    int local1 = 2;
    
    if (true) {
        int local2 = 3;
        int local3 = local1 + local2;
    }
    
    while (true) {
        int local4 = 4;
        break;
    }
    
    return local1;
}
```

```cpp
// Flujo de scopes

// 1. visit(Program)
env.add_level();  // ribs[0] = {globals}
varEnv.add_level();  // ribs[0] = {globals}
env.add_var("global", intType);
varEnv.add_var("global", VarDecl*{global});

// 2. visit(main)
env.add_level();  // ribs[1] = {}
varEnv.add_level();  // ribs[1] = {}
// registrar parámetros (si tuviera)

// 3. visit(VarDecl local1)
env.add_var("local1", intType);  // ribs[1] = {"local1": intType}
varEnv.add_var("local1", VarDecl*{local1});

// 4. visit(IfStmt)
Body* then_branch → visit(Body)
  env.add_level();  // ribs[2] = {}
  varEnv.add_level();  // ribs[2] = {}
  
  // visit(VarDecl local2)
  env.add_var("local2", intType);  // ribs[2] = {"local2": intType}
  varEnv.add_var("local2", VarDecl*{local2});
  
  // visit(VarDecl local3)
  env.add_var("local3", intType);  // ribs[2] = {"local2", "local3"}
  varEnv.add_var("local3", VarDecl*{local3});
  
  // Resolver local1 en expresión
  search_rib("local1") → idx=1 (encontrado en función)
  
  varEnv.remove_level();  // ribs[2] eliminado
  env.remove_level();

// 5. visit(WhileStmt)
Body* body → visit(Body)
  env.add_level();  // ribs[2] = {}
  varEnv.add_level();  // ribs[2] = {}
  
  // visit(VarDecl local4)
  env.add_var("local4", intType);
  varEnv.add_var("local4", VarDecl*{local4});
  
  varEnv.remove_level();
  env.remove_level();

// 6. Salir de main
varEnv.remove_level();
env.remove_level();

// 7. Salir de program
varEnv.remove_level();
env.remove_level();
```

### 3.8 Recolección de Variables Locales (collectVars)

Antes de procesar el body de una función, el TypeChecker recolecta todas las variables locales (incluso las anidadas en if/while/for) para calcular offsets de stack frame.

```cpp
void TypeChecker::collectVars(Stm* stmt, vector<VarDecl*>& vars) {
    if (auto* v = dynamic_cast<VarDecl*>(stmt)) {
        vars.push_back(v);  // agregar VarDecl a la lista
    }
    
    if (auto* body = dynamic_cast<Body*>(stmt)) {
        for (auto s : body->stmts) {
            collectVars(s, vars);  // recursivo
        }
    }
    
    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        collectVars(ifStmt->then_branch, vars);
        if (ifStmt->else_branch) collectVars(ifStmt->else_branch, vars);
    }
    
    // ... similar para while, for, do-while, switch ...
}
```

**Ejemplo:**

```c
// Código fuente
int ejemplo() {
    int a = 1;
    if (true) {
        int b = 2;
        int c = 3;
    }
    return a;
}
```

```cpp
// collectVars(f->body, localVars)
// Recorre el body y encuentra:
localVars = [VarDecl{a}, VarDecl{b}, VarDecl{c}]

// Luego assignOffsets(localVars, 0)
a->offset = 0, a->memSize = 4
b->offset = 8, b->memSize = 4  // slot de 8 bytes
c->offset = 16, c->memSize = 4

// Frame size total = 24 (alineado a 16 → 32)
```

**Nota:** Aunque `b` y `c` están en un bloque interno, comparten el mismo frame que `a` (como en C tradicional con gcc -O0). Los scopes solo afectan la visibilidad, no el layout de memoria.

---

## 4. Ciclo de Vida de los Tipos

### 4.1 Tipos Primitivos (Singletons)

- **Creación**: En constructor de TypeChecker
- **Uso**: Reutilizados en todas las llamadas a type_from_ast()
- **Destrucción**: En destructor de TypeChecker

```cpp
TypeChecker::~TypeChecker() {
    delete intType;
    delete charType;
    delete floatType;
    delete doubleType;
    delete boolType;
    delete voidType;
}
```

### 4.2 Tipos Compuestos (Dinámicos)

- **Creación**: Con `new` en type_from_ast() o instantiate_template()
- **Registro**: Agregados a typeCache
- **Destrucción**: En destructor de TypeChecker

```cpp
TypeChecker::~TypeChecker() {
    for (auto t : typeCache) delete t;
    for (auto& [k, v] : struct_types) delete v;
}
```

### 4.3 StructTypes

- **Creación**: En visit(StructDecl) o instantiate_template()
- **Registro**: En unordered_map struct_types
- **Destrucción**: En destructor de TypeChecker

---

## 5. Resumen de Flujos

### 5.1 Tipo Básico (int)

```
Código: int x = 5;
  ↓
AST: VarDecl{type: PrimitiveTypeNode(INT), ...}
  ↓
type_from_ast(PrimitiveTypeNode(INT))
  ↓
return intType (singleton)
  ↓
x->resolvedType = intType
```

### 5.2 Puntero (int*)

```
Código: int* p;
  ↓
AST: VarDecl{type: PointerTypeNode(PrimitiveTypeNode(INT)), ...}
  ↓
type_from_ast(PointerTypeNode(...))
  ↓
  type_from_ast(PrimitiveTypeNode(INT)) → intType
  ↓
  new PointerType(intType)
  ↓
  typeCache.push_back(ptr)
  ↓
return PointerType{base: intType}
  ↓
p->resolvedType = PointerType{base: intType}
```

### 5.3 Struct Template (Par<int>)

```
Código: Par<int> p;
  ↓
AST: VarDecl{type: TemplateTypeNode("Par", [PrimitiveTypeNode(INT)]), ...}
  ↓
type_from_ast(TemplateTypeNode(...))
  ↓
instantiate_template("Par", [PrimitiveTypeNode(INT)])
  ↓
  mangleTemplateName("Par", ...) → "Par<int>"
  ↓
  struct_types.find("Par<int>") → miss
  ↓
  template_decls.find("Par") → TemplateDecl
  ↓
  subs = {"T": intType}
  ↓
  substitute(NamedTypeNode("T")) → intType
  ↓
  new StructType("Par<int>")
  ↓
  st->members["first"] = intType
  ↓
  struct_types["Par<int>"] = st
  ↓
return StructType{name: "Par<int>", members: {...}}
  ↓
p->resolvedType = struct_types["Par<int>"]
```

### 5.4 Function Template (identidad<int>(42))

```
Código: identidad<int>(42)
  ↓
AST: FcallNode{callee: "identidad", template_args: [PrimitiveTypeNode(INT)], ...}
  ↓
template_decls.find("identidad") → TemplateDecl
  ↓
instantiate_function(TemplateDecl, [PrimitiveTypeNode(INT)])
  ↓
  mangleTemplateName("identidad", ...) → "identidad<int>"
  ↓
  instantiated_function_cache.find("identidad<int>") → miss
  ↓
  subs = {"T": intType}
  ↓
  subst_node(NamedTypeNode("T")) → PrimitiveTypeNode(INT)
  ↓
  new FunDecl(PrimitiveTypeNode(INT), ...)
  ↓
  instantiated_function_cache["identidad<int>"] = fd
  ↓
return FunDecl{return_type: intType, params: [int x]}
  ↓
Verificar llamada con argumentos
```

---

## 6. Manejo de Funciones y Structs

### 6.1 Registro de Funciones (add_function)

**Paso 1: Registro de Firmas en visit(Program)**

Antes de typechecar las funciones, el compilador registra todas las firmas en el mapa `functions` para poder verificar llamadas posteriormente.

```cpp
void TypeChecker::visit(Program* p) {
    program = p;
    for (auto f : p->functions) add_function(f);  // registrar firmas primero
    // ... procesar globals, structs, templates ...
    for (auto f : p->functions) f->accept(this);  // typecheck después
}
```

**Paso 2: add_function — Almacenar Firma**

```cpp
void TypeChecker::add_function(FunDecl* fd) {
    // Detectar redeclaración
    if (functions.find(fd->name) != functions.end()) {
        error("función '" + fd->name + "' ya declarada.");
        return;
    }
    
    // Construir FuncInfo con tipo de retorno y parámetros
    Type* returnType = type_from_ast(fd->return_type);
    FuncInfo info;
    info.returnType = returnType;
    
    for (auto p : fd->params) {
        info.paramTypes.push_back(type_from_ast(p->type));
    }
    
    functions[fd->name] = info;
}
```

**Estructura FuncInfo:**

```cpp
struct FuncInfo {
    ::Type* returnType;           // tipo de retorno
    vector<::Type*> paramTypes;   // tipos de parámetros
};
```

**Ejemplo:**

```c
// Código fuente
int suma(int a, int b) {
    return a + b;
}

float producto(float x, float y) {
    return x * y;
}
```

```cpp
// Después de add_function para cada función
functions["suma"] = FuncInfo{
    returnType: intType,
    paramTypes: [intType, intType]
}

functions["producto"] = FuncInfo{
    returnType: floatType,
    paramTypes: [floatType, floatType]
}
```

### 6.2 Typecheck de Funciones (visit(FunDecl))

**Flujo Completo:**

**Paso 1: Apertura de Scope**

```cpp
void TypeChecker::visit(FunDecl* f) {
    env.add_level();
    varEnv.add_level();
    functionDepth++;
    
    retornodefuncion = type_from_ast(f->return_type);
```

**Paso 2: Procesamiento de Parámetros**

```cpp
vector<VarDecl*> params;
for (auto p : f->params) {
    Type* t = type_from_ast(p->type);
    if (t->match(voidType))
        error("parámetro no puede ser de tipo void.");
    p->memSize = t->size();
    p->resolvedType = t;
    params.push_back(p);
    initialized_vars.insert(p);
}

int paramSize = assignOffsets(params, 0);
```

**Paso 3: Recolección de Variables Locales**

```cpp
vector<VarDecl*> localVars;
collectVars(f->body, localVars);

for (auto v : localVars) {
    if (v->resolvedType == nullptr) {
        Type* t = type_from_ast(v->type);
        // ... manejo de arreglos y auto ...
        v->resolvedType = t;
        v->memSize = t->size();
    }
}

int totalSize = assignOffsets(localVars, paramSize);
```

**Paso 4: Cálculo de Frame Size**

```cpp
f->frameSize = (totalSize + 15) & ~15;  // alinear a 16 bytes

// Convertir offsets a negativos (relativos a %rbp)
for (auto p : params) {
    p->offset = p->offset - f->frameSize;
}
for (auto v : localVars) {
    v->offset = v->offset - f->frameSize;
}
```

**Paso 5: Binding de Parámetros y Typecheck del Body**

```cpp
for (auto p : params) {
    bind_var_decl(p);
}

f->body->accept(this);
```

**Paso 6: Verificación de Return**

```cpp
if (!retornodefuncion->match(voidType)) {
    bool endsWithReturn = false;
    if (!f->body->stmts.empty()) {
        Stm* last = f->body->stmts.back();
        endsWithReturn = (dynamic_cast<ReturnStmt*>(last) != nullptr);
    }
    if (!endsWithReturn)
        error("función no-void no garantiza retorno en todos los caminos.", f->loc);
}

varEnv.remove_level();
env.remove_level();
functionDepth--;
```

**Ejemplo Completo:**

```c
// Código fuente
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    int result = n * factorial(n - 1);
    return result;
}
```

```cpp
// Flujo de visit(FunDecl)

// 1. Abrir scope
env.add_level();  // ribs = [{global}, {}]
varEnv.add_level();

// 2. Procesar parámetro "n"
params = [VarDecl{n, resolvedType: intType, memSize: 4}]
assignOffsets(params, 0) → n->offset = 0, paramSize = 8

// 3. Recolectar variables locales
collectVars(body, localVars) → [VarDecl{result}]
localVars[0]->resolvedType = intType, memSize = 4

assignOffsets(localVars, 8) → result->offset = 8, totalSize = 16

// 4. Frame size
f->frameSize = (16 + 15) & ~15 = 16

// 5. Convertir offsets
n->offset = 0 - 16 = -16
result->offset = 8 - 16 = -8

// 6. Binding
env.add_var("n", intType)
varEnv.add_var("n", VarDecl*{n})

// 7. Typecheck body
// - Verificar condición n <= 1 (int <= int → bool)
// - Verificar return 1 (int → int, ok)
// - Verificar declaración result (int)
// - Verificar expresión n * factorial(n-1)
// - Verificar return result (int → int, ok)

// 8. Verificar return
// Último statement es ReturnStmt → endsWithReturn = true
// No hay error
```

### 6.3 Verificación de Llamadas a Funciones (checkFuncCall)

**Función checkFuncCall:**

```cpp
bool TypeChecker::checkFuncCall(const string& fname, FuncInfo& info, FcallNode* e) {
    // Verificar cantidad de argumentos
    if (e->args.size() != info.paramTypes.size()) {
        error("número de argumentos incorrecto en llamada a '" + fname +
              "' (esperaba " + to_string(info.paramTypes.size()) +
              ", recibió " + to_string(e->args.size()) + ").");
        return false;
    }
    
    // Verificar tipo de cada argumento
    for (size_t i = 0; i < e->args.size(); i++) {
        e->args[i]->accept(this);
        Type* argType = e->args[i]->resolvedType;
        if (!check_assign(info.paramTypes[i], argType)) {
            error("tipo de argumento " + to_string(i+1) +
                  " incorrecto en llamada a '" + fname + "'.");
        }
    }
    return true;
}
```

**Ejemplo:**

```c
// Código fuente
int suma(int a, int b) {
    return a + b;
}

void test() {
    int x = suma(1, 2);        // ok
    int y = suma(1);           // error: cantidad
    int z = suma(1.5, 2.0);    // error: tipos (float → int no permitido)
}
```

```cpp
// checkFuncCall("suma", info, fcall)

// Llamada suma(1, 2)
e->args.size() = 2
info.paramTypes.size() = 2 → cantidad ok
arg[0]: IntegerLiteralNode(1) → intType, check_assign(intType, intType) → true
arg[1]: IntegerLiteralNode(2) → intType, check_assign(intType, intType) → true
return true

// Llamada suma(1)
e->args.size() = 1
info.paramTypes.size() = 2 → cantidad incorrecta
error: "número de argumentos incorrecto en llamada a 'suma' (esperaba 2, recibió 1)"
return false

// Llamada suma(1.5, 2.0)
e->args.size() = 2 → cantidad ok
arg[0]: FloatLiteralNode(1.5) → floatType, check_assign(intType, floatType) → false
error: "tipo de argumento 1 incorrecto en llamada a 'suma'"
return false
```

### 6.4 Llamadas a Funciones (visit(FcallNode))

**Tres Casos Manejados:**

**Caso 1: Llamada a Función Template**

```cpp
if (!e->template_args.empty()) {
    auto tit = template_decls.find(fname);
    if (tit == template_decls.end() || !tit->second->is_function) {
        error("template de función '" + fname + "' no declarado.");
        e->resolvedType = intType; return;
    }
    
    TemplateDecl* tdecl = tit->second;
    instantiate_function(tdecl, e->template_args);
    
    // Construir firma concreta y verificar
    FuncInfo info;
    info.returnType = concrete_ret;
    info.paramTypes = concrete_params;
    functions[fname] = info;
    
    checkFuncCall(fname, info, e);
    e->resolvedType = info.returnType;
    return;
}
```

**Caso 2: Llamada a Variable (Lambda/Puntero)**

```cpp
Type* localType = env.lookup(fname);
if (localType) {
    // Acepta argumentos sin verificar firma (limitación)
    for (auto* arg : e->args) arg->accept(this);
    e->resolvedType = intType;
    return;
}
```

**Caso 3: Llamada a Función Normal**

```cpp
auto it = functions.find(fname);
if (it != functions.end()) {
    FuncInfo& info = it->second;
    checkFuncCall(fname, info, e);
    e->resolvedType = info.returnType;
    return;
}

error("llamada a función no reconocida.");
```

**Ejemplo:**

```c
// Código fuente
template<typename T>
T identidad(T x) {
    return x;
}

int main() {
    int a = identidad<int>(42);      // caso 1: template
    int b = identidad<float>(3.14);  // caso 1: template
    return 0;
}
```

```cpp
// visit(FcallNode) para identidad<int>(42)

// Caso 1: template_args no vacío
template_args = [PrimitiveTypeNode(INT)]
template_decls.find("identidad") → TemplateDecl{is_function: true}
instantiate_function(...) → FunDecl{return_type: intType, params: [int x]}

// Firma concreta
info.returnType = intType
info.paramTypes = [intType]

checkFuncCall("identidad", info, e)
  args.size() = 1, paramTypes.size() = 1 → ok
  arg[0]: IntegerLiteralNode(42) → intType, check_assign(intType, intType) → true

e->resolvedType = intType
```

### 6.5 Declaración de Structs (visit(StructDecl))

**Flujo:**

```cpp
void TypeChecker::visit(StructDecl* s) {
    StructType* st = new StructType(s->name);
    int offset = 0;
    
    for (auto m : s->members) {
        Type* mt = type_from_ast(m->type);
        m->resolvedType = mt;
        st->members[m->name] = mt;
        s->memberOffsets[m->name] = offset;
        s->memberSizes[m->name] = mt->size();
        offset += mt->size();
    }
    
    s->totalSize = offset;
    struct_types[s->name] = st;
}
```

**Ejemplo:**

```c
// Código fuente
struct Point {
    int x;
    float y;
    char z;
};
```

```cpp
// AST
StructDecl {
    name: "Point",
    members: [
        VarDecl{type: PrimitiveTypeNode(INT), name: "x"},
        VarDecl{type: PrimitiveTypeNode(FLOAT), name: "y"},
        VarDecl{type: PrimitiveTypeNode(CHAR), name: "z"}
    ]
}

// Después de visit(StructDecl)
StructType* st = new StructType("Point");

// Procesar miembro "x"
mt = type_from_ast(PrimitiveTypeNode(INT)) → intType
st->members["x"] = intType
s->memberOffsets["x"] = 0
s->memberSizes["x"] = 4
offset = 4

// Procesar miembro "y"
mt = type_from_ast(PrimitiveTypeNode(FLOAT)) → floatType
st->members["y"] = floatType
s->memberOffsets["y"] = 4
s->memberSizes["y"] = 4
offset = 8

// Procesar miembro "z"
mt = type_from_ast(PrimitiveTypeNode(CHAR)) → charType
st->members["z"] = charType
s->memberOffsets["z"] = 8
s->memberSizes["z"] = 1
offset = 9

s->totalSize = 9
struct_types["Point"] = StructType{
    name: "Point",
    members: {"x": intType, "y": floatType, "z": charType}
}
```

### 6.6 Uso de Structs (type_from_ast para StructTypeNode)

**Flujo:**

```cpp
if (auto* st = dynamic_cast<StructTypeNode*>(t)) {
    auto it = struct_types.find(st->name);
    if (it != struct_types.end()) return it->second;
    error("struct '" + st->name + "' no declarado.");
    return intType;
}
```

**Ejemplo:**

```c
// Código fuente
struct Point {
    int x;
    float y;
};

Point p;
Point* ptr;
```

```cpp
// AST para "Point p"
VarDecl {
    type: StructTypeNode("Point"),
    name: "p"
}

// type_from_ast(StructTypeNode("Point"))
struct_types.find("Point") → StructType{name: "Point", ...}
return struct_types["Point"]

p->resolvedType = StructType{name: "Point", members: {...}}

// AST para "Point* ptr"
VarDecl {
    type: PointerTypeNode(StructTypeNode("Point")),
    name: "ptr"
}

// type_from_ast(PointerTypeNode(StructTypeNode("Point")))
base = type_from_ast(StructTypeNode("Point")) → StructType{"Point"}
new PointerType(StructType{"Point"})
return PointerType{base: StructType{"Point"}}

ptr->resolvedType = PointerType{base: StructType{"Point"}}
```

### 6.7 Acceso a Miembros de Struct

**Acceso por Punto (obj.member):**

```c
// Código fuente
Point p;
int x_coord = p.x;
```

```cpp
// AST
MemberAccessNode {
    object: IdentifierNode("p"),
    member: "x"
}

// visit(MemberAccessNode)
p->resolvedType = StructType{"Point"}
object->accept(this) → p->resolvedType = StructType{"Point"}

// Buscar miembro en StructType
StructType* st = (StructType*)object->resolvedType;
auto it = st->members.find("x");
if (it != st->members.end()) {
    e->resolvedType = it->second;  // intType
}
```

**Acceso por Flecha (ptr->member):**

```c
// Código fuente
Point* ptr;
int x_coord = ptr->x;
```

```cpp
// AST
ArrowAccessNode {
    pointer: IdentifierNode("ptr"),
    member: "x"
}

// visit(ArrowAccessNode)
ptr->resolvedType = PointerType{base: StructType{"Point"}}
pointer->accept(this) → ptr->resolvedType

// Desreferenciar y acceder
PointerType* pt = (PointerType*)pointer->resolvedType;
StructType* st = (StructType*)pt->base;
auto it = st->members.find("x");
if (it != st->members.end()) {
    e->resolvedType = it->second;  // intType
}
```

### 6.8 Ejemplo Completo con Funciones y Structs

```c
// Código fuente
struct Point {
    int x;
    int y;
};

Point create_point(int x, int y) {
    Point p;
    p.x = x;
    p.y = y;
    return p;
}

int distance(Point a, Point b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return dx * dx + dy * dy;
}

int main() {
    Point p1 = create_point(0, 0);
    Point p2 = create_point(3, 4);
    int d = distance(p1, p2);
    return d;
}
```

```cpp
// Flujo de typechecking

// 1. visit(StructDecl Point)
struct_types["Point"] = StructType{
    name: "Point",
    members: {"x": intType, "y": intType}
}
memberOffsets: {"x": 0, "y": 4}
totalSize: 8

// 2. add_function(create_point)
functions["create_point"] = FuncInfo{
    returnType: StructType{"Point"},
    paramTypes: [intType, intType]
}

// 3. add_function(distance)
functions["distance"] = FuncInfo{
    returnType: intType,
    paramTypes: [StructType{"Point"}, StructType{"Point"}]
}

// 4. add_function(main)
functions["main"] = FuncInfo{
    returnType: intType,
    paramTypes: []
}

// 5. visit(FunDecl create_point)
// - Parámetros: x (int, offset -16), y (int, offset -24)
// - Local: p (Point, offset -32)
// - Body: verificar asignaciones p.x = x, p.y = y
// - Return: verificar p es Point, retorno es Point

// 6. visit(FunDecl distance)
// - Parámetros: a (Point, offset -16), b (Point, offset -32)
// - Locales: dx (int, offset -40), dy (int, offset -48)
// - Body: verificar a.x - b.x (int - int → int)
// - Return: verificar dx*dx + dy*dy (int + int → int)

// 7. visit(FunDecl main)
// - Locales: p1 (Point, offset -16), p2 (Point, offset -32), d (int, offset -40)
// - Llamada create_point(0, 0):
//   checkFuncCall("create_point", info, fcall)
//   args: [0, 0] → intType, intType
//   params: [intType, intType] → ok
//   resolvedType: StructType{"Point"}
// - Llamada distance(p1, p2):
//   checkFuncCall("distance", info, fcall)
//   args: [p1, p2] → Point, Point
//   params: [Point, Point] → ok
//   resolvedType: intType
// - Return: verificar d es int
```
