# Guía de Implementación: Punteros, Promoción de Tipos, float/double y Matrices

Este documento describe el flujo de cada característica a través de las tres fases principales del compilador `c--`:
**AST → TypeChecker → Code Generation (Gencode)**.

---

## 1. Punteros Explícitos (`*`, `&`, `->`)

### AST

| Nodo AST | Archivo | Rol |
|---|---|---|
| `UnaryOpNode(ADDR)` | `ast.h:111` | Operador `&expr` — obtiene dirección |
| `UnaryOpNode(DEREF)` | `ast.h:111` | Operador `*expr` — desreferencia |
| `PointerTypeNode` | `ast.h:62` | Nodo de tipo `T*` — `TypeNode* base` |
| `ArrowAccessNode` | `ast.h:172` | `ptr->member` — acceso por puntero a struct |
| `PointerType` (semántico) | `semantic_types.h:56` | `Type* base`, `size() = 8` |

**Parser** (`parser.cpp`):
- `parse_type()` (L144): consume `*` tras un tipo base → anida `PointerTypeNode`.
- `parse_unary()` (L690): `&` → `UnaryOpNode(ADDR)`; `*` → `UnaryOpNode(DEREF)`.
- `parse_postfix()` (L718): `->` → `ArrowAccessNode`.

### TypeChecker

**`type_from_ast()`** (`TypeChecker.cpp:128`): `PointerTypeNode` → crea `PointerType(tipo_base)` y lo guarda en `typeCache`.

**`visit(UnaryOpNode)`** (`TypeChecker.cpp:846`):

| Caso | Comportamiento |
|---|---|
| `ADDR` (L877) | `resolvedType = new PointerType(t)` — envuelve el tipo del operando |
| `DEREF` (L884) | Si el operando es `PointerType`, `resolvedType = pt->base` (desenvuelve). Error si no es puntero |

**`visit(ArrowAccessNode)`** (`TypeChecker.cpp:1029`):
- Verifica que `ptrType->ttype == POINTER`
- Verifica que `pt->base->ttype == STRUCT`
- Busca el miembro en el `StructType`
- `resolvedType = tipo_del_miembro`

**Aritmética de punteros** en `visit(BinaryOpNode)` (L773–780):
- `ptr + int` o `int + ptr` → resultado es el tipo puntero
- `ptr - ptr` → resultado es `int`
- `ptr - int` → resultado es el tipo puntero

**Array-to-pointer decay** en `visit(IdNode)` (L1067): en contexto rvalue, si la variable es `ARRAY`, `resolvedType` se convierte a `PointerType(at->base)`.

**Asignación** (`check_assign()`, L173): cualquier puntero → cualquier puntero es válido (coerción laxa).

### Code Generation (Gencode)

**`computeAddress`** (`Gencode.cpp:2099`):

| Nodo | Comportamiento |
|---|---|
| `UnaryOpNode(ADDR)` | Delega a `e->operand->accept(this)` (carga normal del operando) |
| `IdNode` | `leaq offset(%rbp), %rax` |
| `IndexNode` | `emitIndexedAddress()` — calcula dirección lineal |
| `MemberAccessNode` | `leaq` + `addq` del field offset |
| `ArrowAccessNode` | `loadBinding()` + `addq` del field offset |

**`visit(UnaryOpNode)` para DEREF** (`Gencode.cpp:1265`):
- En modo l-value (`lvalTarget` activo, L1167): guarda `kind = Deref`, hace `pushq %rax` con la dirección del puntero para que `storeTarget` escriba ahí.
- En modo r-value (L1265): `mov[sz] (%rax), %rax` — carga desde la dirección apuntada.

**Aritmética de punteros** en `visit(BinaryOpNode)` (L989): `ptr ± int` escala el entero por `elemSize` (tamaño del tipo apuntado) usando `imulq` y luego `addq`.

**`captureLVal` / `storeTarget`** (L493): protocolo para asignaciones. `storeTarget` escribe `%rax` en la ubicación descrita por `LVal` (Id, Index, Member, o Deref).

---

## 2. Promoción/Conversión Automática de Tipos

### AST

No hay nodos AST específicos para promoción — es una operación semántica y de generación de código. Los nodos involucrados son `BinaryOpNode`, `AssignmentNode`, `VarDecl` (con initializer), `ReturnStmt` y `FcallNode`.

### TypeChecker

**Jerarquía de promoción** en `visit(BinaryOpNode)` (L781–797):

```
double > float > long > int/char/bool
```

Para operaciones aritméticas mixtas, el resultado se promueve al tipo más grande:
```cpp
if (left->ttype == Type::DOUBLE || right->ttype == Type::DOUBLE)
    resultType = doubleType;
else if (left->ttype == Type::FLOAT || right->ttype == Type::FLOAT)
    resultType = floatType;
else if (left->ttype == Type::LONG || right->ttype == Type::LONG)
    resultType = longType;
else if (left->isUnsigned || right->isUnsigned)
    resultType = uintType;
else
    resultType = intType;
```

**`check_assign()`** (`TypeChecker.cpp:173`) — tabla de conversiones implícitas en asignación:

| Destino | Origen | Comportamiento |
|---|---|---|
| `T` | `T` | Match exacto |
| `T*` | `U*` | Coerción laxa (cualquier puntero) |
| `int` | `char`, `long` | Promoción/truncamiento |
| `long` | `int`, `char`, `bool` | Promoción |
| `float`/`double` | `int`, `char`, `long` | Promoción entero → punto flotante |
| `double` | `float` | Widening |
| `float` | `double` | Narrowing |
| `int`/`char`/`long` | `float`/`double` | Truncamiento |
| `bool` | `int`/`char`/`long` | Cero → false, no-cero → true |
| `int`/`char`/`long` | `bool` | Booleano a entero |

### Code Generation

**Conversiones en asignación** (`visit(AssignmentNode)`, L1293–1349):

| De | A | Instrucción |
|---|---|---|
| `float` | `double` | `cvtss2sd` |
| `int`/`char` | `double` | `cvtsi2sd` |
| `double` | `float` | `cvtsd2ss` |
| `int`/`char` | `float` | `cvtsi2ss` |
| `float` | `int` | `cvtss2si` |
| `float` | `long` | `cvtss2siq` |
| `double` | `int` | `cvtsd2si` |
| `double` | `long` | `cvtsd2siq` |

**Conversiones en operaciones binarias** (`visit(BinaryOpNode)`, L893–981):
- Si algún operando es `float`/`double`, ambos operandos se convierten al formato más ancho usando `cvtsi2sd`/`cvtss2sd`/`cvtsi2ss`, se opera con instrucciones SSE (`addss`/`addsd`, etc.), y el resultado se deja en `%xmm0` (reempaquetado a `%rax`).
- Si ningún operando es float: se opera con instrucciones enteras.

**Conversiones en paso de argumentos a `printf`** (L1409): los `float` se convierten a `double` con `cvtss2sd` (ABI variádica).

---

## 3. `float` / `double`

### AST

| Nodo AST | Archivo | Rol |
|---|---|---|
| `PrimitiveTypeNode::FLOAT` / `DOUBLE` | `ast.h:52` | Tipo primitivo en declaraciones |
| `FloatLiteralNode` | `ast.h:232` | Literal flotante (`3.14`, `1e-5`), con `literalSuffix` (`SUF_F` → float, `SUF_NONE` → double) |

**Parser**: `Token::FLOAT` y `Token::DOUBLE` en `parse_basic_type()`. `Token::FNUM` → `FloatLiteralNode`.

### TypeChecker

**Singletons**: `floatType`, `doubleType` creados en constructor (L72–73).

**`type_from_ast()`** (`TypeChecker.cpp:113–114`): `PrimitiveTypeNode::FLOAT` → `floatType`, `PrimitiveTypeNode::DOUBLE` → `doubleType`.

**`visit(FloatLiteralNode)`** (`TypeChecker.cpp:1110`):
```cpp
e->resolvedType = (e->literalSuffix == Token::LiteralSuffix::SUF_F)
                  ? floatType : doubleType;
```

**Operaciones unarias** (`visit(UnaryOpNode)`, L851): `-float` → `float`, `-double` → `double` (no promueve a `double` como los enteros).

**Promoción binaria** (L781): `double > float > otros`.

### Code Generation

**`isFloatSemanticType()`** (`Gencode.cpp:54`): helper que detecta `FLOAT` o `DOUBLE`.

**Carga/almacenamiento** (`loadBinding`/`storeBinding`, L278–304):
- `double`: `movsd`/`movq` entre memoria y `%xmm7`/`%rax`
- `float`: `movss`/`movd` entre memoria y `%xmm7`/`%eax`

**Operaciones aritméticas SSE** (`visit(BinaryOpNode)` L924–971):

| Operación | float | double |
|---|---|---|
| `+` | `addss` | `addsd` |
| `-` | `subss` | `subsd` |
| `*` | `mulss` | `mulsd` |
| `/` | `divss` | `divsd` |
| `==` | `ucomiss` + `sete` | `ucomisd` + `sete` |
| `<` | `ucomiss` + `setb` | `ucomisd` + `setb` |
| `>` | `ucomiss` + `seta` | `ucomisd` + `seta` |

**Negación unaria** (`visit(UnaryOpNode)`, L1208): XOR del bit de signo:
- `double`: `xorpd` con `0x8000000000000000`
- `float`: `xorps` con `0x80000000`

**Literales flotantes** (`visit(FloatLiteralNode)`, en el handler de constantes L1176–1192): emisión del patrón IEEE-754 como inmediato:
- `double`: `movq $0x<hex64>, %rax`
- `float`: `movl $0x<hex32>, %eax` + `movslq %eax, %rax`

**Inicialización de globales** (`generate()`, L83–92):
- `double`: `.quad 0x<hex64>`
- `float`: `.long 0x<hex32>`

---

## 4. Matrices (Arreglos Multidimensionales)

### AST

| Nodo AST | Archivo | Rol |
|---|---|---|
| `VarDecl::array_sizes` | `ast.h:423` | `vector<Exp*>` con expresiones de cada `[dim]` |
| `IndexNode` | `ast.h:148` | `base[index]` — acceso por índice |
| `ArrayType` (semántico) | `semantic_types.h:70` | `Type* base`, `int length`, `size() = base * length` |

**Parser** (`parse_array_suffix()`, L131): tras el nombre de la variable, consume `[expr]` repetidas veces y las agrega a `VarDecl::array_sizes`.

Ejemplo: `int m[2][3]` → `array_sizes = [2, 3]` (en orden de escritura).

### TypeChecker

**Construcción del tipo array** en `visit(FunDecl)` (`TypeChecker.cpp:322–330`): Recorre `array_sizes` en **orden inverso**, anidando `ArrayType`:

```
array_sizes = [2, 3], base = int
  → idx=1: ArrayType(int, 3)
  → idx=0: ArrayType(ArrayType(int, 3), 2)
```

El resultado es `ArrayType(ArrayType(int, 3), 2)` — la dimensión más externa envuelve a la interna.

**`visit(IndexNode)`** (`TypeChecker.cpp:984`):
- Si la base es `ARRAY`: `resolvedType = at->base` (pela una dimensión)
- Si la base es `POINTER`: `resolvedType = pt->base`
- Verifica que el índice sea entero (`is_switch_index_type`)

**Array-to-pointer decay** en `visit(IdNode)` (L1067): en rvalue, todo el arreglo decae a `PointerType(at->base)`. Así, pasar un arreglo a una función entrega un puntero al primer elemento.

### Code Generation

**Dimensiones** (`arrayDimsFor()` / `arrayDimsForType()`, L231–252): extrae vector de dimensiones desde el `ArrayType` anidado.

**`array_elem_size()`** (L216): recorre el árbol de `ArrayType` hasta el tipo base y retorna su tamaño.

**`collectIndices()`** (L259): recorre una cadena de `IndexNode`s como `a[i][j][k]` y recolecta `indices = [i, j, k]`, `base = a`.

**`emitIndexedAddress()`** (L327): calcula la dirección de `arr[i][j]`:

1. Carga dirección base del arreglo (`leaBinding`).
2. Para cada índice, calcula el stride (producto de dimensiones siguientes).
3. Acumula `offset = Σ (índice_d × stride_d)`.
4. Escala por `elemSize` (`leaq (%rax,%r10,4)` o `imulq`).

```
Ejemplo: int m[2][3], acceder m[i][j]
  → stride[0] = 3, stride[1] = 1
  → offset_linear = i*3 + j
  → dirección = base + offset_linear * 4
```

**`emitIndexedLoad()`** (L384): calcula dirección y carga el valor desde `(%rax)`.

**`emitIndexedStore()`** (L427): calcula dirección y escribe el valor en `(%rax)`.

**Protocolo con LVal** (`captureLVal` L493): cuando se asigna a `arr[i][j]`, `captureLVal` reconoce `IndexNode` y guarda los índices en `LVal::indices`. Luego `storeTarget` invoca `emitIndexedStore`.

**Structs con arreglos como miembros** (`storeTarget` L512–594): caso especial donde se computa la dirección base del struct, se suman offsets de campos, y luego se aplica el indexado multidimensional.

---

## 5. Benchmarks y Comparación de Rendimiento

### Metodología

Los benchmarks se ejecutan desde `comparativa/scripts/run_all.sh`, que orquesta dos scripts de Python:

1. **`run_benchmarks.py`** — realiza las mediciones y genera CSVs.
2. **`analyze_results.py`** — lee los CSVs, imprime tablas y genera gráficos SVG.

**Configuración de medición:**
- **7 repeticiones** por medición, usando la **mediana** como métrica.
- Timeout de **120 segundos** por ejecución.
- 3 compiladores en 6 configuraciones:

| Abreviatura | Compilador | Flags |
|---|---|---|
| `C--_Codegen` | `c--` | `-c` (solo frontend + codegen → `/dev/null`) |
| `C--_Full` | `c--` | `--exec` (pipeline completo a binario) |
| `GCC_O0` | GCC 16.1.1 | `-O0` |
| `GCC_O2` | GCC 16.1.1 | `-O2` |
| `Clang_O0` | Clang 22.1.6 | `-O0` |
| `Clang_O2` | Clang 22.1.6 | `-O2` |

**9 programas de benchmark**, divididos en:

| Benchmark | Descripción | Carga | Característica ejercitada |
|---|---|---|---|
| `bench_fib` | Fibonacci recursivo | n=35 | Recursión, llamadas, flujo de control |
| `bench_matmul` | Multiplicación de matrices 80×80 | 80×80 | Arrays 2D, loops anidados, aritmética |
| `bench_float` | Suma de cuadrados float | 500,000 iters | Aritmética float, loops |
| `bench_struct` | Struct + punteros | 500,000 iters | Structs, `->`, punteros |
| `bench_prime` | Criba de primos | hasta 40,000 | Bucles, módulo, condicionales, `break` |
| `bench_mixed` | Struct mixto int + float | 150,000 iters | Tipos combinados |
| `bench_constfold` | Expresión constante en loop | 12M iters | Constant folding |
| `bench_conststore` | 6 vars con literales en loop | 15M iters | Constant store peephole |
| `bench_stackframe` | 12 int + 12 char sumados | 1.5M iters | Offset bin packing |

Para cada benchmark existen archivos fuente equivalentes en `benchmarks_cnn/` (C--) y `benchmarks_c/` (C), asegurando una comparación justa.

### Resultados Relevantes

#### Tiempos de Compilación (mediana en segundos)

| Benchmark | C-- Codegen | C-- Full | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|---|---|---|---|---|---|---|
| bench_fib | **0.0043** | 0.0452 | 0.0509 | 0.0953 | 0.1003 | 0.1254 |
| bench_matmul | **0.0028** | 0.0296 | 0.0519 | 0.0723 | 0.0748 | 0.1214 |
| bench_float | **0.0026** | 0.0294 | 0.0516 | 0.0560 | 0.0698 | 0.0734 |
| bench_prime | **0.0032** | 0.0289 | 0.0508 | 0.0654 | 0.0752 | 0.0873 |

C-- es **~15–40× más rápido** que GCC/Clang en compilación.

#### Tiempos de Ejecución (mediana en segundos)

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|---|---|---|---|---|---|
| bench_fib | 0.1295 | 0.1006 | 0.0248 | 0.0900 | **0.0502** |
| bench_matmul | 0.0072 | 0.0037 | **0.0012** | 0.0031 | 0.0014 |
| bench_float | 0.0140 | 0.0045 | 0.0019 | 0.0077 | **0.0017** |
| bench_prime | 0.0069 | 0.0050 | 0.0062 | 0.0065 | **0.0053** |

- **C-- es ~1.3–5.9× más lento** que GCC -O2 en ejecución.
- La brecha más grande está en `bench_float` (8.4×, Clang -O2 vectoriza con SSE/AVX).
- La brecha más pequeña está en `bench_prime` (1.12×, limitado por el algoritmo, no por el compilador).

#### Tamaño de Binarios (bytes en disco)

| Benchmark | C-- | GCC -O0 | GCC -O2 | Clang -O0 | Clang -O2 |
|---|---|---|---|---|---|
| bench_fib | 16112 | **16000** | **16000** | 16016 | 16016 |
| bench_matmul | 16288 | 16032 | 16088 | 16048 | 16104 |
| bench_float | 16056 | **15976** | **15976** | 15992 | 15992 |
| bench_prime | 16240 | **15976** | **15976** | 15992 | 15992 |

Los binarios de C-- son comparables a GCC/Clang (~100–300 bytes de diferencia, atribuibles a secciones fijas de runtime/printf).

### Análisis

**Fortalezas de C--:**
- Compilación extremadamente rápida (2.6–4.3 ms para parse + typecheck + codegen).
- Tamaño de binarios competitivo (diferencia marginal con GCC/Clang).
- Código generado claro y didáctico (útil para enseñanza).

**Debilidades de C-- frente a GCC/Clang:**
- Sin asignación de registros (todo viaja por el stack).
- Sin vectorización SIMD (especialmente crítico en código float).
- Sin desenrollamiento de loops, eliminación de código muerto ni inlineo.
- `bench_fib` se ve especialmente afectado por la ausencia de optimización en llamadas recursivas.

**Conclusión:** C-- prioriza simplicidad y velocidad de compilación sobre optimización de ejecución, comportándose como un compilador didáctico ideal para entender el pipeline de compilación, pero sin competir con GCC/Clang en rendimiento de código generado (factor esperado de 4–5× en el estado actual).
