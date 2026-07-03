#include "visitor.h"
#include "ast.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

using namespace std;

// ============================================================
// GenCodeVisitor — Generación de código x86-64 (sintaxis AT&T)
// ============================================================
// Backend del compilador C--: recorre el AST anotado por
// TypeChecker y emite ensamblador x86-64 para Linux (ABI System V).
//
// Convenciones:
//   - Args enteros/punteros: %rdi, %rsi, %rdx, %rcx, %r8, %r9
//   - Args float/double:     %xmm0–%xmm7
//   - Retorno entero/puntero: %rax
//   - Retorno float/double:  %xmm0 (a veces reempaquetado en %rax)
//   - Stack frame: %rbp base, offsets negativos hacia abajo
//   - Toda expresión deja su resultado en %rax
//   - L-values usan struct LVal + lvalTarget
//
// Dependencias del TypeChecker:
//   - VarDecl: offset, memSize, resolvedType
//   - IdNode: binding → VarDecl
//   - Expresiones: resolvedType, isConstant, constantValue
//   - FunDecl: frameSize
//   - StructDecl: memberOffsets, memberSizes
// ============================================================

// ============================================================
// Stubs computeAddress — despacho desde nodos l-value
// ============================================================
// Los nodos l-value (Id, Index, Member, Arrow, UnaryOp *)
// implementan computeAddress() además de accept().

// Retorna el nombre base del struct (sin transformar).
static string baseStructName(const string& structName) {
    return structName;
}

void UnaryOpNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void IdNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void IndexNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void MemberAccessNode::computeAddress(Visitor* v) { v->computeAddress(this); }
void ArrowAccessNode::computeAddress(Visitor* v) { v->computeAddress(this); }

// ============================================================
// Helpers: detección de tipos float/double
// ============================================================

// true si t es float o double. Usado para elegir instrucciones SSE.
static bool isFloatSemanticType(Type* t) {
    return t && (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE);
}

// ============================================================
// generate — punto de entrada de generación de código
// ============================================================
// Genera el archivo ensamblador completo:
//   1. Inicializa estructuras de datos.
//   2. Procesa structs.
//   3. Escribe sección .data con globales y strings.
//   4. Escribe sección .text con funciones.
//   5. Emite función auxiliar `potencia` si se usa ^.
//   6. Escribe nota GNU-stack.
void GenCodeVisitor::generate(Program *p) {
    stringLabels.clear();
    usedPow = false;
    for (auto g : p->globals)
        memoriaGlobal[g->name] = true;

    // Procesar structs primero para inicializar offsets.
    for (auto s : p->structs)
        s->accept(this);

    // Sección .data
    out << ".data\n";

    for (auto g : p->globals) {
        if (g->initializer && g->initializer->isConstant) {
            if (g->resolvedType && isFloatSemanticType(g->resolvedType)) {
                if (g->resolvedType->ttype == Type::DOUBLE) {
                    union { double d; unsigned long long i; } dc;
                    dc.d = g->initializer->constantValue;
                    out << g->name << ": .quad 0x" << hex << dc.i << dec << "\n";
                } else {
                    union { float f; unsigned int i; } fc;
                    fc.f = (float)g->initializer->constantValue;
                    out << g->name << ": .long 0x" << hex << fc.i << dec << "\n";
                }
            } else {
                long long val = (long long)g->initializer->constantValue;
                if (g->resolvedType && g->resolvedType->ttype == Type::CHAR) {
                    out << g->name << ": .byte " << val << "\n";
                } else if (g->resolvedType && g->resolvedType->ttype == Type::INT) {
                    out << g->name << ": .long " << val << "\n";
                } else {
                    out << g->name << ": .quad " << val << "\n";
                }
            }
        } else {
            out << g->name << ": .quad 0\n";
        }
    }

    // Sección .text
    out << "\n.text\n";

    for (auto f : p->functions)
        f->accept(this);

    // Función auxiliar: potencia(base^exp)
    if (usedPow) {
        out << "\npotencia:\n";
        out << "  pushq %rbp\n";
        out << "  movq %rsp, %rbp\n";
        out << "  cmpq $0, %rsi\n";
        out << "  jne .pot_nz\n";
        out << "  movq $1, %rax\n";
        out << "  jmp .pot_end\n";
        out << ".pot_nz:\n";
        out << "  cmpq $1, %rsi\n";
        out << "  jne .pot_rec\n";
        out << "  movq %rdi, %rax\n";
        out << "  jmp .pot_end\n";
        out << ".pot_rec:\n";
        out << "  pushq %rbx\n";
        out << "  movq %rdi, %rbx\n";
        out << "  testq $1, %rsi\n";
        out << "  jz .pot_even\n";
        // exponente impar
        out << "  imulq %rdi, %rdi\n";
        out << "  sarq $1, %rsi\n";
        out << "  call potencia\n";
        out << "  imulq %rbx, %rax\n";
        out << "  popq %rbx\n";
        out << "  jmp .pot_end\n";
        // exponente par
        out << ".pot_even:\n";
        out << "  imulq %rdi, %rdi\n";
        out << "  sarq $1, %rsi\n";
        out << "  call potencia\n";
        out << "  popq %rbx\n";
        out << ".pot_end:\n";
        out << "  leave\n";
        out << "  ret\n";
    }

    out << "\n.section .note.GNU-stack,\"\",@progbits\n";
}

// ============================================================
// Helpers: sufijos e instrucciones según tamaño
// ============================================================

// Retorna sufijo AT&T según tamaño: 1→b, 4→l, 8→q.
string GenCodeVisitor::instrSuffix(int size) {
    switch (size) {
        case 1: return "b";
        case 4: return "l";
        case 8: return "q";
        default: return "q";
    }
}

// Retorna instrucción de carga con extensión según tamaño y signo.
string GenCodeVisitor::loadInstr(int size, bool isUnsigned) {
    switch (size) {
        case 1: return isUnsigned ? "movzbq" : "movsbq";
        case 4: return "movslq";
        case 8: return "movq";
        default: return "movq";
    }
}

// Retorna instrucción de almacenamiento según tamaño.
string GenCodeVisitor::storeInstr(int size) {
    switch (size) {
        case 1: return "movb";
        case 4: return "movl";
        case 8: return "movq";
        default: return "movq";
    }
}

// ============================================================
// bindingMem — representación de memoria de una variable
// ============================================================
// Retorna string AT&T con la dirección de la variable:
//   Global → "nombre(%rip)"
//   Local  → "offset(%rbp)"
string GenCodeVisitor::bindingMem(VarDecl* vd) const {
    if (!vd)
        throw runtime_error("Variable sin binding en codegen");
    if (memoriaGlobal.count(vd->name))
        return vd->name + "(%rip)";
    return to_string(vd->offset) + "(%rbp)";
}

// ============================================================
// bind_var_decl — registra offset de variable local
// ============================================================
// Guarda el offset en `memoria` para accesos por nombre.
void GenCodeVisitor::bind_var_decl(VarDecl* vd) {
    if (!vd) return;
    memoria[vd->name] = vd->offset;
}

// ============================================================
// array_elem_size — tamaño del elemento base de un arreglo
// ============================================================
// Extrae el tamaño del tipo base tras desanidar ArrayTypes.
// Para punteros, usa el tipo apuntado.
int GenCodeVisitor::array_elem_size(VarDecl* vd) const {
    if (!vd || !vd->resolvedType) return 8;
    Type* t = vd->resolvedType;
    if (t->ttype == Type::ARRAY) {
        Type* base = t;
        while (base && base->ttype == Type::ARRAY)
            base = ((ArrayType*)base)->base;
        if (base) return base->size();
    }
    if (t->ttype == Type::POINTER && ((PointerType*)t)->base)
        return ((PointerType*)t)->base->size();
    return 8;
}

// Extrae dimensiones desde un ArrayType anidado.
static vector<int> arrayDimsForType(Type* t) {
    vector<int> dims;
    while (t && t->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)t;
        if (at->length > 0) dims.push_back(at->length);
        t = at->base;
    }
    return dims;
}

// Extrae dimensiones desde el tipo resuelto de una VarDecl.
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
// collectIndices — recolecta índices anidados de arr[i][j][k]
// ============================================================
// Dada una cadena de IndexNodes, extrae los índices y la base.
//   Ej: a[i][j] → indices=[i,j], base=a
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
// loadBinding / storeBinding / leaBinding
// ============================================================

// Carga el valor de una variable en %rax. Maneja enteros y floats.
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
    out << "  " << loadInstr(size, type && type->isUnsigned) << " " << bindingMem(vd) << ", %rax\n";
}

// Almacena el valor en %rax en la variable. Maneja enteros y floats.
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
    if (type && type->ttype == Type::BOOL) {
        out << "  testq %rax, %rax\n";
        out << "  setne %al\n";
        out << "  movzbq %al, %rax\n";
    }
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
    out << "  " << storeInstr(size) << " " << reg << ", " << bindingMem(vd) << "\n";
}

// Carga la dirección efectiva de la variable en %rax.
void GenCodeVisitor::leaBinding(VarDecl* vd) {
    if (!vd) return;
    out << "  leaq " << bindingMem(vd) << ", %rax\n";
}

// ============================================================
// emitIndexedAddress — calcula dirección de a[i][j] en %rax
// ============================================================
// Para arreglos multidimensionales calcula índice lineal y lo
// multiplica por elemSize. Para 1D usa direccionamiento indexado.
void GenCodeVisitor::emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);

    if (dims.size() > 1) {
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
        else {
            out << "  movq $" << elemSize << ", %r11\n";
            out << "  imulq %r11, %r10\n";
            out << "  addq %r10, %rax\n";
        }
        return;
    }

    // Caso 1D
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    if (elemSize == 1) {
        out << "  addq %rdi, %rax\n";
    } else if (elemSize == 2) {
        out << "  leaq (%rax,%rdi,2), %rax\n";
    } else if (elemSize == 4) {
        out << "  leaq (%rax,%rdi,4), %rax\n";
    } else {
        out << "  movq $" << elemSize << ", %rcx\n";
        out << "  imulq %rcx, %rdi\n";
        out << "  addq %rdi, %rax\n";
    }
}

// ============================================================
// emitIndexedLoad — carga a[i][j] en %rax
// ============================================================
// Similar a emitIndexedAddress pero carga el valor.
void GenCodeVisitor::emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);
    Type* elemType = vd->resolvedType;
    while (elemType && elemType->ttype == Type::ARRAY)
        elemType = ((ArrayType*)elemType)->base;
    bool elemUnsigned = elemType && elemType->isUnsigned;

    if (dims.size() > 1) {
        emitIndexedAddress(vd, indices);
        if (indices.size() < dims.size()) return; // resultado es array → dejar dirección
        if (elemSize == 1) out << "  " << loadInstr(1, elemUnsigned) << " (%rax), %rax\n";
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

    if (elemSize == 1) {
        out << "  " << loadInstr(1, elemUnsigned) << " (%rax,%rdi), %rax\n";
    } else if (elemSize == 2) {
        out << "  movslq (%rax,%rdi,2), %rax\n";
    } else if (elemSize == 4) {
        out << "  movslq (%rax,%rdi,4), %rax\n";
    } else {
        out << "  movq $" << elemSize << ", %rcx\n";
        out << "  imulq %rcx, %rdi\n";
        out << "  addq %rdi, %rax\n";
        out << "  " << loadInstr(elemSize, elemUnsigned) << " (%rax), %rax\n";
    }
}

// ============================================================
// emitIndexedStore — guarda valueReg en a[i][j]
// ============================================================
// Similar a emitIndexedLoad pero almacena el valor recibido.
void GenCodeVisitor::emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices,
                                      const string& valueReg) {
    if (!vd || indices.empty()) return;
    // Normalizar a 0/1 si el elemento es bool
    Type* elemType = vd->resolvedType;
    while (elemType && elemType->ttype == Type::ARRAY)
        elemType = ((ArrayType*)elemType)->base;
    bool isBoolStore = elemType && elemType->ttype == Type::BOOL;
    if (isBoolStore) {
        out << "  testq %rax, %rax\n";
        out << "  setne %al\n";
        out << "  movzbq %al, %rax\n";
    }

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

    if (elemSize == 1) {
        out << "  movb %cl, (%rax,%rdi)\n";
    } else if (elemSize == 2) {
        out << "  movw %cx, (%rax,%rdi,2)\n";
    } else if (elemSize == 4) {
        out << "  movl %ecx, (%rax,%rdi,4)\n";
    } else {
        out << "  pushq %rcx\n";
        out << "  movq $" << elemSize << ", %rcx\n";
        out << "  imulq %rcx, %rdi\n";
        out << "  addq %rdi, %rax\n";
        out << "  popq %rcx\n";
        out << "  " << storeInstr(elemSize) << " " << valReg << ", (%rax)\n";
    }
}

// ============================================================
// captureLVal / storeTarget — protocolo de asignación
// ============================================================

// Evalúa una expresión como l-value, llenando LVal con la
// información necesaria para leer/escribir. Usado en asignaciones
// y operadores ++/--.
GenCodeVisitor::LVal GenCodeVisitor::captureLVal(Exp *e) {
    LVal lv;
    lvalTarget = &lv;
    e->accept(this);
    lvalTarget = nullptr;
    return lv;
}

// Escribe el valor actual en %rax en la dirección del l-value.
// Soporta Id, Index, Member y Deref.
void GenCodeVisitor::storeTarget(const LVal &lv) {
    switch (lv.kind) {
    case LValKind::Id:
        storeBinding(lv.binding);
        break;
    case LValKind::Index:
    {
        vector<Exp*> indices = lv.indices;

        if (!lv.members.empty()) {
            out << "  movq %rax, %rcx\n";
            // Array miembro de struct: computar base vía cadena de miembros + offset
            string structName = lv.structName;
            if (lv.isArrow && lv.binding) {
                loadBinding(lv.binding);
            } else if (lv.binding) {
                leaBinding(lv.binding);
            }
            Type* finalMemberType = nullptr;
            if (!lv.members.empty() && lv.binding &&
                structMemberTypes.count(lv.structName) &&
                structMemberTypes[lv.structName].count(lv.members.back())) {
                finalMemberType = structMemberTypes[lv.structName][lv.members.back()];
            }
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
            if (finalMemberType && finalMemberType->ttype == Type::POINTER) {
                out << "  movq (%rax), %rax\n";
            }
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
        out << "  movq %rax, %rcx\n";

        string structName = lv.structName;
        if (lv.isArrow && lv.binding) {
            loadBinding(lv.binding);
        } else if (lv.binding) {
            leaBinding(lv.binding);
        }

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

        if (!lv.indices.empty()) {
            out << "  pushq %rcx\n";
            out << "  pushq %rax\n";
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
            out << "  popq %rcx\n";

            int storeSize = 4;
            if (!lv.members.empty()) {
                storeSize = structMemberSizes[baseStructName(
                    lv.structName.empty() ? structName : lv.structName)][lv.members.back()];
                Type* mt = structMemberTypes[baseStructName(
                    lv.structName.empty() ? structName : lv.structName)][lv.members.back()];
                if (mt && mt->ttype == Type::BOOL) {
                    out << "  testq %rcx, %rcx\n";
                    out << "  setne %cl\n";
                    out << "  movzbq %cl, %rcx\n";
                }
            }
            string reg = (storeSize == 1) ? "%cl" : (storeSize == 4) ? "%ecx" : "%rcx";
            out << "  " << storeInstr(storeSize) << " " << reg << ", (%rax)\n";
        } else {
            int size = structMemberSizes[baseStructName(structName)][lv.members.back()];
            Type* mt = structMemberTypes[baseStructName(structName)][lv.members.back()];
            if (mt && mt->ttype == Type::BOOL) {
                out << "  testq %rcx, %rcx\n";
                out << "  setne %cl\n";
                out << "  movzbq %cl, %rcx\n";
            }
            string reg = (size == 1) ? "%cl" : (size == 4) ? "%ecx" : "%rcx";
            out << "  " << storeInstr(size) << " " << reg << ", (%rax)\n";
        }
        break;
    }
    case LValKind::Deref: {
        int size = lv.storeSize;
        string valReg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
        out << "  popq %rbx\n";
        out << "  " << storeInstr(size) << " " << valReg << ", (%rbx)\n";
        break;
    }
    default:
        throw runtime_error("Asignación a expresión que no es lvalue");
    }
}

// ============================================================
// directStoreForConstant — store directo de constante a variable
// ============================================================
// Si value es constante, emite el store directo usando la constante
// como inmediato, evitando pasar por %rax. Retorna true si emitió.
// No aplica para double (no cabe inmediato de 64 bits).
bool GenCodeVisitor::directStoreForConstant(Exp* value, VarDecl* vd) {
    if (!value || !vd || !vd->resolvedType) return false;

    long long intVal = 0;
    unsigned int floatBits = 0;
    bool hasFloatBits = false;

    if (auto* p = dynamic_cast<NumberLiteralNode*>(value)) {
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

    if (size == 8 && (intVal < -0x80000000LL || intVal > 0x7FFFFFFFLL)) {
        out << "  movabsq $" << intVal << ", %rax\n";
        out << "  movq %rax, " << mem << "\n";
    } else {
        string instr = (size == 1) ? "movb" : (size == 4) ? "movl" : "movq";
        out << "  " << instr << " $" << intVal << ", " << mem << "\n";
    }
    return true;
}

// ============================================================
// Expressions
// ============================================================

// Literales — cargan constantes directamente en %rax.

// Entero: maneja valores grandes con movabsq si es necesario.
void GenCodeVisitor::visit(NumberLiteralNode *e) {
    long long v = e->value;
    if (v >= -0x80000000LL && v <= 0x7FFFFFFFLL) {
        out << "  movq $" << v << ", %rax\n";
    } else if ((unsigned long long)v < 0x100000000ULL) {
        out << "  movl $" << (unsigned)(v & 0xFFFFFFFFULL) << ", %eax\n";
    } else {
        out << "  movabsq $" << v << ", %rax\n";
    }
}

// Float: empaqueta bits IEEE-754 en %rax.
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
    out << "  movq $" << (e->value ? 1 : 0) << ", %rax\n";
}

void GenCodeVisitor::visit(CharLiteralNode *e) {
    out << "  movq $" << (int)e->value << ", %rax\n";
}

void GenCodeVisitor::visit(StringLiteralNode *e) {
    // Strings en .rodata con label .LstrN.
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

// visit(IdNode) — carga variable o captura l-value.
// En modo l-value llena la estructura LVal. Si es arreglo en
// contexto rvalue, carga su dirección (array decay).
void GenCodeVisitor::visit(IdNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;
        lvalTarget->name = e->name;
        lvalTarget->binding = e->binding;
        return;
    }
    if (e->binding && e->binding->resolvedType->ttype == Type::ARRAY) {
        leaBinding(e->binding);
        return;
    }
    loadBinding(e->binding);
}

// visit(BinaryOpNode) — operaciones binarias.
// Maneja:
//   - Constantes plegadas (isConstant)
//   - Potencias x^2 y x^4 (imulq)
//   - Punto flotante vía SSE
//   - Enteros y aritmética de punteros
void GenCodeVisitor::visit(BinaryOpNode *e) {
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

    // Optimización de potencias x^2 y x^4
    if (e->op == BinaryOp::POW) {
        usedPow = true;
        auto *rightNum = dynamic_cast<NumberLiteralNode *>(e->right);
        if (rightNum && rightNum->value == 2) {
            e->left->accept(this);
            out << "  imulq %rax, %rax\n";
            return;
        }
        if (rightNum && rightNum->value == 4) {
            e->left->accept(this);
            out << "  imulq %rax, %rax\n";
            out << "  imulq %rax, %rax\n";
            return;
        }
    }

    Type* leftType = e->left->resolvedType;
    Type* rightType = e->right->resolvedType;
    bool useFloat = false;
    bool useDouble = false;
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

    // Rama de punto flotante (SSE)
    if (useFloat) {
        e->left->accept(this);
        if (useDouble) {
            if (!isFloatSemanticType(leftType))
                out << "  cvtsi2sd %rax, %xmm0\n";
            else if (leftType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm0\n";
                out << "  cvtss2sd %xmm0, %xmm0\n";
            } else
                out << "  movq %rax, %xmm0\n";
            e->right->accept(this);
            if (!isFloatSemanticType(rightType))
                out << "  cvtsi2sd %rax, %xmm1\n";
            else if (rightType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm1\n";
                out << "  cvtss2sd %xmm1, %xmm1\n";
            } else
                out << "  movq %rax, %xmm1\n";
        } else {
            if (!isFloatSemanticType(leftType))
                out << "  cvtsi2ss %eax, %xmm0\n";
            else
                out << "  movd %eax, %xmm0\n";
            e->right->accept(this);
            if (!isFloatSemanticType(rightType))
                out << "  cvtsi2ss %eax, %xmm1\n";
            else
                out << "  movd %eax, %xmm1\n";
        }

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
        case BinaryOp::NE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::LT:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setb %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::GT:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  seta %al\n";
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

        if (useDouble)
            out << "  movq %xmm0, %rax\n";
        else {
            out << "  movd %xmm0, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }

    // Rama entera y aritmética de punteros
    bool leftIsPtr = leftType && leftType->ttype == Type::POINTER;
    bool rightIsPtr = rightType && rightType->ttype == Type::POINTER;

    if ((leftIsPtr && !rightIsPtr) || (!leftIsPtr && rightIsPtr)) {
        // ptr ± int
        if (leftIsPtr) {
            e->left->accept(this);
            out << "  pushq %rax\n";
            e->right->accept(this);
            if (leftType && leftType->ttype == Type::POINTER) {
                PointerType* pt = static_cast<PointerType*>(leftType);
                int elemSize = pt->base ? pt->base->size() : 8;
                if (elemSize != 1) {
                    out << "  movq $" << elemSize << ", %rcx\n";
                    out << "  imulq %rcx, %rax\n";
                }
            }
            out << "  popq %rcx\n";
            if (e->op == BinaryOp::ADD) {
                out << "  addq %rax, %rcx\n";
            } else if (e->op == BinaryOp::SUB) {
                out << "  subq %rax, %rcx\n";
            }
            out << "  movq %rcx, %rax\n";
        } else {
            e->right->accept(this);
            out << "  pushq %rax\n";
            e->left->accept(this);
            if (rightType && rightType->ttype == Type::POINTER) {
                PointerType* pt = static_cast<PointerType*>(rightType);
                int elemSize = pt->base ? pt->base->size() : 8;
                if (elemSize != 1) {
                    out << "  movq $" << elemSize << ", %rcx\n";
                    out << "  imulq %rcx, %rax\n";
                }
            }
            out << "  popq %rcx\n";
            out << "  addq %rcx, %rax\n";
        }
    } else if (leftIsPtr && rightIsPtr && e->op == BinaryOp::SUB) {
        // ptr - ptr
        e->left->accept(this);
        out << "  pushq %rax\n";
        e->right->accept(this);
        out << "  popq %rcx\n";
        out << "  subq %rax, %rcx\n";
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
        // Operaciones enteras
        e->left->accept(this);
        out << "  pushq %rax\n";
        e->right->accept(this);
        out << "  movq %rax, %rcx\n";
        out << "  popq %rax\n";

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
                if (size == 8) {
                    out << "  mulq " << reg2 << "\n";
                } else {
                    out << "  mul" << suffix << " " << reg2 << "\n";
                }
            } else {
                out << "  imul" << suffix << " " << reg2 << ", " << reg1 << "\n";
            }
            break;
        case BinaryOp::DIV:
            if (isUnsigned) {
                out << "  xorq %rdx, %rdx\n";
                out << "  div" << suffix << " " << reg2 << "\n";
            } else {
                out << "  cqto\n";
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
            out << "  movq %rdx, %rax\n";
            break;
        case BinaryOp::POW:
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
            out << "  movq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        case BinaryOp::LOG_OR:
            out << "  or" << suffix << " " << reg2 << ", " << reg1 << "\n";
            out << "  movq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
            break;
        }
    }
}

// visit(UnaryOpNode) — operaciones unarias.
//   MINUS: negación aritmética (entero) o flip de signo SSE
//   LOG_NOT: comparación con 0 y sete
//   PRE_INC/PRE_DEC: inc/dec y store
//   POST_INC/POST_DEC: guarda valor original, inc/dec, store, restaura
//   ADDR: computeAddress del operando
//   DEREF: carga desde puntero; en modo l-value, push dirección
void GenCodeVisitor::visit(UnaryOpNode *e) {
    if (lvalTarget && e->op == UnaryOp::DEREF) {
        lvalTarget->kind = LValKind::Deref;
        lvalTarget->storeSize = e->resolvedType ? e->resolvedType->size() : 8;
        lvalTarget = nullptr;
        e->operand->accept(this);
        out << "  pushq %rax\n";
        return;
    }

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

    if (e->op == UnaryOp::ADDR) {
        e->operand->computeAddress(this);
        return;
    }

    e->operand->accept(this);

    int size = e->resolvedType ? e->resolvedType->size() : 8;
    string suffix = (size == 1) ? "b" : (size == 4) ? "l" : "q";
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";

    switch (e->op) {
    case UnaryOp::MINUS:
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
        LVal preLv = captureLVal(e->operand);
        out << "  inc" << suffix << " " << reg << "\n";
        storeTarget(preLv);
        break;
    }
    case UnaryOp::PRE_DEC:
    {
        LVal preLv = captureLVal(e->operand);
        out << "  dec" << suffix << " " << reg << "\n";
        storeTarget(preLv);
        break;
    }
    case UnaryOp::POST_INC:
    {
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
    case UnaryOp::DEREF: {
        int derefSize = e->resolvedType ? e->resolvedType->size() : 8;
        bool derefUnsigned = e->resolvedType && e->resolvedType->isUnsigned;
        out << "  " << loadInstr(derefSize, derefUnsigned) << " (%rax), %rax\n";
        break;
    }
    default:
        break;
    }
}

// visit(AssignmentNode) — asignación.
// Flujo:
//   1. Capturar target como l-value.
//   2. Si es variable simple con constante, usar directStoreForConstant.
//   3. Evaluar value → %rax.
//   4. Promocionar/converter según tipo destino.
//   5. storeTarget escribe %rax en el l-value.
void GenCodeVisitor::visit(AssignmentNode *e) {
    LVal target = captureLVal(e->target);

    if (target.kind == LValKind::Id && target.binding) {
        if (directStoreForConstant(e->value, target.binding))
            return;
    }

    e->value->accept(this);

    Type* tgtType = nullptr;
    Type* valType = e->value->resolvedType;
    if (target.kind == LValKind::Id && target.binding) {
        tgtType = target.binding->resolvedType;
    } else if (target.kind == LValKind::Index && target.binding) {
        tgtType = target.binding->resolvedType;
        while (tgtType && tgtType->ttype == Type::ARRAY)
            tgtType = ((ArrayType*)tgtType)->base;
    }
    if (tgtType && tgtType->ttype == Type::DOUBLE) {
        if (valType && valType->ttype == Type::FLOAT) {
            out << "  movd %eax, %xmm7\n";
            out << "  cvtss2sd %xmm7, %xmm7\n";
            out << "  movq %xmm7, %rax\n";
        } else if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
            out << "  cvtsi2sd %rax, %xmm7\n";
            out << "  movq %xmm7, %rax\n";
        }
    } else if (tgtType && tgtType->ttype == Type::FLOAT) {
        if (valType && valType->ttype == Type::DOUBLE) {
            out << "  movq %rax, %xmm7\n";
            out << "  cvtsd2ss %xmm7, %xmm7\n";
            out << "  movd %xmm7, %eax\n";
            out << "  movslq %eax, %rax\n";
        } else if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
            out << "  cvtsi2ss %eax, %xmm7\n";
            out << "  movd %xmm7, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
    } else if (tgtType && (tgtType->ttype == Type::INT || tgtType->ttype == Type::CHAR || tgtType->ttype == Type::LONG)) {
        if (valType && valType->ttype == Type::FLOAT) {
            out << "  movd %eax, %xmm7\n";
            if (tgtType->ttype == Type::LONG) {
                out << "  cvtss2siq %xmm7, %rax\n";
            } else {
                out << "  cvtss2si %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        } else if (valType && valType->ttype == Type::DOUBLE) {
            out << "  movq %rax, %xmm7\n";
            if (tgtType->ttype == Type::LONG) {
                out << "  cvtsd2siq %xmm7, %rax\n";
            } else {
                out << "  cvtsd2si %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        }
    }

    storeTarget(target);
}

// parsePrintfSpecs — analiza especificadores de formato de printf.
// Retorna "int" o "float" por cada argumento esperado.
static vector<string> parsePrintfSpecs(const string& fmt) {
    vector<string> specs;
    for (size_t i = 0; i < fmt.size(); i++) {
        if (fmt[i] != '%') continue;
        i++;
        if (i >= fmt.size()) break;
        if (fmt[i] == '%') continue;

        // Saltar flags
        while (i < fmt.size() && (fmt[i] == '-' || fmt[i] == '+' ||
               fmt[i] == ' ' || fmt[i] == '#' || fmt[i] == '0'))
            i++;

        // Width
        if (i < fmt.size() && fmt[i] == '*') { i++; }
        else {
            while (i < fmt.size() && fmt[i] >= '0' && fmt[i] <= '9') i++;
        }

        // Precision
        if (i < fmt.size() && fmt[i] == '.') {
            i++;
            if (i < fmt.size() && fmt[i] == '*') { i++; }
            else {
                while (i < fmt.size() && fmt[i] >= '0' && fmt[i] <= '9') i++;
            }
        }

        // Length modifier
        if (i < fmt.size() && fmt[i] == 'l') {
            i++;
            if (i < fmt.size() && fmt[i] == 'l') i++;
        } else if (i < fmt.size() && fmt[i] == 'h') {
            i++;
            if (i < fmt.size() && fmt[i] == 'h') i++;
        } else if (i < fmt.size() && (fmt[i] == 'L' || fmt[i] == 'z' || fmt[i] == 'j' || fmt[i] == 't')) {
            i++;
        }

        if (i >= fmt.size()) break;
        char conv = fmt[i];

        bool isFloat = (conv == 'f' || conv == 'F' || conv == 'e' || conv == 'E' ||
                       conv == 'g' || conv == 'G' || conv == 'a' || conv == 'A');
        specs.push_back(isFloat ? "float" : "int");
    }
    return specs;
}

// visit(PrintfNode) — llamada a printf.
// Sigue la convención System V para funciones variádicas:
//   %rdi = formato, %rsi/%rdx/%rcx/%r8/%r9 = enteros,
//   %xmm0–%xmm7 = floats, %rax = número de xmm usados.
void GenCodeVisitor::visit(PrintfNode *e) {
    auto it = stringLabels.find(e->format);
    int lbl;
    if (it == stringLabels.end()) {
        lbl = (int)stringLabels.size();
        stringLabels[e->format] = lbl;
        out << ".section .rodata\n";
        out << ".Lfmt" << lbl << ": .string \"" << e->format << "\"\n";
        out << ".text\n";
    } else {
        lbl = it->second;
    }
    string fmtLabel = ".Lfmt" + to_string(lbl);

    vector<string> specs = parsePrintfSpecs(e->format);

    const vector<string> intRegs = {"%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    const vector<string> xmmRegs = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"};

    int intIdx = 0, xmmIdx = 0;
    for (size_t i = 0; i < e->args.size() && i < specs.size(); i++) {
        e->args[i]->accept(this);

        if (specs[i] == "float") {
            if (xmmIdx < (int)xmmRegs.size()) {
                Type* argType = e->args[i]->resolvedType;
                if (argType && argType->ttype == Type::FLOAT) {
                    out << "  movd %eax, " << xmmRegs[xmmIdx] << "\n";
                    out << "  cvtss2sd " << xmmRegs[xmmIdx] << ", " << xmmRegs[xmmIdx] << "\n";
                } else {
                    out << "  movq %rax, " << xmmRegs[xmmIdx] << "\n";
                }
                xmmIdx++;
            } else {
                out << "  pushq %rax\n";
            }
        } else {
            if (intIdx < (int)intRegs.size()) {
                out << "  movq %rax, " << intRegs[intIdx] << "\n";
                intIdx++;
            } else {
                out << "  pushq %rax\n";
            }
        }
    }

    out << "  leaq " << fmtLabel << "(%rip), %rdi\n";
    out << "  movq $" << xmmIdx << ", %rax\n";
    out << "  call printf@PLT\n";

    // Limpiar argumentos extra del stack
    int totalRegs = min((int)e->args.size(), (int)(intRegs.size() + xmmRegs.size()));
    if ((int)e->args.size() > totalRegs) {
        out << "  addq $" << ((e->args.size() - totalRegs) * 8) << ", %rsp\n";
    }

    out << "  movq $0, %rax\n";
}

// visit(FcallNode) — llamada a función.
// Push de argumentos extra (>6) en orden inverso, carga de los
// primeros 6 en registros, call, y limpieza de stack.
void GenCodeVisitor::visit(FcallNode *e) {
    const vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    auto *id = dynamic_cast<IdNode *>(e->callee);
    string fname = id ? id->name : "";

    int nArgs = (int)e->args.size();

    for (int i = nArgs - 1; i >= 6; i--) {
        e->args[i]->accept(this);
        out << "  pushq %rax\n";
    }

    for (int i = 0; i < nArgs && i < 6; i++) {
        e->args[i]->accept(this);
        out << "  movq %rax, " << argRegs[i] << "\n";
    }

    out << "  call " << fname << "\n";

    if (nArgs > 6) {
        out << "  addq $" << ((nArgs - 6) * 8) << ", %rsp\n";
    }
}

// visit(IndexNode) — acceso por índice.
// En modo l-value: guarda índices y binding en LVal.
// En modo valor: usa emitIndexedLoad para arrays planos, o path general.
void GenCodeVisitor::visit(IndexNode *e) {
    if (lvalTarget) {
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

    // Path rápido para arrays planos a[i][j]
    if (dynamic_cast<IdNode*>(e->base)) {
        vector<Exp*> indices;
        Exp* b = nullptr;
        collectIndices(e, indices, b);
        if (auto* id = dynamic_cast<IdNode*>(b)) {
            if (e->resolvedType && e->resolvedType->ttype == Type::ARRAY) {
                emitIndexedAddress(id->binding, indices);
            } else {
                emitIndexedLoad(id->binding, indices);
            }
            return;
        }
    }

    // Path general
    e->base->accept(this);
    out << "  pushq %rax\n";
    e->index->accept(this);
    out << "  movq %rax, %rcx\n";

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

    if (!(e->resolvedType && e->resolvedType->ttype == Type::ARRAY)) {
        int loadSize = 4;
        if (e->resolvedType) loadSize = e->resolvedType->size();
        bool idxUnsigned = e->resolvedType && e->resolvedType->isUnsigned;
        if (loadSize == 1) out << "  " << loadInstr(1, idxUnsigned) << " (%rax), %rax\n";
        else if (loadSize == 4) out << "  movslq (%rax), %rax\n";
        else out << "  movq (%rax), %rax\n";
    }
}

// visit(MemberAccessNode) — acceso a miembro (.).
// En modo l-value: guarda info en LVal. En modo valor: computa
// dirección del objeto y carga desde offset del miembro.
void GenCodeVisitor::visit(MemberAccessNode *e) {
    if (lvalTarget) {
        LVal objLVal;
        LVal *oldTarget = lvalTarget;
        lvalTarget = &objLVal;
        e->object->accept(this);
        lvalTarget = oldTarget;

        *lvalTarget = objLVal;
        lvalTarget->kind = LValKind::Member;
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

    Type* objType = e->object->resolvedType;
    string currentStructName;
    LVal objLVal = captureLVal(e->object);
    
    if (objLVal.kind == LValKind::Id) {
        leaBinding(objLVal.binding);
        if (objLVal.binding && objLVal.binding->resolvedType && 
            objLVal.binding->resolvedType->ttype == Type::STRUCT) {
            currentStructName = ((StructType*)objLVal.binding->resolvedType)->name;
        }
    } else if (objLVal.kind == LValKind::Index) {
        emitIndexedAddress(objLVal.binding, objLVal.indices);
    } else if (objLVal.kind == LValKind::Member) {
        if (objLVal.binding) {
            if (objLVal.isArrow) {
                loadBinding(objLVal.binding);
            } else {
                leaBinding(objLVal.binding);
            }
        }
        currentStructName = objLVal.structName;
        for (size_t i = 0; i < objLVal.members.size(); ++i) {
            const string& m = objLVal.members[i];
            if (i > 0 && i < objLVal.memberArrow.size() && objLVal.memberArrow[i]) {
                out << "  movq (%rax), %rax\n";
            }
            int off = structFieldOffset[baseStructName(currentStructName)][m];
            out << "  addq $" << off << ", %rax\n";
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
    }
    
    if (currentStructName.empty() && objType) {
        if (objType->ttype == Type::STRUCT) {
            currentStructName = ((StructType*)objType)->name;
        }
    }

    int off = structFieldOffset[baseStructName(currentStructName)][e->member];
    Type* mt = nullptr;
    if (structMemberTypes.count(baseStructName(currentStructName)) &&
        structMemberTypes[baseStructName(currentStructName)].count(e->member)) {
        mt = structMemberTypes[baseStructName(currentStructName)][e->member];
        if (mt && mt->ttype == Type::ARRAY) {
            out << "  addq $" << off << ", %rax\n";
            return;
        }
    }
    int size = structMemberSizes[baseStructName(currentStructName)][e->member];
    bool memberUnsigned = mt && mt->isUnsigned;
    out << "  " << loadInstr(size, memberUnsigned) << " " << off << "(%rax), %rax\n";
}

// visit(ArrowAccessNode) — acceso a miembro vía puntero (->).
// Similar a MemberAccessNode pero carga primero el puntero.
void GenCodeVisitor::visit(ArrowAccessNode *e) {
    if (lvalTarget) {
        LVal ptrLVal;
        LVal *oldTarget = lvalTarget;
        lvalTarget = &ptrLVal;
        e->pointer->accept(this);
        lvalTarget = oldTarget;

        *lvalTarget = ptrLVal;
        lvalTarget->kind = LValKind::Member;
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

    e->pointer->accept(this);
    Type* ptrType = e->pointer->resolvedType;
    string structName;
    if (ptrType && ptrType->ttype == Type::POINTER) {
        PointerType* pt = (PointerType*)ptrType;
        if (pt->base && pt->base->ttype == Type::STRUCT) {
            structName = ((StructType*)pt->base)->name;
        }
    }
    int off = structFieldOffset[baseStructName(structName)][e->member];
    Type* mt = structMemberTypes[baseStructName(structName)][e->member];
    if (mt && mt->ttype == Type::ARRAY) {
        out << "  addq $" << off << ", %rax\n";
    } else {
        int size = structMemberSizes[baseStructName(structName)][e->member];
        bool memberUnsigned = mt && mt->isUnsigned;
        out << "  " << loadInstr(size, memberUnsigned) << " " << off << "(%rax), %rax\n";
    }
}

// visit(MallocNode) — llamada a malloc.
// Evalúa el tamaño en %rdi y llama a malloc@PLT.
void GenCodeVisitor::visit(MallocNode *e) {
    e->size->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call malloc@PLT\n";
}

// visit(SizeOfNode) — sizeof.
// TypeChecker ya calculó constantValue.
void GenCodeVisitor::visit(SizeOfNode *e) {
    out << "  movq $" << (long long)e->constantValue << ", %rax\n";
}

// Nodos de tipo puro: no generan instrucciones.
void GenCodeVisitor::visit(PrimitiveTypeNode *) {}
void GenCodeVisitor::visit(PointerTypeNode *) {}
void GenCodeVisitor::visit(StructTypeNode *) {}


// ============================================================
// Statements
// ============================================================

// visit(Body) — bloque { stmt; ... }.
// Genera cada statement en orden.
void GenCodeVisitor::visit(Body *s) {
    for (auto st : s->stmts)
        st->accept(this);
}

// visit(ExprStmtNode) — expresión como statement.
void GenCodeVisitor::visit(ExprStmtNode *s) {
    if (s->expr)
        s->expr->accept(this);
}

// visit(IfStmt) — if/else.
// Genera: cond; je else; then; jmp endif; else: ...; endif:
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

// visit(WhileStmt) — while loop.
// Genera: while_N: cond; je end; body; jmp while_N; end:
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

// visit(DoWhileStmt) — do-while loop.
// Genera: dowhile_N: body; docond_N: cond; jne dowhile_N; end:
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

// visit(ForStmt) — for loop.
// Genera: init; for_N: cond; je end; body; forinc_N: inc; jmp for_N; end:
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

// visit(SwitchStmt) — switch.
// Genera: eval expr en %r10; cmp con cada case; je case_N;
// jmp default; case_N: ...; default: ...; endswitch_N:
void GenCodeVisitor::visit(SwitchStmt *s) {
    int lbl = labelcont++;
    s->expr->accept(this);
    out << "  movq %rax, %r10\n";

    int caseIdx = 0;
    for (auto cc : s->cases) {
        if (auto *lit = dynamic_cast<NumberLiteralNode *>(cc->value))
            out << "  movq $" << lit->value << ", %rax\n";
        else
            out << "  movq $0, %rax\n";
        out << "  cmpq %rax, %r10\n";
        out << "  je case_" << lbl << "_" << caseIdx << "\n";
        caseIdx++;
    }

    out << "  jmp default_" << lbl << "\n";

    string oldBreak = currentBreakLabel;
    currentBreakLabel = "endswitch_" + to_string(lbl);

    caseIdx = 0;
    for (auto cc : s->cases) {
        out << "case_" << lbl << "_" << caseIdx << ":\n";
        for (auto st : cc->body) st->accept(this);
        caseIdx++;
    }

    out << "default_" << lbl << ":\n";
    for (auto st : s->default_body) st->accept(this);

    currentBreakLabel = oldBreak;
    out << "endswitch_" << lbl << ":\n";
}

// CaseClause no se visita directamente.
void GenCodeVisitor::visit(CaseClause *) {}

// visit(BreakStmt) — salta al label de fin del loop/switch actual.
void GenCodeVisitor::visit(BreakStmt *) {
    if (currentBreakLabel.empty()) {
        cerr << "Error: break fuera de ciclo o switch\n";
        exit(1);
    }
    out << "  jmp " << currentBreakLabel << "\n";
}

// visit(ContinueStmt) — salta al label de continuación del loop actual.
void GenCodeVisitor::visit(ContinueStmt *) {
    if (currentContinueLabel.empty()) {
        cerr << "Error: continue fuera de ciclo\n";
        exit(1);
    }
    out << "  jmp " << currentContinueLabel << "\n";
}

// visit(ReturnStmt) — evalúa la expresión y salta al epílogo.
void GenCodeVisitor::visit(ReturnStmt *s) {
    if (s->expr)
        s->expr->accept(this);
    else
        out << "  movq $0, %rax\n";
    string target = returnLabel.empty() ? ".end_" + funcName : returnLabel;
    out << "  jmp " << target << "\n";
}

// visit(FreeStmt) — free.
void GenCodeVisitor::visit(FreeStmt *s) {
    s->expr->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call free@PLT\n";
}

// ============================================================
// Declarations
// ============================================================

// visit(VarDecl) — declaración de variable.
// Globales: ya están en .data. Locales: bind + evaluar inicializador
// o lista de inicialización.
void GenCodeVisitor::visit(VarDecl *d) {
    if (!inFunction) {
        memoriaGlobal[d->name] = true;
        return;
    }

    bind_var_decl(d);

    if (d->initializer) {
        if (directStoreForConstant(d->initializer, d))
            return;
        
        d->initializer->accept(this);
        if (d->resolvedType && d->resolvedType->ttype == Type::DOUBLE) {
            Type* initType = d->initializer->resolvedType;
            if (initType && initType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm7\n";
                out << "  cvtss2sd %xmm7, %xmm7\n";
                out << "  movq %xmm7, %rax\n";
            } else if (!initType || initType->ttype == Type::INT || initType->ttype == Type::CHAR) {
                out << "  cvtsi2sd %rax, %xmm7\n";
                out << "  movq %xmm7, %rax\n";
            }
        } else if (d->resolvedType && d->resolvedType->ttype == Type::FLOAT) {
            Type* initType = d->initializer->resolvedType;
            if (initType && initType->ttype == Type::DOUBLE) {
                out << "  movq %rax, %xmm7\n";
                out << "  cvtsd2ss %xmm7, %xmm7\n";
                out << "  movd %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        } else if (d->resolvedType && (d->resolvedType->ttype == Type::INT || d->resolvedType->ttype == Type::CHAR || d->resolvedType->ttype == Type::LONG)) {
            Type* initType = d->initializer->resolvedType;
            if (initType && initType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm7\n";
                if (d->resolvedType->ttype == Type::LONG) {
                    out << "  cvtss2siq %xmm7, %rax\n";
                } else {
                    out << "  cvtss2si %xmm7, %eax\n";
                    out << "  movslq %eax, %rax\n";
                }
            } else if (initType && initType->ttype == Type::DOUBLE) {
                out << "  movq %rax, %xmm7\n";
                if (d->resolvedType->ttype == Type::LONG) {
                    out << "  cvtsd2siq %xmm7, %rax\n";
                } else {
                    out << "  cvtsd2si %xmm7, %eax\n";
                    out << "  movslq %eax, %rax\n";
                }
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

// visit(FunDecl) — declaración de función.
// Genera: .globl, prólogo, copia de parámetros a stack, body,
// epílogo con .end_nombre: leave; ret.
void GenCodeVisitor::visit(FunDecl *d) {
    inFunction = true;
    memoria.clear();
    funcName = d->name;
    returnLabel = ".end_" + d->name;

    int frameSize = d->frameSize;

    out << "\n.globl " << d->name << "\n";
    out << d->name << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";

    const vector<string> argRegs32 = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    const vector<string> argRegs64 = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    const vector<string> argRegs8 = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
    for (size_t i = 0; i < d->params.size() && i < 6; i++) {
        bind_var_decl(d->params[i]);

        int size = d->params[i]->memSize;
        if (size == 4)
            out << "  movl " << argRegs32[i] << ", " << d->params[i]->offset << "(%rbp)\n";
        else if (size == 1)
            out << "  movb " << argRegs8[i] << ", " << d->params[i]->offset << "(%rbp)\n";
        else
            out << "  movq " << argRegs64[i] << ", " << d->params[i]->offset << "(%rbp)\n";
    }

    d->body->accept(this);

    out << ".end_" << d->name << ":\n";
    out << "  leave\n";
    out << "  ret\n";
    inFunction = false;
}

// visit(StructDecl) — copia offsets y tamaños de miembros a mapas
// de GenCodeVisitor para accesos posteriores.
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

// visit(Program) — redirige al punto de entrada generate().
void GenCodeVisitor::visit(Program *p) {
    generate(p);
}

// ============================================================
// computeAddress — calcula dirección de un l-value en %rax
// ============================================================
// Invocado vía node->computeAddress(this). Difiere de visit()
// porque deja la dirección, no el valor.

// &(*p) o dirección para *p = val: la dirección es el valor del puntero.
void GenCodeVisitor::computeAddress(UnaryOpNode *e) {
    e->operand->accept(this);
}

// &variable → leaq offset(%rbp), %rax
void GenCodeVisitor::computeAddress(IdNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;
        lvalTarget->name = e->name;
        lvalTarget->binding = e->binding;
        return;
    }
    leaBinding(e->binding);
}

// &arr[i][j] → emitIndexedAddress.
void GenCodeVisitor::computeAddress(IndexNode *e) {
    vector<Exp*> indices;
    Exp* b = nullptr;
    collectIndices(e, indices, b);

    VarDecl* arrBinding = nullptr;
    if (auto *id = dynamic_cast<IdNode *>(b))
        arrBinding = id->binding;

    emitIndexedAddress(arrBinding, indices);
}

// &s.miembro → dirección de s + offset del campo.
void GenCodeVisitor::computeAddress(MemberAccessNode *e) {
    if (auto *id = dynamic_cast<IdNode *>(e->object)) {
        leaBinding(id->binding);
        string structName = id->name;
        if (id->resolvedType && id->resolvedType->ttype == Type::STRUCT) {
            structName = ((StructType*)id->resolvedType)->name;
        }
        out << "  addq $" << structFieldOffset[baseStructName(structName)][e->member] << ", %rax\n";
    }
}

// &p->miembro → carga p y suma offset del campo.
void GenCodeVisitor::computeAddress(ArrowAccessNode *e) {
    if (auto *id = dynamic_cast<IdNode *>(e->pointer)) {
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
