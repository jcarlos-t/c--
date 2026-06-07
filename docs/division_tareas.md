# División de Tareas — Proyecto Compilador C--

Persona 1 -> Joseph
Persona 2 -> Juan
Persona 3 -> Paulo

## 👤 Persona 1: Frontend — Lexer, Parser y Aplicación

**Responsabilidad:** Transformar texto fuente a AST + construir la aplicación visual.

| Tarea | Descripción |
|---|---|
| **Lexer** | Implementar tokenización del código fuente: palabras clave, identificadores, literales (enteros, flotantes, chars, strings). Manejo de errores léxicos. |
| **Parser** | Construir el AST (recursive descent) a partir de los tokens según la gramática EBNF especificada. Manejo de errores sintácticos. |
| **AST Nodes** | Implementar todas las estructuras de nodos del AST (`ast.md`) y la función `freeNode` para liberación de memoria. |
| **Aplicación (bono)** | Desarrollar la interfaz funcional con: editor de código, visualización del AST, visualización del assembly generado, y ejecución con resultados. |

**Dependencias externas:** Ninguna. Puede trabajar con ASTs de prueba desde el día 1.
**Contrato de salida:** AST válido (`Program*`) y reportes de error léxico/sintáctico.

---

## 👤 Persona 2: Middle-end — Análisis Semántico y Tipos

**Responsabilidad:** Validar semánticamente el AST y gestionar símbolos.

| Tarea | Descripción |
|---|---|
| **Tabla de Símbolos** | Implementar scoping anidado con `unordered_map<string, SymbolInfo>`. Soporte para variables, funciones, structs y parámetros. |
| **Declaraciones** | Registrar funciones, variables, structs y templates. Verificar declaraciones duplicadas. |
| **Verificación de Tipos** | Recorrer el AST validando compatibilidad de tipos en asignaciones, expresiones binarias, llamadas a función, retornos, etc. |
| **Características Avanzadas** | Inferencia (`auto`), conversión/promoción automática, punteros, arreglos multidimensionales, plantillas (instanciación básica), lambdas. |
| **Errores Semánticos** | Reportar: variable no declarada, tipo incompatible, argumentos incorrectos, break/continue fuera de ciclo, etc. |

**Dependencias externas:** AST (`ast.md`). Puede usar mocks del AST mientras Persona 1 termina.
**Contrato de entrada:** AST (`Program*`).
**Contrato de salida:** AST semánticamente válido o lista de errores semánticos.

---

## 👤 Persona 3: Backend — Generación x86, Optimización y Benchmarks

**Responsabilidad:** Traducir el AST a assembly x86 y evaluar rendimiento.

| Tarea | Descripción |
|---|---|
| **Generación de Código x86** | Recorrer el AST validado y emitir instrucciones NASM/Intel. Manejar registro de asignación (`%eax`, `%ebx`, etc.), calling convention, y secciones `.data`, `.bss`, `.text`. |
| **Estructuras de Control** | Generar saltos condicionales (`jmp`, `je`, `jne`, etc.) para `if`, `while`, `for`, `switch`, `break`, `continue`. |
| **Funciones y Llamadas** | Prólogo/epílogo (`push %ebp`, `mov %esp, %ebp`), paso de argumentos, valor de retorno. |
| **Punteros y Memoria** | Instrucciones para `malloc`/`free` (via `sbrk` o syscalls), aritmética de punteros, `->`, `&`, `*`. |
| **Arreglos y Structs** | Cálculo de direcciones para subscripts multidimensionales y acceso a miembros. |
| **Optimización Básica** | Plegamiento de constantes, eliminación de código muerto, asignación básica de registros. |
| **Benchmarks** | Desarrollar suite de pruebas de rendimiento. Comparar contra GCC/Clang con tablas y gráficos. |
| **Integración (bono)** | Ensamblar y linkear el código generado, ejecutar el binario, capturar salida para mostrarla en la app. |

**Dependencias externas:** AST semánticamente válido. Puede usar ASTs de prueba manuales.
**Contrato de entrada:** AST validado (`Program*`).
**Contrato de salida:** Código assembly `.asm`, binario ejecutable, resultados de benchmarks.

---

## Cronograma (6 – 20 de junio)

### Lun 8 – Sáb 13: Implementación paralela

| Persona | Lun 8 – Mar 9 | Mié 10 – Jue 11 | Vie 12 – Sáb 13 |
|---|---|---|---|
| **1 (Frontend)** | Lexer + estructura de tokens | Parser recursive descent + construcción de AST | Pruebas del parser, manejo de errores sintácticos. Inicio de app (editor + visualización AST). |
| **2 (Semántico)** | Tabla de símbolos + scoping | Visitor de declaraciones (funciones, vars, structs) | Verificación de tipos en expresiones y statements. Inicio de avanzadas (auto, punteros). |
| **3 (Backend)** | Esqueleto del codegen + visitor base | Asignación de registros + expresiones aritméticas/lógicas | Control flow (if, while, for) + llamadas a función. Inicio de benchmarks. |

### Dom 14 – Lun 15: Integración
- **Dom 14:** Integrar Frontend → Middle-end. Depurar errores semánticos con casos reales del parser.
- **Lun 15:** Integrar Middle-end → Backend. Obtener primer assembly compilable desde código fuente real.

### Mar 16 – Mié 17: Cierre
- **Persona 1:** Terminar app (ensamblar + ejecutar desde la UI, mostrar resultados).
- **Persona 2:** Completar características avanzadas restantes (templates, lambdas, cast, malloc/free).
- **Persona 3:** Optimización básica + benchmarks formales contra GCC/Clang (tablas, gráficos).

### Jue 18 – Sáb 20: Documentación
- Cada uno redacta su sección del reporte técnico.
- Entrega final.

---

**Hito diario recomendado:** Cada día los 3 hacen un sync de 10 min al final para reportar progreso y bloques.
