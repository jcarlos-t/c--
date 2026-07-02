#include "visitor.h"
#include "ast.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

using namespace std;

// ============================================================
// GenCodeVisitor — Generación de código x86-64 (sintaxis AT&T)
// ============================================================
// Implementa el backend del compilador C--: recorre el AST y emite
// ensamblador x86-64 listo para ensamblar con GCC (Linux, ABI System V).
//
// Convenciones de este generador:
//   - ABI System V AMD64 (Linux)
//   - Args enteros/punteros: %rdi, %rsi, %rdx, %rcx, %r8, %r9
//   - Args float/double:      %xmm0–%xmm7
//   - Retorno entero/puntero: %rax
//   - Retorno float/double:   %xmm0 (a veces reempaquetado en %rax)
//   - Stack frame:            %rbp = base, offsets negativos hacia abajo
//   - Toda expresión deja su resultado en %rax (convención interna)
//   - L-values (asignaciones, ++, &): struct LVal + lvalTarget
//
// Dependencias del TypeChecker (anotadas en el AST antes de codegen):
//   - VarDecl: offset, memSize, resolvedType
//   - IdentifierNode: binding → VarDecl
//   - Expresiones: resolvedType, isConstant, constantValue
//   - FunDecl: frameSize (stack con bin packing)
//   - StructDecl: memberOffsets, memberSizes
//
// Índice del archivo:
//   1. baseStructName, computeAddress stubs
//   2. generate() — punto de entrada, .data/.text, potencia
//   3. Helpers de tamaño (instrSuffix, loadInstr, storeInstr)
//   4. bindingMem, bind_var_decl, array_elem_size, arrayDimsFor
//   5. collectIndices, load/store/lea Binding, emitIndexed*
//   6. captureLVal, storeTarget — protocolo de asignación
//   7. visit(expresiones): literales, binarias, unarias, calls, etc.
//   8. visit(statements): if/while/for/switch/break/return
//   9. visit(declaraciones): VarDecl, FunDecl, StructDecl, lambda
//  10. computeAddress — direcciones de l-values para & y asignaciones
//
// Flujo típico:
//   main.cpp → GenCodeVisitor gen(out); gen.generate(program);
//   → produce assembly/test05_tipos_char.s

// ============================================================
// Stubs computeAddress — despacho desde nodos l-value del AST
// ============================================================
// Los nodos l-value (Identifier, Index, etc.) tienen computeAddress()
// además de accept(). Solo GenCodeVisitor lo implementa con código real;
// la clase base Visitor define versiones vacías.



// ============================================================
// baseStructName — retorna nombre base del struct
// ============================================================
static string baseStructName(const string& structName) {
    return structName;
}

// ============================================================
// computeAddress(Visitor*) — lvalue nodes only
// ============================================================
void UnaryOpNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void IdentifierNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void IndexNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void MemberAccessNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void ArrowAccessNode::computeAddress(Visitor* v) { v->computeAddress(this); }

// ============================================================
// generate — entry point de generación de código
// ============================================================
// Genera el archivo ensamblador completo:
//   1. Inicializa estructuras de datos (stringLabels, memoriaGlobal)
//   2. Procesa declaraciones de struct y template
//   3. Escribe sección .data con formatos de printf y variables globales
//   4. Escribe sección .text con funciones
//   5. Incluye función auxiliar `potencia` (para operador ^)
//   6. Nota GNU-stack (no ejecutable)
//
//   Ej: GenCodeVisitor gen(out); gen.generate(program);
//       → produce código ASM en el ostream `out`
void GenCodeVisitor::generate(Program *p) {
    stringLabels.clear();
    usedPow = false;
    for (auto g : p->globals)
        memoriaGlobal[g->name] = true;

    // Procesar structs primero para inicializar los mapas de offsets
    for (auto s : p->structs)
        s->accept(this);

    // --- Sección .data ---
    out << ".data\n";
    out << "print_fmt: .string \"%ld\\n\"\n";    // formato por defecto para printf (int/bool/ptr)
    out << "print_fmt_float: .string \"%f\\n\"\n"; // formato por defecto para float/double
    out << "println_fmt: .string \"\\n\"\n";     // salto de línea

    // Variables globales (inicializadas a 0)
    //   Ej: int x; → x: .quad 0
    for (auto &[name, _] : memoriaGlobal)
        out << name << ": .quad 0\n";

    // --- Sección .text ---
    out << "\n.text\n";

    // Generar código para cada función declarada
    for (auto f : p->functions)
        f->accept(this);

    // --- Función auxiliar: potencia(base^exp) ---
    // Solo se emite si el código realmente usa **
    if (usedPow) {
        out << "\npotencia:\n";
        out << "  pushq %rbp\n";
        out << "  movq %rsp, %rbp\n";
        out << "  cmpq $0, %rsi\n";
        out << "  jne .pot_nz\n";
        out << "  movq $1, %rax\n";       // x^0 = 1
        out << "  jmp .pot_end\n";
        out << ".pot_nz:\n";
        out << "  cmpq $1, %rsi\n";
        out << "  jne .pot_rec\n";
        out << "  movq %rdi, %rax\n";     // x^1 = x
        out << "  jmp .pot_end\n";
        out << ".pot_rec:\n";
        out << "  pushq %rbx\n";
        out << "  movq %rdi, %rbx\n";     // guardar base original
        out << "  testq $1, %rsi\n";
        out << "  jz .pot_even\n";        // exponente par?
        // Exponencial impar: result = base * (base^2)^(exp/2)
        out << "  imulq %rdi, %rdi\n";    // base^2
        out << "  sarq $1, %rsi\n";       // exp / 2
        out << "  call potencia\n";
        out << "  imulq %rbx, %rax\n";    // multiplicar por base original
        out << "  popq %rbx\n";
        out << "  jmp .pot_end\n";
        // Exponencial par: result = (base^2)^(exp/2)
        out << ".pot_even:\n";
        out << "  imulq %rdi, %rdi\n";    // base^2
        out << "  sarq $1, %rsi\n";       // exp / 2
        out << "  call potencia\n";
        out << "  popq %rbx\n";
        out << ".pot_end:\n";
        out << "  leave\n";
        out << "  ret\n";
    }

    out << "\n.section .note.GNU-stack,\"\",@progbits\n";
}

// ============================================================
//  Helpers: sufijos e instrucciones según tamaño
// ============================================================

// instrSuffix: retorna el sufijo de instrucción AT&T según tamaño
//   1 → "b" (byte),   4 → "l" (long/word de 32 bits)
//   8 → "q" (quad/64 bits)
string GenCodeVisitor::instrSuffix(int size) {
    switch (size) {
        case 1: return "b";
        case 4: return "l";
        case 8: return "q";
        default: return "q";
    }
}

// loadInstr: instrucción de carga con extensión según tamaño
//   1 → "movzbq" (zero-extend byte a quad)
//   4 → "movslq" (sign-extend long a quad)
//   8 → "movq"   (carga directa de quad)
//
//   Ej: loadInstr(1) → movzbq (%rax), %rax  (carga byte sin signo)
//       loadInstr(4) → movslq (%rax), %rax  (carga int con signo)
string GenCodeVisitor::loadInstr(int size) {
    switch (size) {
        case 1: return "movzbq";  // zero-extend 1 byte -> 8 bytes
        case 4: return "movslq";  // sign-extend 4 bytes -> 8 bytes
        case 8: return "movq";    // 8 bytes (punteros, double)
        default: return "movq";
    }
}

// storeInstr: instrucción de almacenamiento según tamaño
//   1 → "movb" (almacena byte), 4 → "movl" (almacena 32 bits)
//   8 → "movq" (almacena 64 bits)
//
//   Ej: storeInstr(1) → movb %al, (%rax)
//       storeInstr(4) → movl %eax, (%rax)
string GenCodeVisitor::storeInstr(int size) {
    switch (size) {
        case 1: return "movb";
        case 4: return "movl";
        case 8: return "movq";
        default: return "movq";
    }
}

// ============================================================
// Helpers: detección de tipos float/double
// ============================================================

// isFloatSemanticType: true si el tipo es float o double
//   Se usa en operaciones aritméticas para decidir si usar
//   instrucciones de punto flotante (addss/mulsd/etc.) o enteras.
//
//   Ej: isFloatSemanticType(IntType) → false
//       isFloatSemanticType(FloatType) → true
//       isFloatSemanticType(DoubleType) → true
static bool isFloatSemanticType(Type* t) {
    return t && (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE);
}

// ============================================================
// bindingMem — obtener representación de memoria de una variable
// ============================================================
// Retorna el string que representa la dirección de una variable
// en ensamblador, en formato AT&T.
//
// Casos:
//   Variable global:  "nombre(%rip)"  (RIP-relative)
//   Variable lambda:  "offset(%rbp)"  (desde mapa memoria local)
//   Variable normal:  "offset(%rbp)"  (offset calculado por TypeChecker)
//
//   Ej: int x global → "x(%rip)"
//       int x local (offset=-8) → "-8(%rbp)"
string GenCodeVisitor::bindingMem(VarDecl* vd) const {
    if (!vd)
        throw runtime_error("Variable sin binding en codegen");
    if (memoriaGlobal.count(vd->name))
        return vd->name + "(%rip)";
    // Offset negativo calculado por TypeChecker (bin packing en visit(FunDecl)).
    return to_string(vd->offset) + "(%rbp)";
}

// ============================================================
// bind_var_decl — registrar variable local en el mapa memoria
// ============================================================
// Guarda el offset de una variable local en el mapa `memoria`.
// Esto permite acceder a variables de lambda que pueden no
// tener el offset en vd (porque se asignan manualmente).
//
//   Ej: bind_var_decl(vd) → memoria["x"] = vd->offset
void GenCodeVisitor::bind_var_decl(VarDecl* vd) {
    if (!vd) return;
    memoria[vd->name] = vd->offset;
}

// ============================================================
// array_elem_size — obtener tamaño del elemento base de un arreglo
// ============================================================
// Para un arreglo multidimensional, extrae el tamaño del tipo
// base (el elemento más interno). Ejemplos:
//
//   int a[5]       → elem_size = 4 (int)
//   int m[3][4]    → elem_size = 4 (int)
//   char s[10]     → elem_size = 1 (char)
//   double v[5]    → elem_size = 8 (double)
//   int* p         → elem_size = 4 (int, vía puntero)
//
// Se usa para calcular el stride y la instrucción de carga/almacenamiento.
int GenCodeVisitor::array_elem_size(VarDecl* vd) const {
    if (!vd || !vd->resolvedType) return 8;
    Type* t = vd->resolvedType;
    // Recorre ArrayTypes anidados hasta el tipo base
    if (t->ttype == Type::ARRAY) {
        Type* base = t;
        while (base && base->ttype == Type::ARRAY)
            base = ((ArrayType*)base)->base;
        if (base) return base->size();
    }
    // Si es puntero, el elemento es el tipo apuntado
    if (t->ttype == Type::POINTER && ((PointerType*)t)->base)
        return ((PointerType*)t)->base->size();
    return 8;
}

// ============================================================
// arrayDimsFor — obtener dimensiones de un arreglo
// ============================================================
// Extrae las dimensiones de un ArrayType anidado.
// Retorna vector con los tamaños de cada dimensión.
//
//   Ej: int m[2][3] → ArrayType(ArrayType(IntType, 3), 2)
//       arrayDimsFor(m) → [2, 3]
//
//       int a[5] → ArrayType(IntType, 5)
//       arrayDimsFor(a) → [5]
//
//       int* p → [] (no es arreglo)
// arrayDimsFromType — obtener dimensiones desde un ArrayType
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
    vector<int> dims;
    Type* t = vd->resolvedType;
    while (t && t->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)t;
        if (at->length > 0) dims.push_back(at->length);
        t = at->base;
    }
    return dims;
}

// ============================================================
// collectIndices — recolectar índices anidados de arr[i][j][k]
// ============================================================
// Dada una cadena de IndexNodes (a[i][j][k]), extrae todos
// los índices y la expresión base. Útil para arreglos
// multidimensionales.
//
//   Ej: a[i][j] → indices = [i, j], base = a
//       b[x]    → indices = [x],    base = b
static void collectIndices(Exp* e, vector<Exp*>& indices, Exp*& base) {
    Exp* b = e;
    while (auto* inner = dynamic_cast<IndexNode*>(b)) {
        indices.push_back(inner->index);
        b = inner->base;
    }
    reverse(indices.begin(), indices.end());
    base = b;
}

// ============================================================
// loadBinding — cargar valor de variable en %rax
// ============================================================
// Genera código para cargar el valor de una variable en %rax.
// Maneja: enteros (movzbq/movslq/movq), floats (movss/movsd).
//
//   Ej: int x → movslq x(%rip), %rax
//       float f → movss f(%rip), %xmm7; movd %xmm7, %eax
//       double d → movsd d(%rip), %xmm7; movq %xmm7, %rax
void GenCodeVisitor::loadBinding(VarDecl* vd) {
    if (!vd) return;
    int size = vd->memSize > 0 ? vd->memSize : 8;
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
    out << "  " << loadInstr(size) << " " << bindingMem(vd) << ", %rax\n";
}

// ============================================================
// storeBinding — guardar %eax/%rax en variable
// ============================================================
// Genera código para almacenar el valor en %rax en la variable.
// Maneja: enteros (movb/movl/movq), floats (movss/movsd).
//
//   Ej: int x → movl %eax, x(%rip)
//       float f → movd %eax, %xmm7; movss %xmm7, f(%rip)
void GenCodeVisitor::storeBinding(VarDecl* vd) {
    if (!vd) return;
    int size = vd->memSize > 0 ? vd->memSize : 8;
    Type* type = vd->resolvedType;
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
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
    out << "  " << storeInstr(size) << " " << reg << ", " << bindingMem(vd) << "\n";
}

// ============================================================
// leaBinding — cargar dirección de variable en %rax
// ============================================================
// Genera leaq para obtener la dirección efectiva de la variable.
// Útil para &x y para pasar direcciones.
//
//   Ej: int x → leaq -8(%rbp), %rax
void GenCodeVisitor::leaBinding(VarDecl* vd) {
    if (!vd) return;
    out << "  leaq " << bindingMem(vd) << ", %rax\n";
}

// ============================================================
// emitIndexedAddress — calcular dirección de a[i][j] en %rax
// ============================================================
// Genera código para calcular la dirección efectiva de un
// elemento en un arreglo multidimensional.
//
// Fórmula: &a + (i * dims[1] + j) * elemSize
//
// Para 1D: usa scaled index (lea (%rax,%rdi,4))
// Para ND: suma manual con strides
//
//   Ej: int m[2][3]; m[1][2]
//       → stride = 3, índice lineal = 1*3 + 2 = 5
//       → leaq (%rax,%r10,4), %rax  (donde r10=5)
void GenCodeVisitor::emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);

    if (dims.size() > 1) {
        // Caso multidimensional: calcular índice lineal manualmente
        leaBinding(vd);
        out << "  movq %rax, %r8\n";
        out << "  movq $0, %r10\n";
        for (size_t d = 0; d < indices.size(); d++) {
            indices[d]->accept(this);
            int stride = 1;
            for (size_t s = d + 1; s < dims.size(); s++)
                stride *= dims[s];
            if (stride != 1) {
                out << "  movq $" << stride << ", %rcx\n";
                out << "  imulq %rcx, %rax\n";
            }
            out << "  addq %rax, %r10\n";
        }
        out << "  movq %r8, %rax\n";
        if (elemSize == 1) out << "  addq %r10, %rax\n";
        else if (elemSize == 2) out << "  leaq (%rax,%r10,2), %rax\n";
        else if (elemSize == 4) out << "  leaq (%rax,%r10,4), %rax\n";
        else out << "  leaq (%rax,%r10,8), %rax\n";
        return;
    }

    // Caso 1D: usar direccionamiento indexado (%base, %indice, escala)
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    string scale;
    if (elemSize == 1) scale = "";
    else if (elemSize == 2) scale = ",2";
    else if (elemSize == 4) scale = ",4";
    else scale = ",8";
    out << "  leaq (%rax,%rdi" << scale << "), %rax\n";
}

// ============================================================
// emitIndexedLoad — cargar a[i][j] en %rax
// ============================================================
// Similar a emitIndexedAddress pero carga el valor en lugar
// de calcular la dirección.
//
//   Ej: int a[5]; a[3] → movslq (%rax,%rdi,4), %rax
//       int m[2][3]; m[1][2] → calcula dirección, luego movslq (%rax), %rax
void GenCodeVisitor::emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);

    if (dims.size() > 1) {
        emitIndexedAddress(vd, indices);
        if (indices.size() < dims.size()) return; // result is array → leave address
        if (elemSize == 1) out << "  movzbq (%rax), %rax\n";
        else if (elemSize == 4) out << "  movslq (%rax), %rax\n";
        else out << "  movq (%rax), %rax\n";
        return;
    }

    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    string scale;
    if (elemSize == 1) scale = "";
    else if (elemSize == 2) scale = ",2";
    else if (elemSize == 4) scale = ",4";
    else scale = ",8";
    out << "  " << loadInstr(elemSize) << " (%rax,%rdi" << scale << "), %rax\n";
}

// ============================================================
// emitIndexedStore — guardar en a[i][j]
// ============================================================
// Similar a emitIndexedLoad pero almacena el valor de valueReg
// en la posición calculada.
//
//   Ej: a[3] = val → movl %ecx, (%rax,%rdi,4)
void GenCodeVisitor::emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices,
                                      const string& valueReg) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);
    string reg = valueReg;
    if (elemSize == 1 && valueReg == "%rax") reg = "%al";
    else if (elemSize == 4 && valueReg == "%rax") reg = "%eax";

    if (dims.size() > 1) {
        out << "  movq %rax, %r9\n";
        emitIndexedAddress(vd, indices);
        out << "  movq %rax, %rbx\n";
        string valReg = "%r9";
        if (elemSize == 1) valReg = "%r9b";
        else if (elemSize == 4) valReg = "%r9d";
        out << "  " << storeInstr(elemSize) << " " << valReg << ", (%rbx)\n";
        return;
    }

    out << "  movq %rax, %rcx\n";
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    string valReg = "%rcx";
    if (elemSize == 1) valReg = "%cl";
    else if (elemSize == 4) valReg = "%ecx";

    string scale;
    if (elemSize == 1) scale = "";
    else if (elemSize == 2) scale = ",2";
    else if (elemSize == 4) scale = ",4";
    else scale = ",8";
    out << "  " << storeInstr(elemSize) << " " << valReg << ", (%rax,%rdi" << scale << ")\n";
}

// ============================================================
// captureLVal — capturar l-value de una expresión
// ============================================================
// Evalúa una expresión como l-value, llenando la estructura
// LVal con la información necesaria para leer o escribir.
// Se usa en asignaciones (x = expr) y operadores ++/--.
//
//   Ej: captureLVal(x) → LVal{Id, name="x", binding=&x}
//       captureLVal(a[i]) → LVal{Index, binding=&a, indices=[i]}
// Activa el “modo l-value”: mientras lvalTarget != nullptr, visit(Identifier/Index/...)
// no cargan valor sino que rellenan la estructura LVal para storeTarget.
GenCodeVisitor::LVal GenCodeVisitor::captureLVal(Exp *e) {
    LVal lv;
    lvalTarget = &lv;
    e->accept(this);
    lvalTarget = nullptr;
    return lv;
}

// ============================================================
// storeTarget — escribir %rax/%eax en la dirección del l-value
// ============================================================
// Según el tipo de l-value, genera el código apropiado para
// almacenar el valor actual en %rax:
//
//   Id:     storeBinding (carga offset desde VarDecl)
//   Index:  emitIndexedStore (cálculo de índice + store)
//   Member: store en offset del struct (vía lea o load)
//   Deref:  pop dirección, guardar (%rbx) = %rax
//
//   Ej: x = 5 → storeBinding(x)
//       a[3] = 5 → emitIndexedStore(a, [3], "%rax")
//       s.x = 5 → movl %ecx, 0(%rax) (después de lea)
void GenCodeVisitor::storeTarget(const LVal &lv) {
    switch (lv.kind) {
    case LValKind::Id:
        storeBinding(lv.binding);
        break;
    case LValKind::Index:
    {
        vector<Exp*> indices = lv.indices;
        if (indices.empty() && lv.index)
            indices.push_back(lv.index);

        if (!lv.members.empty()) {
            out << "  movq %rax, %rcx\n"; // Save value in %rcx for member path
            // Array is a member of a struct: compute base via member chain + index offset
            string structName = lv.structName;
            if (lv.isArrow && lv.binding) {
                loadBinding(lv.binding);
            } else if (lv.binding) {
                leaBinding(lv.binding);
            }
            // Determine final member type for dims/elemSize
            Type* finalMemberType = nullptr;
            if (!lv.members.empty() && lv.binding &&
                structMemberTypes.count(lv.structName) &&
                structMemberTypes[lv.structName].count(lv.members.back())) {
                finalMemberType = structMemberTypes[lv.structName][lv.members.back()];
            }
            // Apply member offsets, loading for nested arrows
            for (size_t i = 0; i < lv.members.size(); ++i) {
                const string& m = lv.members[i];
                if (i > 0 && i < lv.memberArrow.size() && lv.memberArrow[i]) {
                    out << "  movq (%rax), %rax\n";
                }
                int off = structFieldOffset[baseStructName(structName)][m];
                out << "  addq $" << off << ", %rax\n";
                if (structMemberTypes.count(baseStructName(structName)) &&
                    structMemberTypes[baseStructName(structName)].count(m)) {
                    Type* mt = structMemberTypes[baseStructName(structName)][m];
                    if (mt->ttype == Type::STRUCT) {
                        structName = ((StructType*)mt)->name;
                    } else if (mt->ttype == Type::POINTER) {
                        PointerType* pt = (PointerType*)mt;
                        if (pt->base && pt->base->ttype == Type::STRUCT) {
                            structName = ((StructType*)pt->base)->name;
                        }
                    }
                }
            }
            // If final member is a pointer, load to get array address
            if (finalMemberType && finalMemberType->ttype == Type::POINTER) {
                out << "  movq (%rax), %rax\n";
            }
            // Get array dims and elemSize from the final member type
            vector<int> dims = (finalMemberType && finalMemberType->ttype == Type::ARRAY)
                ? arrayDimsForType(finalMemberType) : vector<int>();
            int elemSize = 4;
            if (finalMemberType && finalMemberType->ttype == Type::ARRAY) {
                ArrayType* at = static_cast<ArrayType*>(finalMemberType);
                while (at->base && at->base->ttype == Type::ARRAY) at = static_cast<ArrayType*>(at->base);
                if (at->base) elemSize = at->base->size();
            } else if (finalMemberType && finalMemberType->ttype == Type::POINTER) {
                PointerType* pt = static_cast<PointerType*>(finalMemberType);
                if (pt->base) elemSize = pt->base->size();
            }
            // Compute linear index with proper strides
            out << "  pushq %rax\n";
            out << "  movq $0, %r10\n";
            for (size_t d = 0; d < indices.size(); d++) {
                indices[d]->accept(this);
                int stride = 1;
                if (dims.size() > 1 && d + 1 < dims.size()) {
                    for (size_t s = d + 1; s < dims.size(); s++)
                        stride *= dims[s];
                }
                if (stride != 1) {
                    out << "  movq $" << stride << ", %rdi\n";
                    out << "  imulq %rdi, %rax\n";
                }
                out << "  addq %rax, %r10\n";
            }
            out << "  popq %rax\n";
            // %rax = base address, %r10 = linear index, elemSize = element size
            if (elemSize == 4) {
                out << "  leaq (%rax,%r10,4), %rax\n";
            } else if (elemSize == 8) {
                out << "  leaq (%rax,%r10,8), %rax\n";
            } else if (elemSize == 1) {
                out << "  addq %r10, %rax\n";
            } else {
                out << "  movq %rax, %rdi\n";
                out << "  movq %r10, %rax\n";
                out << "  movq $" << elemSize << ", %r11\n";
                out << "  imulq %r11, %rax\n";
                out << "  addq %rdi, %rax\n";
            }
            string reg = (elemSize == 1) ? "%cl" : (elemSize == 4) ? "%ecx" : "%rcx";
            out << "  " << storeInstr(elemSize) << " " << reg << ", (%rax)\n";
        } else {
            emitIndexedStore(lv.binding, indices, "%rax");
        }
        break;
    }
    case LValKind::Member:
    {
        out << "  movq %rax, %rcx\n"; // Save value in %rcx

        string structName = lv.structName;
        // First, get base address in %rax
        if (lv.isArrow && lv.binding) {
            loadBinding(lv.binding); // p->m, load p first
        } else if (lv.binding) {
            leaBinding(lv.binding); // s.m, get &s
        }

        // Apply ALL member offsets to compute address of final member.
        // For members accessed via -> (arrow), first load the pointer value
        // at the current address before adding the offset (nested arrows).
        for (size_t i = 0; i < lv.members.size(); ++i) {
            const string& m = lv.members[i];
            // If this member was accessed via -> and it's not the first level
            // (first level handled by initial loadBinding for isArrow),
            // load the pointer value at current address before adding offset.
            if (i > 0 && i < lv.memberArrow.size() && lv.memberArrow[i]) {
                out << "  movq (%rax), %rax\n";
            }
            int off = structFieldOffset[baseStructName(structName)][m];
            out << "  addq $" << off << ", %rax\n";
            // Update structName to type of this member for next iteration
            if (structMemberTypes.count(baseStructName(structName)) &&
                structMemberTypes[baseStructName(structName)].count(m)) {
                Type* mt = structMemberTypes[baseStructName(structName)][m];
                if (mt->ttype == Type::STRUCT) {
                    structName = ((StructType*)mt)->name;
                } else if (mt->ttype == Type::POINTER) {
                    PointerType* pt = (PointerType*)mt;
                    if (pt->base && pt->base->ttype == Type::STRUCT) {
                        structName = ((StructType*)pt->base)->name;
                    }
                }
            }
        }

        // If there are indices (member is an array), apply the index offset
        if (!lv.indices.empty()) {
            out << "  pushq %rcx\n"; // save value
            out << "  pushq %rax\n"; // save base address of the array member
            // strideElemSize: stride for index computation
            // If binding is an array, indices index into it (e.g. arreglo[i].id)
            int strideElemSize = 4;
            if (lv.binding && lv.binding->resolvedType &&
                lv.binding->resolvedType->ttype == Type::ARRAY) {
                strideElemSize = array_elem_size(lv.binding);
            } else if (!lv.members.empty() &&
                structMemberTypes.count(baseStructName(structName)) &&
                structMemberTypes[baseStructName(structName)].count(lv.members.back())) {
                Type* mt = structMemberTypes[baseStructName(structName)][lv.members.back()];
                if (mt && mt->ttype == Type::ARRAY) {
                    ArrayType* at = static_cast<ArrayType*>(mt);
                    if (at->base) strideElemSize = at->base->size();
                }
            }

            // Compute linear offset from indices (como emitIndexedAddress)
            out << "  movq $0, %r10\n";
            for (size_t d = 0; d < lv.indices.size(); d++) {
                lv.indices[d]->accept(this);
                if (strideElemSize != 1) {
                    out << "  movq $" << strideElemSize << ", %rcx\n";
                    out << "  imulq %rcx, %rax\n";
                }
                out << "  addq %rax, %r10\n";
            }
            out << "  popq %rax\n";
            out << "  addq %r10, %rax\n";
            out << "  popq %rcx\n"; // restore value

            // Store value (%rcx) at final address — use member size, not stride
            int storeSize = 4;
            if (!lv.members.empty()) {
                storeSize = structMemberSizes[baseStructName(
                    lv.structName.empty() ? structName : lv.structName)][lv.members.back()];
            }
            string reg = (storeSize == 1) ? "%cl" : (storeSize == 4) ? "%ecx" : "%rcx";
            out << "  " << storeInstr(storeSize) << " " << reg << ", (%rax)\n";
        } else {
            // No indices: store at the final member address (%rax = &member)
            int size = structMemberSizes[baseStructName(structName)][lv.members.back()];
            string reg = (size == 1) ? "%cl" : (size == 4) ? "%ecx" : "%rcx";
            out << "  " << storeInstr(size) << " " << reg << ", (%rax)\n";
        }
        break;
    }
    case LValKind::Deref:
        // *p = val: pop dirección previamente pusheada en visit(UnaryOp)
        out << "  popq %rbx\n";
        out << "  movq %rax, (%rbx)\n";
        break;
    default:
        throw runtime_error("Asignación a expresión que no es lvalue");
    }
}

// ============================================================
// directStoreForConstant — store directo a variable con valor constante
// ============================================================
// Si `value` es un literal o una expresión ya evaluada por ConstantFolding
// (isConstant == true), emite el store directo a memoria usando la constante
// como inmediato, sin pasar por %rax. Retorna true si emitió, false en caso
// contrario (y se usa el camino normal: eval -> %rax -> store).
//
// Casos cubiertos:
//   IntegerLiteral, BoolLiteral, CharLiteral, FloatLiteral, isConstant
// Destinos cubiertos:
//   int (4B), char/bool (1B), long (8B), float (4B)
//   double no aplica: imm64 no puede ir directo a memoria en x86-64.
//
//   Ej: x = 5;       -> movl $5, -8(%rbp)
//       int x = 5;   -> movl $5, -8(%rbp)
//       float f = 3.14; -> movl $0x4048f5c3, -4(%rbp)
bool GenCodeVisitor::directStoreForConstant(Exp* value, VarDecl* vd) {
    if (!value || !vd || !vd->resolvedType) return false;

    long long intVal = 0;
    unsigned int floatBits = 0;
    bool hasFloatBits = false;

    if (auto* p = dynamic_cast<IntegerLiteralNode*>(value)) {
        intVal = (long long)p->value;
    } else if (auto* p = dynamic_cast<BoolLiteralNode*>(value)) {
        intVal = p->value ? 1 : 0;
    } else if (auto* p = dynamic_cast<CharLiteralNode*>(value)) {
        intVal = (int)p->value;
    } else if (auto* p = dynamic_cast<FloatLiteralNode*>(value)) {
        union { float f; unsigned int i; } fc;
        fc.f = (float)p->value;
        floatBits = fc.i;
        hasFloatBits = true;
        intVal = (long long)fc.i;
    } else if (value->isConstant) {
        intVal = (long long)value->constantValue;
    } else {
        return false;
    }

    Type* tt = vd->resolvedType;
    int size = vd->memSize > 0 ? vd->memSize : tt->size();
    string mem = bindingMem(vd);

    if (tt->ttype == Type::FLOAT) {
        if (!hasFloatBits) {
            union { float f; unsigned int i; } fc;
            fc.f = (float)intVal;
            floatBits = fc.i;
        }
        out << "  movl $0x" << hex << floatBits << dec << ", " << mem << "\n";
        return true;
    }

    if (tt->ttype == Type::DOUBLE) {
        return false;
    }

    string instr = (size == 1) ? "movb" : (size == 4) ? "movl" : "movq";
    out << "  " << instr << " $" << intVal << ", " << mem << "\n";
    return true;
}

// ============================================================
//  Expressions
// ============================================================

// -----------------------------------------------------------
// Literales — cargan constantes directamente en %rax
// -----------------------------------------------------------

// Entero: movq $N, %rax (siempre 64 bits en registro; el store trunca si hace falta).
void GenCodeVisitor::visit(IntegerLiteralNode *e) {
    out << "  movq $" << e->value << ", %rax\n";
}

// Float: se empaqueta como bits IEEE-754 en %eax y se extiende a %rax.
// No usa %xmm aquí para mantener la convención “resultado en %rax”.
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

void GenCodeVisitor::visit(BoolLiteralNode *e) {
    // bool se representa como 0/1 en %rax (1 byte lógico, registro completo).
    out << "  movq $" << (e->value ? 1 : 0) << ", %rax\n";
}

void GenCodeVisitor::visit(CharLiteralNode *e) {
    // Valor ASCII en %rax; storeBinding usará movb si el destino es char.
    out << "  movq $" << (int)e->value << ", %rax\n";
}

void GenCodeVisitor::visit(StringLiteralNode *e) {
    // Los strings se almacenan en .rodata (read-only data).
    // Cada string único tiene un label .LstrN.
    // Se carga la dirección con leaq.
    //
    //   Ej: printf("hola") → leaq .Lstr0(%rip), %rax
    auto it = stringLabels.find(e->value);
    int lbl;
    if (it == stringLabels.end()) {
        lbl = (int)stringLabels.size();
        stringLabels[e->value] = lbl;
        out << ".section .rodata\n";
        out << ".Lstr" << lbl << ": .string \"" << e->value << "\"\n";
        out << ".text\n";
    } else {
        lbl = it->second;
    }
    out << "  leaq .Lstr" << lbl << "(%rip), %rax\n";
}

// -----------------------------------------------------------
// visit(IdentifierNode) — carga de variable o captura l-value
// -----------------------------------------------------------
// Si estamos en modo l-value (lvalTarget != null), guarda el
// binding y retorna. Si no, carga el valor de la variable en %rax.
void GenCodeVisitor::visit(IdentifierNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;
        lvalTarget->name = e->name;
        lvalTarget->binding = e->binding;
        return;
    }
    loadBinding(e->binding);
}

// -----------------------------------------------------------
// visit(BinaryOpNode) — operaciones binarias
// -----------------------------------------------------------
// Genera código para operaciones binarias:
//   - Constantes: si isConstant, genera directamente el valor
//   - Potencia: optimización para x^2 y x^4 (imulq), else call potencia()
//   - Punto flotante: usa SSE (addss, addsd, etc.) con promoción
//   - Enteras: operaciones normales con extensión de signo cuando corresponda
//
// Flujo para enteros:
//   left → push %rax; right → %rcx; pop → %rax; op %rcx, %rax
//   Ej: a + b → push %rax; b → %rcx; pop %rax; addl %ecx, %eax
//
// Flujo para float/double:
//   left → %xmm0; right → %xmm1; op %xmm1, %xmm0; → %rax
void GenCodeVisitor::visit(BinaryOpNode *e) {
    // Si constante (después de constant folding), emitir directamente
    if (e->isConstant) {
        double val = e->constantValue;
        if (e->resolvedType && isFloatSemanticType(e->resolvedType)) {
            if (e->resolvedType->ttype == Type::DOUBLE) {
                union { double d; unsigned long long i; } dc;
                dc.d = val;
                out << "  movq $0x" << hex << dc.i << dec << ", %rax\n";
            } else {
                union { float f; unsigned int i; } fc;
                fc.f = (float)val;
                out << "  movl $0x" << hex << fc.i << dec << ", %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        } else {
            out << "  movq $" << (long long)val << ", %rax\n";
        }
        return;
    }

    // Optimización para potencias: x^2, x^4
    if (e->op == BinaryOp::POW) {
        usedPow = true;
        auto *rightNum = dynamic_cast<IntegerLiteralNode *>(e->right);
        if (rightNum && rightNum->value == 2) {
            e->left->accept(this);
            out << "  imulq %rax, %rax\n";  // x^2 = x * x
            return;
        }
        if (rightNum && rightNum->value == 4) {
            e->left->accept(this);
            out << "  imulq %rax, %rax\n";
            out << "  imulq %rax, %rax\n";  // x^4 = (x^2)^2
            return;
        }
    }

    Type* leftType = e->left->resolvedType;
    Type* rightType = e->right->resolvedType;
    bool useFloat = false;
    bool useDouble = false;
    // Detectar si la operación involucra floats o doubles
    switch (e->op) {
        case BinaryOp::ADD: case BinaryOp::SUB: case BinaryOp::MUL: case BinaryOp::DIV:
        case BinaryOp::EQ: case BinaryOp::NE: case BinaryOp::LT: case BinaryOp::GT:
        case BinaryOp::LE: case BinaryOp::GE:
            if (isFloatSemanticType(leftType) || isFloatSemanticType(rightType)) {
                useFloat = true;
                useDouble = (leftType && leftType->ttype == Type::DOUBLE) ||
                            (rightType && rightType->ttype == Type::DOUBLE) ||
                            (e->resolvedType && e->resolvedType->ttype == Type::DOUBLE);
            }
            break;
        default:
            break;
    }

    // --- Rama de punto flotante (SSE) ---
    if (useFloat) {
        // Cargar operando izquierdo en %xmm0 (con promoción si es entero)
        e->left->accept(this);
        if (useDouble) {
            if (!isFloatSemanticType(leftType))
                out << "  cvtsi2sd %rax, %xmm0\n";      // int → double
            else if (leftType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm0\n";
                out << "  cvtss2sd %xmm0, %xmm0\n";      // float → double
            } else
                out << "  movq %rax, %xmm0\n";           // double ya
            // Cargar operando derecho en %xmm1
            e->right->accept(this);
            if (!isFloatSemanticType(rightType))
                out << "  cvtsi2sd %rax, %xmm1\n";
            else if (rightType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm1\n";
                out << "  cvtss2sd %xmm1, %xmm1\n";
            } else
                out << "  movq %rax, %xmm1\n";
        } else {
            // float (single precision)
            if (!isFloatSemanticType(leftType))
                out << "  cvtsi2ss %eax, %xmm0\n";      // int → float
            else
                out << "  movd %eax, %xmm0\n";           // float ya
            e->right->accept(this);
            if (!isFloatSemanticType(rightType))
                out << "  cvtsi2ss %eax, %xmm1\n";
            else
                out << "  movd %eax, %xmm1\n";
        }

        // Generar la operación SSE correspondiente
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
        // Comparaciones de punto flotante con ucomiss/ucomisd
        case BinaryOp::EQ:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  sete %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::NE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::LT:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setb %al\n";     // below (unsigned <)
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::GT:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  seta %al\n";     // above (unsigned >)
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::LE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setbe %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::GE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setae %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        default:
            break;
        }

        // Mover resultado SSE a %rax
        if (useDouble)
            out << "  movq %xmm0, %rax\n";
        else {
            out << "  movd %xmm0, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }

    // --- Rama entera y aritmética de punteros ---
    bool leftIsPtr = leftType && leftType->ttype == Type::POINTER;
    bool rightIsPtr = rightType && rightType->ttype == Type::POINTER;

    if ((leftIsPtr && !rightIsPtr) || (!leftIsPtr && rightIsPtr)) {
        // ptr + int o int + ptr o ptr - int
        if (leftIsPtr) {
            e->left->accept(this);  // ptr en %rax
            out << "  pushq %rax\n";
            e->right->accept(this); // int en %rax
            // Multiplicar int por tamaño del tipo base del puntero
            if (leftType && leftType->ttype == Type::POINTER) {
                PointerType* pt = static_cast<PointerType*>(leftType);
                int elemSize = pt->base ? pt->base->size() : 8;
                if (elemSize != 1) {
                    out << "  movq $" << elemSize << ", %rcx\n";
                    out << "  imulq %rcx, %rax\n";
                }
            }
            out << "  popq %rcx\n"; // ptr en %rcx
            if (e->op == BinaryOp::ADD) {
                out << "  addq %rax, %rcx\n";
            } else if (e->op == BinaryOp::SUB) {
                out << "  subq %rax, %rcx\n";
            }
            out << "  movq %rcx, %rax\n";
        } else {
            e->right->accept(this); // ptr en %rax
            out << "  pushq %rax\n";
            e->left->accept(this);  // int en %rax
            // Multiplicar int por tamaño del tipo base del puntero
            if (rightType && rightType->ttype == Type::POINTER) {
                PointerType* pt = static_cast<PointerType*>(rightType);
                int elemSize = pt->base ? pt->base->size() : 8;
                if (elemSize != 1) {
                    out << "  movq $" << elemSize << ", %rcx\n";
                    out << "  imulq %rcx, %rax\n";
                }
            }
            out << "  popq %rcx\n"; // ptr en %rcx
            out << "  addq %rcx, %rax\n";
        }
    } else if (leftIsPtr && rightIsPtr && e->op == BinaryOp::SUB) {
        // ptr - ptr
        e->left->accept(this);
        out << "  pushq %rax\n";
        e->right->accept(this);
        out << "  popq %rcx\n";
        out << "  subq %rax, %rcx\n"; // rcx = left - right (direcciones)
        // Dividir por el tamaño del tipo base
        if (leftType && leftType->ttype == Type::POINTER) {
            PointerType* pt = static_cast<PointerType*>(leftType);
            int elemSize = pt->base ? pt->base->size() : 8;
            if (elemSize != 1) {
                out << "  movq %rcx, %rax\n";
                out << "  movq $" << elemSize << ", %rcx\n";
                out << "  cqto\n";
                out << "  idivq %rcx\n";
            } else {
                out << "  movq %rcx, %rax\n";
            }
        }
    } else {
        // Operaciones normales de enteros
        e->left->accept(this);
        out << "  pushq %rax\n";
        e->right->accept(this);
        out << "  movq %rax, %rcx\n";
        out << "  popq %rax\n";

        // Usar tamaño de los operandos, no del resultado.
        int size = 8;
        if (e->left->resolvedType) {
            size = e->left->resolvedType->size();
            if (e->right->resolvedType) {
                int rsize = e->right->resolvedType->size();
                if (rsize > size) size = rsize;
            }
        } else if (e->resolvedType) {
            size = e->resolvedType->size();
        }

        string suffix = (size == 1) ? "b" : (size == 4) ? "l" : "q";
        string reg1 = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
        string reg2 = (size == 1) ? "%cl" : (size == 4) ? "%ecx" : "%rcx";

        bool isUnsigned = (leftType && leftType->isUnsigned) || (rightType && rightType->isUnsigned);

        switch (e->op) {
        case BinaryOp::ADD: out << "  add" << suffix << " " << reg2 << ", " << reg1 << "\n"; break;
        case BinaryOp::SUB: out << "  sub" << suffix << " " << reg2 << ", " << reg1 << "\n"; break;
        case BinaryOp::MUL: 
            if (isUnsigned) {
                // Unsigned multiply: single-operand form (uses %rdx:%rax)
                // For 64-bit, we need to handle it differently
                if (size == 8) {
                    // For 64-bit unsigned, we use mulq, which takes one operand
                    out << "  mulq " << reg2 << "\n";
                } else {
                    // For smaller sizes, we can use the single-operand form too
                    out << "  mul" << suffix << " " << reg2 << "\n";
                }
            } else {
                out << "  imul" << suffix << " " << reg2 << ", " << reg1 << "\n";
            }
            break;
        case BinaryOp::DIV:
            if (isUnsigned) {
                out << "  xorq %rdx, %rdx\n";  // zero-extend %rax → %rdx:%rax
                out << "  div" << suffix << " " << reg2 << "\n";
            } else {
                out << "  cqto\n";          // sign-extend %rax → %rdx:%rax
                out << "  idiv" << suffix << " " << reg2 << "\n";
            }
            break;
        case BinaryOp::MOD:
            if (isUnsigned) {
                out << "  xorq %rdx, %rdx\n";
                out << "  div" << suffix << " " << reg2 << "\n";
            } else {
                out << "  cqto\n";
                out << "  idiv" << suffix << " " << reg2 << "\n";
            }
            out << "  movq %rdx, %rax\n";  // módulo queda en %rdx
            break;
        case BinaryOp::POW:
            // Llamar a la función potencia(base=%rax, exp=%rcx)
            out << "  movq %rax, %rdi\n";
            out << "  movq %rcx, %rsi\n";
            out << "  call potencia\n";
            break;
        case BinaryOp::EQ:
            out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  sete %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::NE:
            out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::LT:
            out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  set" << (isUnsigned ? "b" : "l") << " %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::GT:
            out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  set" << (isUnsigned ? "a" : "g") << " %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::LE:
            out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  set" << (isUnsigned ? "be" : "le") << " %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::GE:
            out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  set" << (isUnsigned ? "ae" : "ge") << " %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::LOG_AND:
            out << "  and" << suffix << " " << reg2 << ", " << reg1 << "\n";
            break;
        case BinaryOp::LOG_OR:
            out << "  or" << suffix << " " << reg2 << ", " << reg1 << "\n";
            break;
        }
    }
}

// -----------------------------------------------------------
// visit(UnaryOpNode) — operaciones unarias
// -----------------------------------------------------------
// - MINUS: neg (negación aritmética)
// - LOG_NOT: cmp $0, %rax; sete (negación lógica)
// - PRE_INC/PRE_DEC: inc/dec y store
// - POST_INC/POST_DEC: push valor original, inc/dec, store, pop
// - ADDR (&): leaBinding (dirección de variable)
// - DEREF (*): movq (%rax), %rax (carga desde puntero)
//   En modo l-value: push dirección para storeTarget
void GenCodeVisitor::visit(UnaryOpNode *e) {
    // Modo l-value para *p = expr: evalúa p (dirección en %rax), la guarda en stack
    // para que storeTarget haga pop y escriba ahí.
    if (lvalTarget && e->op == UnaryOp::DEREF) {
        lvalTarget->kind = LValKind::Deref;
        lvalTarget = nullptr;
        e->operand->accept(this);
        out << "  pushq %rax\n";  // dirección será popeada por storeTarget
        return;
    }

    // Constante (después de constant folding)
    if (e->isConstant) {
        double val = e->constantValue;
        if (e->resolvedType && isFloatSemanticType(e->resolvedType)) {
            if (e->resolvedType->ttype == Type::DOUBLE) {
                union { double d; unsigned long long i; } dc;
                dc.d = val;
                out << "  movq $0x" << hex << dc.i << dec << ", %rax\n";
            } else {
                union { float f; unsigned int i; } fc;
                fc.f = (float)val;
                out << "  movl $0x" << hex << fc.i << dec << ", %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        } else {
            out << "  movq $" << (long long)val << ", %rax\n";
        }
        return;
    }

    // ADDR: calcular dirección del operando (no evaluar como r-value)
    if (e->op == UnaryOp::ADDR) {
        e->operand->computeAddress(this);
        return;
    }

    e->operand->accept(this);

    // Tamaño según tipo del resultado de la operación (TypeChecker).
    // Para - sobre char, resolvedType es int → negl; para ++ sobre char → incb.
    int size = e->resolvedType ? e->resolvedType->size() : 8;

    string suffix = (size == 1) ? "b" : (size == 4) ? "l" : "q";
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";

    switch (e->op) {
    case UnaryOp::MINUS:
        // Para float/double: negación SSE (flip sign bit via xorps/xorpd)
        if (e->resolvedType && isFloatSemanticType(e->resolvedType)) {
            if (e->resolvedType->ttype == Type::DOUBLE) {
                out << "  movq %rax, %xmm0\n";
                out << "  movabsq $0x8000000000000000, %rcx\n";
                out << "  movq %rcx, %xmm7\n";
                out << "  xorpd %xmm7, %xmm0\n";
                out << "  movq %xmm0, %rax\n";
            } else {
                out << "  movd %eax, %xmm0\n";
                out << "  movl $0x80000000, %ecx\n";
                out << "  movd %ecx, %xmm7\n";
                out << "  xorps %xmm7, %xmm0\n";
                out << "  movd %xmm0, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        } else {
            out << "  neg" << suffix << " " << reg << "\n";
        }
        break;
    case UnaryOp::LOG_NOT:
        out << "  cmpq $0, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  sete %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case UnaryOp::PRE_INC:
    {
        // ++x: incrementa y guarda el nuevo valor
        LVal preLv = captureLVal(e->operand);
        out << "  inc" << suffix << " " << reg << "\n";
        storeTarget(preLv);
        break;
    }
    case UnaryOp::PRE_DEC:
    {
        // --x: decrementa y guarda
        LVal preLv = captureLVal(e->operand);
        out << "  dec" << suffix << " " << reg << "\n";
        storeTarget(preLv);
        break;
    }
    case UnaryOp::POST_INC:
    {
        // x++: guarda valor original (push), incrementa, store, restaura (pop)
        LVal postLv = captureLVal(e->operand);
        out << "  pushq %rax\n";
        out << "  inc" << suffix << " " << reg << "\n";
        storeTarget(postLv);
        out << "  popq %rax\n";
        break;
    }
    case UnaryOp::POST_DEC:
    {
        LVal postLv = captureLVal(e->operand);
        out << "  pushq %rax\n";
        out << "  dec" << suffix << " " << reg << "\n";
        storeTarget(postLv);
        out << "  popq %rax\n";
        break;
    }
    case UnaryOp::DEREF:
        // *p: cargar valor apuntado
        out << "  movq (%rax), %rax\n";
        break;
    default:
        break;
    }
}

// -----------------------------------------------------------
// visit(AssignmentNode) — asignación
// -----------------------------------------------------------
// Flujo:
//   1. Capturar target como l-value (LVal)
//   2. Evaluar value → %rax
//   3. Promocionar float/double si es necesario
//   4. storeTarget escribe %rax en la dirección del l-value
//
//   Ej: x = 5 → captureLVal(x); eval(5); storeBinding(x)
//       a[i] = 3 → captureLVal(a[i]); eval(3); emitIndexedStore
void GenCodeVisitor::visit(AssignmentNode *e) {
    LVal target = captureLVal(e->target);

    // Atajo: si el destino es una variable simple y el valor es constante,
    // emitir el store directo (sin pasar por %rax).
    if (target.kind == LValKind::Id && target.binding) {
        if (directStoreForConstant(e->value, target.binding))
            return;
    }

    e->value->accept(this); // value in %rax

    // Promociones automáticas según el tipo del destino
    if (target.kind == LValKind::Id && target.binding) {
        Type* tgtType = target.binding->resolvedType;
        Type* valType = e->value->resolvedType;
        if (tgtType && tgtType->ttype == Type::DOUBLE) {
            if (valType && valType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm7\n";
                out << "  cvtss2sd %xmm7, %xmm7\n";  // float → double
                out << "  movq %xmm7, %rax\n";
            } else if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
                out << "  cvtsi2sd %rax, %xmm7\n";   // int → double
                out << "  movq %xmm7, %rax\n";
            }
        } else if (tgtType && tgtType->ttype == Type::FLOAT) {
            if (valType && valType->ttype == Type::DOUBLE) {
                out << "  movq %rax, %xmm7\n";
                out << "  cvtsd2ss %xmm7, %xmm7\n";  // double → float
                out << "  movd %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            } else if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
                out << "  cvtsi2ss %eax, %xmm7\n";   // int → float
                out << "  movd %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        }
    }

    switch (e->op) {
    case AssignOp::ASSIGN:
        break;
    }

    storeTarget(target);
}

// -----------------------------------------------------------
// visit(PrintfNode) — llamada a printf
// -----------------------------------------------------------
// Genera código para printf con formato y argumentos variables.
// Sigue la convención System V AMD64 para funciones variádicas:
//   - %rdi = formato (string)
//   - %rsi, %rdx, %rcx, %r8, %r9 = argumentos enteros
//   - %xmm0–%xmm7 = argumentos float/double
//   - %rax = número de registros xmm usados
//
//   Ej: printf("%d %f", x, y)
//       → formato en %rdi, x en %rsi, y en %xmm0, %rax = 1
void GenCodeVisitor::visit(PrintfNode *e) {
    // Seleccionar formato por defecto según el tipo del primer argumento
    string fmtLabel = "print_fmt";
    if (e->format == "%ld" && !e->args.empty()) {
        // Sin formato explícito: elegir según el tipo del primer argumento
        if (e->args[0]->resolvedType) {
            Type* t = e->args[0]->resolvedType;
            if (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE)
                fmtLabel = "print_fmt_float";
            else
                fmtLabel = "print_fmt";
        }
    } else if (e->format != "%ld") {
        auto it = stringLabels.find(e->format);
        int lbl;
        if (it == stringLabels.end()) {
            lbl = (int)stringLabels.size();
            stringLabels[e->format] = lbl;
            out << ".section .rodata\n";
            out << ".Lfmt" << lbl << ": .string \"" << e->format << "\"\n";
            out << ".text\n";
            fmtLabel = ".Lfmt" + to_string(lbl);
        } else {
            fmtLabel = ".Lfmt" + to_string(it->second);
        }
    }

    // Cargar argumentos en registros según convención System V AMD64 ABI
    const vector<string> intRegs = {"%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    const vector<string> xmmRegs = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"};

    for (size_t i = 0; i < e->args.size(); i++) {
        e->args[i]->accept(this);

        // Detectar si el argumento es float/double
        bool isFloat = false;
        Type* argType = nullptr;
        if (e->args[i]->resolvedType) {
            argType = e->args[i]->resolvedType;
            isFloat = (argType->ttype == Type::FLOAT || argType->ttype == Type::DOUBLE);
        }

        if (isFloat && i < xmmRegs.size()) {
            // printf %f espera double; convertir float→double si es necesario
            if (argType && argType->ttype == Type::FLOAT) {
                out << "  movd %eax, " << xmmRegs[i] << "\n";
                out << "  cvtss2sd " << xmmRegs[i] << ", " << xmmRegs[i] << "\n";
            } else {
                out << "  movq %rax, " << xmmRegs[i] << "\n";
            }
        } else if (i < intRegs.size()) {
            out << "  movq %rax, " << intRegs[i] << "\n";
        } else {
            out << "  pushq %rax\n";  // argumentos extra van al stack
        }
    }

    // Formato en %rdi
    out << "  leaq " << fmtLabel << "(%rip), %rdi\n";

    // %rax = número de registros xmm usados (para ABI variádica)
    int xmmCount = 0;
    for (size_t i = 0; i < e->args.size() && i < xmmRegs.size(); i++) {
        if (e->args[i]->resolvedType) {
            Type* t = e->args[i]->resolvedType;
            if (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE) {
                xmmCount++;
            }
        }
    }
    out << "  movq $" << xmmCount << ", %rax\n";

    out << "  call printf@PLT\n";

    // Limpiar argumentos extra del stack
    int regsUsed = min((int)e->args.size(), (int)max(intRegs.size(), xmmRegs.size()));
    if ((int)e->args.size() > regsUsed) {
        out << "  addq $" << ((e->args.size() - regsUsed) * 8) << ", %rsp\n";
    }

    out << "  movq $0, %rax\n";
}

// -----------------------------------------------------------
// visit(FcallNode) — llamada a función
// -----------------------------------------------------------
// Genera código para llamar a funciones:
//   - Push argumentos extra (>6) en orden inverso
//   - Cargar primeros 6 argumentos en registros
//   - call directo
//   - Limpiar stack si hay argumentos extra
//
//   Ej: f(1, 2, 3) → movq $1, %rdi; movq $2, %rsi; movq $3, %rdx; call f
void GenCodeVisitor::visit(FcallNode *e) {
    const vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    auto *id = dynamic_cast<IdentifierNode *>(e->callee);
    string fname = id ? id->name : "";

    int nArgs = (int)e->args.size();

    // Push argumentos extra (>6) en orden inverso
    for (int i = nArgs - 1; i >= 6; i--) {
        e->args[i]->accept(this);
        out << "  pushq %rax\n";
    }

    // Cargar primeros 6 argumentos en registros
    for (int i = 0; i < nArgs && i < 6; i++) {
        e->args[i]->accept(this);
        out << "  movq %rax, " << argRegs[i] << "\n";
    }

    out << "  call " << fname << "\n";

    // Limpiar argumentos extra del stack
    if (nArgs > 6) {
        out << "  addq $" << ((nArgs - 6) * 8) << ", %rsp\n";
    }
}

// -----------------------------------------------------------
// visit(IndexNode) — acceso a arreglo por índice
// -----------------------------------------------------------
// En modo l-value: guarda índices y binding en LVal.
// En modo valor: emite emitIndexedLoad.
//
//   Ej: a[i] (l-value) → LVal{Index, binding=&a, indices=[i]}
//       a[i] (valor)   → emitIndexedLoad(a, [i]) → carga en %rax
void GenCodeVisitor::visit(IndexNode *e) {
    if (lvalTarget) {
        // Recurse into base to capture its LVal, then append our index
        LVal objLVal;
        LVal* oldTarget = lvalTarget;
        lvalTarget = &objLVal;
        e->base->accept(this);
        lvalTarget = oldTarget;

        *lvalTarget = objLVal;
        lvalTarget->kind = LValKind::Index;
        lvalTarget->indices.push_back(e->index);
        return;
    }

    // --- Value path ---
    // Primero intentar collectIndices para arrays planos (a[i][j])
    if (dynamic_cast<IdentifierNode*>(e->base)) {
        vector<Exp*> indices;
        Exp* b = nullptr;
        collectIndices(e, indices, b);
        if (auto* id = dynamic_cast<IdentifierNode*>(b)) {
            if (e->resolvedType && e->resolvedType->ttype == Type::ARRAY) {
                emitIndexedAddress(id->binding, indices);
            } else {
                emitIndexedLoad(id->binding, indices);
            }
            return;
        }
    }

    // Path general: evaluar base recursivamente, luego aplicar índice
    e->base->accept(this); // retorna dirección para arrays, valor para escalares
    out << "  pushq %rax\n";
    e->index->accept(this);
    out << "  movq %rax, %rcx\n";

    // Element size = sizeof(inner type del array base)
    int elemSize = 4;
    if (e->resolvedType) elemSize = e->resolvedType->size();
    Type* baseType = e->base->resolvedType;
    if (baseType && baseType->ttype == Type::ARRAY) {
        ArrayType* at = static_cast<ArrayType*>(baseType);
        if (at->base) elemSize = at->base->size();
    }

    if (elemSize != 1) {
        out << "  movq $" << elemSize << ", %rax\n";
        out << "  imulq %rcx, %rax\n";
        out << "  movq %rax, %rcx\n";
    }
    out << "  popq %rax\n";
    out << "  addq %rcx, %rax\n";

    // Si el resultado NO es un array, cargar valor desde la dirección
    if (!(e->resolvedType && e->resolvedType->ttype == Type::ARRAY)) {
        int loadSize = 4;
        if (e->resolvedType) loadSize = e->resolvedType->size();
        if (loadSize == 1) out << "  movzbq (%rax), %rax\n";
        else if (loadSize == 4) out << "  movslq (%rax), %rax\n";
        else out << "  movq (%rax), %rax\n";
    }
}

// -----------------------------------------------------------
// visit(MemberAccessNode) — acceso a miembro de struct (.)
// -----------------------------------------------------------
// En modo l-value: guarda info en LVal para storeTarget.
// En modo valor: leaBinding + load desde offset del miembro.
//
//   Ej: p.x → lea p; movslq offset_x(%rax), %rax
void GenCodeVisitor::visit(MemberAccessNode *e) {
    if (lvalTarget) {
        // First capture lval for object
        LVal objLVal;
        LVal *oldTarget = lvalTarget;
        lvalTarget = &objLVal;
        e->object->accept(this);
        lvalTarget = oldTarget;

        // Merge into target lval (preserve isArrow from inner LVal)
        *lvalTarget = objLVal;
        lvalTarget->kind = LValKind::Member; // <-- Critical! Set kind to Member!
        lvalTarget->members.push_back(e->member);
        lvalTarget->memberArrow.push_back(false);
        if (lvalTarget->structName.empty()) {
            Type* objType = e->object->resolvedType;
            if (objType && objType->ttype == Type::STRUCT) {
                lvalTarget->structName = ((StructType*)objType)->name;
            } else if (objLVal.binding && objLVal.binding->resolvedType && 
                       objLVal.binding->resolvedType->ttype == Type::STRUCT) {
                lvalTarget->structName = ((StructType*)objLVal.binding->resolvedType)->name;
            }
        }
        return;
    }
    // Get the ADDRESS of the object (e->object), leave it in %rax, and get currentStructName
    Type* objType = e->object->resolvedType;
    string currentStructName;
    
    // Use captureLVal to get address of e->object
    LVal objLVal = captureLVal(e->object);
    
    // Now compute the address of the object from objLVal
    if (objLVal.kind == LValKind::Id) {
        leaBinding(objLVal.binding);
        // Get struct name
        if (objLVal.binding && objLVal.binding->resolvedType && 
            objLVal.binding->resolvedType->ttype == Type::STRUCT) {
            currentStructName = ((StructType*)objLVal.binding->resolvedType)->name;
        }
    } else if (objLVal.kind == LValKind::Index) {
        emitIndexedAddress(objLVal.binding, objLVal.indices);
    } else if (objLVal.kind == LValKind::Member) {
        // Need to compute address step by step, applying existing member offsets
        if (objLVal.binding) {
            if (objLVal.isArrow) {
                loadBinding(objLVal.binding); // p->m: load pointer value
            } else {
                leaBinding(objLVal.binding);   // s.m: get address of struct
            }
        }
        // Apply existing members and track currentStructName
        currentStructName = objLVal.structName;
        for (size_t i = 0; i < objLVal.members.size(); ++i) {
            const string& m = objLVal.members[i];
            // For nested arrow accesses, load pointer value before adding offset
            if (i > 0 && i < objLVal.memberArrow.size() && objLVal.memberArrow[i]) {
                out << "  movq (%rax), %rax\n";
            }
            int off = structFieldOffset[baseStructName(currentStructName)][m];
            out << "  addq $" << off << ", %rax\n";
            // Update to type of this member
            if (structMemberTypes.count(baseStructName(currentStructName)) && 
                structMemberTypes[baseStructName(currentStructName)].count(m)) {
                Type* mt = structMemberTypes[baseStructName(currentStructName)][m];
                if (mt->ttype == Type::STRUCT) {
                    currentStructName = ((StructType*)mt)->name;
                } else if (mt->ttype == Type::POINTER) {
                    PointerType* pt = (PointerType*)mt;
                    if (pt->base && pt->base->ttype == Type::STRUCT) {
                        currentStructName = ((StructType*)pt->base)->name;
                    }
                }
            }
        }
    } else if (objLVal.kind == LValKind::Deref) {
        // For dereference, we already have the address
    }
    
    // Now, if objLVal.structName is still empty, try to get it from objType
    if (currentStructName.empty() && objType) {
        if (objType->ttype == Type::STRUCT) {
            currentStructName = ((StructType*)objType)->name;
        }
    }

    // Add offset for final member
    int off = structFieldOffset[baseStructName(currentStructName)][e->member];
    // Si el miembro es un array, dejar dirección en %rax (no cargar valor)
    if (structMemberTypes.count(baseStructName(currentStructName)) &&
        structMemberTypes[baseStructName(currentStructName)].count(e->member)) {
        Type* mt = structMemberTypes[baseStructName(currentStructName)][e->member];
        if (mt && mt->ttype == Type::ARRAY) {
            out << "  addq $" << off << ", %rax\n";
            return;
        }
    }
    int size = structMemberSizes[baseStructName(currentStructName)][e->member];
    out << "  " << loadInstr(size) << " " << off << "(%rax), %rax\n";
}

// -----------------------------------------------------------
// visit(ArrowAccessNode) — acceso a miembro vía puntero (->)
// -----------------------------------------------------------
// Similar a MemberAccessNode pero carga primero el puntero.
//
//   Ej: p->x → loadBinding(p); movslq offset_x(%rax), %rax
void GenCodeVisitor::visit(ArrowAccessNode *e) {
    if (lvalTarget) {
        // First capture lval for pointer
        LVal ptrLVal;
        LVal *oldTarget = lvalTarget;
        lvalTarget = &ptrLVal;
        e->pointer->accept(this);
        lvalTarget = oldTarget;

        // Merge into target lval
        *lvalTarget = ptrLVal;
        lvalTarget->kind = LValKind::Member; // Critical! Set kind to Member!
        lvalTarget->members.push_back(e->member);
        lvalTarget->memberArrow.push_back(true);
        lvalTarget->isArrow = true;
        if (lvalTarget->structName.empty()) {
            Type* ptrType = e->pointer->resolvedType;
            if (ptrType && ptrType->ttype == Type::POINTER) {
                PointerType* pt = (PointerType*)ptrType;
                if (pt->base && pt->base->ttype == Type::STRUCT) {
                    lvalTarget->structName = ((StructType*)pt->base)->name;
                }
            }
        }
        return;
    }
    // Load pointer value (address of struct), then add member offset
    e->pointer->accept(this); // %rax has pointer value (struct address)
    Type* ptrType = e->pointer->resolvedType;
    string structName;
    if (ptrType && ptrType->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)ptrType;
        if (pt->base && pt->base->ttype == Type::STRUCT) {
            structName = ((StructType*)pt->base)->name;
        }
    }
    int off = structFieldOffset[baseStructName(structName)][e->member];
    // Si el miembro es un array, dejar dirección en %rax (no cargar valor)
    Type* mt = structMemberTypes[baseStructName(structName)][e->member];
    if (mt && mt->ttype == Type::ARRAY) {
        out << "  addq $" << off << ", %rax\n";
    } else {
        int size = structMemberSizes[baseStructName(structName)][e->member];
        out << "  " << loadInstr(size) << " " << off << "(%rax), %rax\n";
    }
}

// -----------------------------------------------------------
// visit(MallocNode) — llamada a malloc
// -----------------------------------------------------------
// Evalúa el tamaño, lo pasa en %rdi, llama a malloc.
// El puntero devuelto queda en %rax.
//
//   Ej: malloc(8) → movq $8, %rdi; call malloc@PLT
void GenCodeVisitor::visit(MallocNode *e) {
    e->size->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call malloc@PLT\n";
}

// -----------------------------------------------------------
// visit(SizeOfNode) — sizeof
// -----------------------------------------------------------
// Genera el tamaño en bytes de un tipo como constante en %rax.
//   void/char/bool → 1
//   int/float      → 4
//   double         → 8
//   puntero        → 8
//   struct         → n (calculado como campos * 8)
void GenCodeVisitor::visit(SizeOfNode *e) {
    if (auto *pt = dynamic_cast<PrimitiveTypeNode *>(e->target_type)) {
        switch (pt->prim) {
        case PrimitiveTypeNode::VOID:
        case PrimitiveTypeNode::CHAR:
        case PrimitiveTypeNode::BOOL: out << "  movq $1, %rax\n"; return;
        case PrimitiveTypeNode::INT:
        case PrimitiveTypeNode::FLOAT: out << "  movq $4, %rax\n"; return;
        case PrimitiveTypeNode::DOUBLE:
        case PrimitiveTypeNode::LONG: out << "  movq $8, %rax\n"; return;
        }
    }
    if (dynamic_cast<PointerTypeNode *>(e->target_type))
        out << "  movq $8, %rax\n";
    else if (auto *st = dynamic_cast<StructTypeNode *>(e->target_type)) {
        // Aproximación: nº de campos × 8. No usa memberSizes reales del struct.
        auto it = structFieldCount.find(st->name);
        if (it != structFieldCount.end()) {
            out << "  movq $" << (it->second * 8) << ", %rax\n";
        } else {
            out << "  movq $0, %rax\n";
        }
    }
    else
        out << "  movq $0, %rax\n";
}

// -----------------------------------------------------------
// Nodos de tipo puro del AST: no generan instrucciones en runtime.
// Aparecen en sizeof(T), declaraciones, etc.; el codegen los ignora al visitarlos solos.
void GenCodeVisitor::visit(PrimitiveTypeNode *) {}
void GenCodeVisitor::visit(PointerTypeNode *) {}
void GenCodeVisitor::visit(StructTypeNode *) {}


// ============================================================
//  Statements
// ============================================================

// -----------------------------------------------------------
// visit(Body) — bloque { stmt; stmt; ... }
// -----------------------------------------------------------
// Secuencia simple: cada statement genera código en orden.
void GenCodeVisitor::visit(Body *s) {
    for (auto st : s->stmts)
        st->accept(this);
}

// Expresión usada como statement (p. ej. llamada a función); descarta el valor en %rax.
void GenCodeVisitor::visit(ExprStmtNode *s) {
    if (s->expr)
        s->expr->accept(this);
}

// -----------------------------------------------------------
// visit(IfStmt) — if/else
// -----------------------------------------------------------
// Genera: condition → cmp $0; je else_N; then; jmp endif_N; else_N: else; endif_N:
//
//   Ej: if (x > 0) { a = 1; } else { a = 2; }
//       → ...cmp $0, %rax; je else_0; ... jmp endif_0; else_0: ... endif_0:
void GenCodeVisitor::visit(IfStmt *s) {
    if (s->condition->isConstant) {
        if (s->condition->constantValue != 0)
            s->then_branch->accept(this);
        else if (s->else_branch)
            s->else_branch->accept(this);
        return;
    }
    int lbl = labelcont++;
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  je else_" << lbl << "\n";
    s->then_branch->accept(this);
    out << "  jmp endif_" << lbl << "\n";
    out << "else_" << lbl << ":\n";
    if (s->else_branch)
        s->else_branch->accept(this);
    out << "endif_" << lbl << ":\n";
}

// -----------------------------------------------------------
// visit(WhileStmt) — while loop
// -----------------------------------------------------------
// Genera: while_N: condition → cmp $0; je endwhile_N; body; jmp while_N; endwhile_N:
// Guarda currentBreakLabel y currentContinueLabel para break/continue.
//
//   Ej: while (i < 10) { i++; }
void GenCodeVisitor::visit(WhileStmt *s) {
    if (s->condition->isConstant) {
        if (s->condition->constantValue == 0)
            return;
        int lbl = labelcont++;
        string oldBreak = currentBreakLabel;
        string oldContinue = currentContinueLabel;
        currentBreakLabel = "endwhile_" + to_string(lbl);
        currentContinueLabel = "while_" + to_string(lbl);
        out << "while_" << lbl << ":\n";
        s->body->accept(this);
        out << "  jmp while_" << lbl << "\n";
        out << "endwhile_" << lbl << ":\n";
        currentBreakLabel = oldBreak;
        currentContinueLabel = oldContinue;
        return;
    }
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    string oldContinue = currentContinueLabel;
    currentBreakLabel = "endwhile_" + to_string(lbl);
    currentContinueLabel = "while_" + to_string(lbl);

    out << "while_" << lbl << ":\n";
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  je endwhile_" << lbl << "\n";
    s->body->accept(this);
    out << "  jmp while_" << lbl << "\n";
    out << "endwhile_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
    currentContinueLabel = oldContinue;
}

// -----------------------------------------------------------
// visit(DoWhileStmt) — do-while loop
// -----------------------------------------------------------
// Genera: dowhile_N: body; docond_N: condition → cmp; jne dowhile_N; endwhile_N:
void GenCodeVisitor::visit(DoWhileStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    string oldContinue = currentContinueLabel;
    currentBreakLabel = "endwhile_" + to_string(lbl);
    currentContinueLabel = "docond_" + to_string(lbl);

    out << "dowhile_" << lbl << ":\n";
    s->body->accept(this);
    if (s->condition->isConstant) {
        if (s->condition->constantValue != 0) {
            out << "  jmp dowhile_" << lbl << "\n";
        }
        out << "endwhile_" << lbl << ":\n";
        currentBreakLabel = oldBreak;
        currentContinueLabel = oldContinue;
        return;
    }
    out << "docond_" << lbl << ":\n";
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  jne dowhile_" << lbl << "\n";
    out << "endwhile_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
    currentContinueLabel = oldContinue;
}

// -----------------------------------------------------------
// visit(ForStmt) — for loop
// -----------------------------------------------------------
// Genera: init; for_N: condition → cmp; je endfor_N; body; forinc_N: increment; jmp for_N; endfor_N:
void GenCodeVisitor::visit(ForStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    string oldContinue = currentContinueLabel;
    currentBreakLabel = "endfor_" + to_string(lbl);
    currentContinueLabel = "forinc_" + to_string(lbl);

    if (s->init)
        s->init->accept(this);

    if (s->condition && s->condition->isConstant) {
        if (s->condition->constantValue == 0) {
            out << "endfor_" << lbl << ":\n";
            currentBreakLabel = oldBreak;
            currentContinueLabel = oldContinue;
            return;
        }
        out << "for_" << lbl << ":\n";
        s->body->accept(this);
        out << "forinc_" << lbl << ":\n";
        if (s->increment)
            s->increment->accept(this);
        out << "  jmp for_" << lbl << "\n";
        out << "endfor_" << lbl << ":\n";
        currentBreakLabel = oldBreak;
        currentContinueLabel = oldContinue;
        return;
    }

    out << "for_" << lbl << ":\n";
    if (s->condition) {
        s->condition->accept(this);
        out << "  cmpq $0, %rax\n";
        out << "  je endfor_" << lbl << "\n";
    }
    s->body->accept(this);
    out << "forinc_" << lbl << ":\n";
    if (s->increment)
        s->increment->accept(this);
    out << "  jmp for_" << lbl << "\n";
    out << "endfor_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
    currentContinueLabel = oldContinue;
}

// -----------------------------------------------------------
// visit(SwitchStmt) — switch
// -----------------------------------------------------------
// Genera: eval expr → %r10; cmp con cada case; je case_N; jmp default_N;
//         case_0: ... case_1: ... default: ... endswitch_N:
//
// No tiene fall-through (cada case es independiente).
void GenCodeVisitor::visit(SwitchStmt *s) {
    int lbl = labelcont++;
    s->expr->accept(this);
    out << "  movq %rax, %r10\n";

    // Generar comparaciones para cada case (solo literales enteros por ahora).
    int caseIdx = 0;
    for (auto cc : s->cases) {
        if (auto *lit = dynamic_cast<IntegerLiteralNode *>(cc->value))
            out << "  movq $" << lit->value << ", %rax\n";
        else
            out << "  movq $0, %rax\n";  // case con char u otra forma: sin soporte completo
        out << "  cmpq %rax, %r10\n";
        out << "  je case_" << lbl << "_" << caseIdx << "\n";
        caseIdx++;
    }

    out << "  jmp default_" << lbl << "\n";

    string oldBreak = currentBreakLabel;
    currentBreakLabel = "endswitch_" + to_string(lbl);

    // Generar código para cada case
    caseIdx = 0;
    for (auto cc : s->cases) {
        out << "case_" << lbl << "_" << caseIdx << ":\n";
        for (auto st : cc->body) st->accept(this);
        caseIdx++;
    }

    // Código para default
    out << "default_" << lbl << ":\n";
    for (auto st : s->default_body) st->accept(this);

    currentBreakLabel = oldBreak;
    out << "endswitch_" << lbl << ":\n";
}

// CaseClause no se visita directamente: visit(SwitchStmt) recorre cc->body.
void GenCodeVisitor::visit(CaseClause *) {}

// -----------------------------------------------------------
// visit(BreakStmt) — break
// -----------------------------------------------------------
// Salta al label de final del loop/switch actual.
void GenCodeVisitor::visit(BreakStmt *) {
    if (currentBreakLabel.empty()) {
        cerr << "Error: break fuera de ciclo o switch\n";
        exit(1);
    }
    out << "  jmp " << currentBreakLabel << "\n";
}

// -----------------------------------------------------------
// visit(ContinueStmt) — continue
// -----------------------------------------------------------
// Salta al label de continuación del loop actual.
void GenCodeVisitor::visit(ContinueStmt *) {
    if (currentContinueLabel.empty()) {
        cerr << "Error: continue fuera de ciclo\n";
        exit(1);
    }
    out << "  jmp " << currentContinueLabel << "\n";
}

// -----------------------------------------------------------
// visit(ReturnStmt) — return
// -----------------------------------------------------------
// Evalúa la expresión (si existe) y salta al epílogo de la función.
void GenCodeVisitor::visit(ReturnStmt *s) {
    if (s->expr)
        s->expr->accept(this);
    else
        out << "  movq $0, %rax\n";
    string target = returnLabel.empty() ? ".end_" + funcName : returnLabel;
    out << "  jmp " << target << "\n";
}

// -----------------------------------------------------------
// visit(FreeStmt) — free
// -----------------------------------------------------------
// Evalúa el puntero y llama a free.
//
//   Ej: free(ptr) → movq ptr(%rip), %rdi; call free@PLT
void GenCodeVisitor::visit(FreeStmt *s) {
    s->expr->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call free@PLT\n";
}

// ============================================================
//  Declarations
// ============================================================

// -----------------------------------------------------------
// visit(VarDecl) — declaración de variable
// -----------------------------------------------------------
// Variables globales: se marcan en memoriaGlobal (ya están en .data).
// Variables locales: se registran en memoria, se evalúa inicializador.
//
//   Ej: int x = 5; → bind_var_decl(x); eval(5); storeBinding(x)
void GenCodeVisitor::visit(VarDecl *d) {
    if (!inFunction) {
        memoriaGlobal[d->name] = true;
        return;
    }

    bind_var_decl(d);

    if (d->initializer) {
        d->initializer->accept(this);
        // Promover/convertir según el tipo del destino
        if (d->resolvedType && d->resolvedType->ttype == Type::DOUBLE) {
            Type* initType = d->initializer->resolvedType;
            if (initType && initType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm7\n";
                out << "  cvtss2sd %xmm7, %xmm7\n";  // float → double
                out << "  movq %xmm7, %rax\n";
            } else if (!initType || initType->ttype == Type::INT || initType->ttype == Type::CHAR) {
                out << "  cvtsi2sd %rax, %xmm7\n";   // int → double
                out << "  movq %xmm7, %rax\n";
            }
        } else if (d->resolvedType && d->resolvedType->ttype == Type::FLOAT) {
            Type* initType = d->initializer->resolvedType;
            if (initType && initType->ttype == Type::DOUBLE) {
                out << "  movq %rax, %xmm7\n";
                out << "  cvtsd2ss %xmm7, %xmm7\n";  // double → float
                out << "  movd %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        }
        storeBinding(d);
    }
    if (!d->init_list.empty()) {
        int elemSize = array_elem_size(d);
        Type* elemType = d->resolvedType;
        while (elemType && elemType->ttype == Type::ARRAY)
            elemType = ((ArrayType*)elemType)->base;
        for (size_t i = 0; i < d->init_list.size(); i++) {
            d->init_list[i]->accept(this);
            int off = d->offset + (int)i * elemSize;
            if (isFloatSemanticType(elemType)) {
                if (elemType->ttype == Type::DOUBLE) {
                    out << "  movq %rax, %xmm7\n";
                    out << "  movsd %xmm7, " << off << "(%rbp)\n";
                } else {
                    out << "  movd %eax, %xmm7\n";
                    out << "  movss %xmm7, " << off << "(%rbp)\n";
                }
            } else {
                int size = elemSize;
                string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
                out << "  " << storeInstr(size) << " " << reg << ", " << off << "(%rbp)\n";
            }
        }
    }
}

// -----------------------------------------------------------
// visit(FunDecl) — declaración de función
// -----------------------------------------------------------
// Genera el código completo de una función:
//   1. .globl nombre (para enlace externo)
//   2. Prólogo: push %rbp, mov %rsp %rbp, sub frameSize %rsp
//   3. Copiar parámetros de registros al stack
//   4. Generar body
//   5. Epílogo: leave, ret
//
//   Ej: int suma(int a, int b) { return a + b; }
//       → pushq %rbp; movq %rsp, %rbp; subq $16, %rsp
//         movl %edi, -8(%rbp); movl %esi, -12(%rbp)
//         ... cuerpo ...
//         .end_suma: leave; ret
void GenCodeVisitor::visit(FunDecl *d) {
    inFunction = true;
    memoria.clear();
    funcName = d->name;
    returnLabel = ".end_" + d->name;

    // Usar frameSize calculado por TypeChecker (con bin packing)
    int frameSize = d->frameSize;

    out << "\n.globl " << d->name << "\n";
    out << d->name << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";

    // Copiar parámetros de registros al stack
    const vector<string> argRegs32 = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    const vector<string> argRegs64 = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    for (size_t i = 0; i < d->params.size() && i < 6; i++) {
        bind_var_decl(d->params[i]);

        int size = d->params[i]->memSize;
        if (size == 4)
            out << "  movl " << argRegs32[i] << ", " << d->params[i]->offset << "(%rbp)\n";
        else if (size == 1)
            out << "  movb " << argRegs32[i] << ", " << d->params[i]->offset << "(%rbp)\n";
        else
            out << "  movq " << argRegs64[i] << ", " << d->params[i]->offset << "(%rbp)\n";
    }

    // Generar código del body
    d->body->accept(this);

    // Epílogo
    out << ".end_" << d->name << ":\n";
    out << "  leave\n";
    out << "  ret\n";
    inFunction = false;
}

// -----------------------------------------------------------
// visit(StructDecl) — declaración de struct
// -----------------------------------------------------------
// Copia los offsets y tamaños de miembros calculados por
// TypeChecker a los mapas de GenCodeVisitor.
//
//   Ej: struct Punto { int x; float y; };
//       → structFieldOffset["Punto"]["x"] = 0
//         structFieldOffset["Punto"]["y"] = 4
//         structMemberSizes["Punto"]["x"] = 4
//         structMemberSizes["Punto"]["y"] = 4
//         structFieldCount["Punto"] = 2
void GenCodeVisitor::visit(StructDecl *d) {
    for (auto& pair : d->memberOffsets) {
        structFieldOffset[d->name][pair.first] = pair.second;
    }
    for (auto& pair : d->memberSizes) {
        structMemberSizes[d->name][pair.first] = pair.second;
    }
    for (VarDecl* m : d->members) {
        structMemberTypes[d->name][m->name] = m->resolvedType;
    }
    structFieldCount[d->name] = (int)d->members.size();
}

// Redirige al punto de entrada real (generate). Program::accept en el AST
// termina aquí en lugar de recorrer hijos manualmente.
void GenCodeVisitor::visit(Program *p) {
    generate(p);
}

// ============================================================
// computeAddress — calcular dirección de un l-value (dejar en %rax)
// ============================================================
// Se invoca vía node->computeAddress(this) cuando hace falta la dirección
// (operador &, o antes de storeTarget en algunos casos), no el valor.
// Difiere de visit() del mismo nodo, que normalmente carga el valor.

// &(*p) o dirección para asignación a *p: %rax = valor del puntero.
void GenCodeVisitor::computeAddress(UnaryOpNode *e) {
    // *p: la dirección es el valor del puntero mismo
    e->operand->accept(this);
}

// &variable → leaq offset(%rbp), %rax
void GenCodeVisitor::computeAddress(IdentifierNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;
        lvalTarget->name = e->name;
        lvalTarget->binding = e->binding;
        return;
    }
    leaBinding(e->binding);
}

// &arr[i][j] → emitIndexedAddress (índice lineal × elemSize + base).
void GenCodeVisitor::computeAddress(IndexNode *e) {
    vector<Exp*> indices;
    Exp* b = nullptr;
    collectIndices(e, indices, b);

    VarDecl* arrBinding = nullptr;
    if (auto *id = dynamic_cast<IdentifierNode *>(b))
        arrBinding = id->binding;

    emitIndexedAddress(arrBinding, indices);
}

// &s.miembro → dirección de s + offset del campo (acceso por valor en stack).
void GenCodeVisitor::computeAddress(MemberAccessNode *e) {
    if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
        leaBinding(id->binding);
        string structName = id->name;
        if (id->resolvedType && id->resolvedType->ttype == Type::STRUCT) {
            structName = ((StructType*)id->resolvedType)->name;
        }
        out << "  addq $" << structFieldOffset[baseStructName(structName)][e->member] << ", %rax\n";
    }
}

// &p->miembro → carga p en %rax, luego suma offset del campo.
void GenCodeVisitor::computeAddress(ArrowAccessNode *e) {
    if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer)) {
        loadBinding(id->binding);
        string structName = id->name;
        if (id->resolvedType && id->resolvedType->ttype == Type::POINTER) {
            PointerType* pt = (PointerType*)id->resolvedType;
            if (pt->base && pt->base->ttype == Type::STRUCT) {
                structName = ((StructType*)pt->base)->name;
            }
        }
        out << "  addq $" << structFieldOffset[baseStructName(structName)][e->member] << ", %rax\n";
    }
}
