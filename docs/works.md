# Checklist de Tareas — Proyecto Compilador x86

> Marcar `[x]` al completar cada tarea. El checklist es atómico: cada ítem representa una unidad de trabajo verificable.

---

## 1. Selección y definición formal del lenguaje

- [-] 1.1 Seleccionar un lenguaje de programación con especificación formal y documentación clara.
- [-] 1.2 Redactar documento de especificación del lenguaje (gramática, sintaxis, semántica).

### 1.3 Tipos de datos básicos y definidos por el usuario

- [-] 1.3.1 Definir tipos de datos básicos (int, float, bool, char, etc.).
- [-] 1.3.2 Definir alias de tipos (typedef / type alias).
- [x] 1.3.4 Implementar lexer para tokens de tipos básicos.
- [x] 1.3.5 Implementar parser para declaraciones de tipos definidos por el usuario.
- [x] 1.3.6 Implementar verificación semántica de tipos definidos por el usuario.

### 1.4 Variables y manejo de alcance (scope)

- [ ] 1.4.1 Definir reglas de declaración de variables.
- [ ] 1.4.2 Definir reglas de alcance (global, local, bloque).
- [x] 1.4.3 Implementar lexer para declaraciones de variables.
- [x] 1.4.4 Implementar parser para declaraciones y asignaciones de variables.
- [ ] 1.4.5 Implementar tabla de símbolos con soporte de scopes anidados.
- [ ] 1.4.6 Implementar verificación semántica de variables no declaradas y redeclaraciones.

### 1.5 Funciones

- [ ] 1.5.1 Definir sintaxis de declaración y llamada a funciones.
- [ ] 1.5.2 Definir reglas de paso de parámetros y retorno.
- [x] 1.5.3 Implementar lexer para tokens de funciones.
- [x] 1.5.4 Implementar parser para declaraciones de funciones (parámetros, tipo de retorno, cuerpo).
- [x] 1.5.5 Implementar parser para llamadas a funciones.
- [ ] 1.5.6 Implementar verificación semántica de firmas, aridad y tipos de retorno.

### 1.6 Estructuras de control

- [ ] 1.6.1 Definir `if` / `else`.
- [ ] 1.6.2 Definir `while`.
- [ ] 1.6.3 Definir `for` (o `do-while` si aplica).
- [ ] 1.6.4 Definir `switch` / `case` (si aplica).
- [x] 1.6.5 Implementar lexer para palabras clave de control de flujo.
- [x] 1.6.6 Implementar parser para `if` / `else`.
- [x] 1.6.7 Implementar parser para `while`.
- [x] 1.6.8 Implementar parser para `for`.
- [ ] 1.6.9 Implementar verificación semántica de condiciones (deben ser booleanas).

### 1.7 Structs, arreglos y cadenas de caracteres (strings)

- [ ] 1.7.1 Definir sintaxis de structs (declaración, inicialización, acceso a miembros).
- [ ] 1.7.2 Definir sintaxis de arreglos unidimensionales (declaración, indexación).
- [ ] 1.7.3 Definir sintaxis de strings (literales, operaciones básicas).
- [x] 1.7.4 Implementar lexer para tokens de structs, arreglos y strings.
- [x] 1.7.5 Implementar parser para declaraciones de structs.
- [x] 1.7.6 Implementar parser para acceso a miembros de structs.
- [x] 1.7.7 Implementar parser para declaración e indexación de arreglos.
- [x] 1.7.8 Implementar parser para literales de string.
- [ ] 1.7.9 Implementar verificación semántica de structs (miembros válidos, anidamiento).
- [ ] 1.7.10 Implementar verificación semántica de arreglos (índices enteros, límites).

### 1.8 Punteros, direccionamiento de memoria y manejo de memoria dinámica

- [ ] 1.8.1 Definir sintaxis de punteros (declaración, desreferencia, referencia).
- [ ] 1.8.2 Definir operaciones de manejo de memoria dinámica (alloc / free o similar).
- [x] 1.8.3 Implementar lexer para tokens de punteros y memoria dinámica.
- [x] 1.8.4 Implementar parser para declaraciones de punteros.
- [x] 1.8.5 Implementar parser para operaciones de desreferencia y referencia (`*`, `&`).
- [x] 1.8.6 Implementar parser para operaciones de memoria dinámica.
- [ ] 1.8.7 Implementar verificación semántica de tipos de punteros.
- [ ] 1.8.8 Implementar verificación semántica de liberación de memoria no alocada.

### 1.9 Tipos genéricos y plantillas (templates)

- [ ] 1.9.1 Definir sintaxis de genéricos/templates.
- [x] 1.9.2 Implementar lexer para tokens de genéricos.
- [x] 1.9.3 Implementar parser para declaraciones genéricas.
- [x] 1.9.4 Implementar parser para instanciación de genéricos.
- [ ] 1.9.5 Implementar verificación semántica de parámetros de tipo.
- [ ] 1.9.6 Implementar generación de código para templates instanciados.

### 1.10 Inferencia, conversión y promoción automática de tipos

- [ ] 1.10.1 Definir reglas de inferencia de tipos (var / auto o similar).
- [ ] 1.10.2 Definir reglas de conversión implícita entre tipos.
- [ ] 1.10.3 Definir reglas de conversión explícita (casting).
- [ ] 1.10.4 Definir reglas de promoción automática en expresiones.
- [x] 1.10.5 Implementar lexer para tokens de inferencia y casting.
- [x] 1.10.6 Implementar parser para declaraciones con inferencia de tipos.
- [x] 1.10.7 Implementar parser para expresiones de casting explícito.
- [ ] 1.10.8 Implementar verificación semántica para inferencia de tipos.
- [ ] 1.10.9 Implementar verificación semántica para conversiones válidas.
- [ ] 1.10.10 Implementar promoción automática en el análisis semántico de expresiones.

### 1.11 Arreglos multidimensionales y funciones lambda

- [ ] 1.11.1 Definir sintaxis de arreglos multidimensionales.
- [ ] 1.11.2 Definir sintaxis de funciones lambda (expresiones lambda).
- [x] 1.11.3 Implementar lexer para tokens de arreglos multidimensionales y lambdas.
- [x] 1.11.4 Implementar parser para declaraciones de arreglos multidimensionales.
- [x] 1.11.5 Implementar parser para indexación de arreglos multidimensionales.
- [x] 1.11.6 Implementar parser para expresiones lambda.
- [ ] 1.11.7 Implementar verificación semántica de dimensiones de arreglos.
- [ ] 1.11.8 Implementar verificación semántica de tipos en lambdas.
- [ ] 1.11.9 Implementar captura de entorno (closures) para lambdas.

---

## 2. Análisis léxico (Lexer)

- [x] 2.1 Definir el conjunto completo de tokens del lenguaje.
- [x] 2.2 Implementar el lexer (escáner) para todos los tokens.
- [x] 2.3 Implementar manejo de errores léxicos (caracteres no reconocidos, strings no cerrados, etc.).
- [x] 2.4 Implementar conteo de líneas y columnas para reporte de errores.
- [x] 2.5 Escribir pruebas unitarias para el lexer.

---

## 3. Análisis sintáctico (Parser) y AST

- [x] 3.1 Definir la gramática formal del lenguaje (EBNF o similar).
- [x] 3.2 Elegir estrategia de parsing (recursive descent, LL, LR, etc.).
- [x] 3.3 Diseñar la estructura de nodos del AST.
- [x] 3.4 Implementar el parser para el programa completo (entry point).
- [x] 3.5 Implementar el parser para todas las expresiones (aritméticas, lógicas, relacionales).
- [x] 3.6 Implementar el parser para todas las declaraciones (variables, funciones, tipos).
- [x] 3.7 Implementar el parser para todas las sentencias (control de flujo, asignación, etc.).
- [x] 3.8 Implementar construcción del AST durante el parsing.
- [x] 3.9 Implementar manejo de errores sintácticos (recuperación o reporte claro).
- [x] 3.10 Escribir pruebas unitarias para el parser.

---

## 4. Tabla de símbolos

- [ ] 4.1 Diseñar la estructura de la tabla de símbolos con scopes jerárquicos.
- [ ] 4.2 Implementar inserción de símbolos (variables, funciones, tipos, structs).
- [ ] 4.3 Implementar búsqueda de símbolos respetando reglas de scope.
- [ ] 4.4 Implementar soporte para scope global, de función y de bloque.
- [ ] 4.5 Implementar almacenamiento de metadatos (tipo, offset, scope, etc.) en cada símbolo.

---

## 5. Análisis semántico

- [ ] 5.1 Implementar verificación de tipos en expresiones aritméticas.
- [ ] 5.2 Implementar verificación de tipos en expresiones lógicas y relacionales.
- [ ] 5.3 Implementar verificación de tipos en asignaciones.
- [ ] 5.4 Implementar verificación de tipos en llamadas a funciones.
- [ ] 5.5 Implementar verificación de tipos en retorno de funciones.
- [ ] 5.6 Implementar verificación de tipos en operaciones con punteros.
- [ ] 5.7 Implementar verificación de tipos en operaciones con structs.
- [ ] 5.8 Implementar verificación de tipos en operaciones con arreglos.
- [ ] 5.9 Implementar verificación de tipos en genéricos/templates.
- [ ] 5.10 Implementar verificación de conversiones y promociones de tipos.
- [ ] 5.11 Implementar regla de variable declarada antes de uso.
- [ ] 5.12 Implementar regla de función declarada antes de llamado.
- [ ] 5.13 Implementar regla de break/continue dentro de ciclos.
- [ ] 5.14 Implementar regla de retorno en todos los caminos de función no void.
- [ ] 5.15 Implementar reporte de errores semánticos con ubicación en código fuente.
- [ ] 5.16 Escribir pruebas unitarias para el análisis semántico.

---

## 6. Generación de código ensamblador x86

- [ ] 6.1 Seleccionar sintaxis de ensamblador (AT&T vs Intel) y assembler (NASM, GAS, MASM).
- [ ] 6.2 Diseñar la estrategia de generación de código (recorrido del AST con visitor o similar).
- [ ] 6.3 Implementar generación de prólogo y epílogo de programa (entry point, exit).
- [ ] 6.4 Implementar generación de código para funciones (setup de stack frame, retorno).
- [ ] 6.5 Implementar generación de código para llamadas a funciones (convención de llamada).
- [ ] 6.6 Implementar generación de código para paso de parámetros.
- [ ] 6.7 Implementar generación de código para declaraciones y acceso a variables locales.
- [ ] 6.8 Implementar generación de código para variables globales.
- [ ] 6.9 Implementar generación de código para expresiones aritméticas.
- [ ] 6.10 Implementar generación de código para expresiones lógicas y relacionales.
- [ ] 6.11 Implementar generación de código para estructuras de control (`if`/`else`).
- [ ] 6.12 Implementar generación de código para `while`.
- [ ] 6.13 Implementar generación de código para `for`.
- [ ] 6.14 Implementar generación de código para punteros (desreferencia, referencia).
- [ ] 6.15 Implementar generación de código para manejo de memoria dinámica.
- [ ] 6.16 Implementar generación de código para structs (acceso a miembros, alineación).
- [ ] 6.17 Implementar generación de código para arreglos unidimensionales.
- [ ] 6.18 Implementar generación de código para arreglos multidimensionales.
- [ ] 6.19 Implementar generación de código para strings.
- [ ] 6.20 Implementar generación de código para lambdas.
- [ ] 6.21 Implementar generación de código para templates/genéricos.
- [ ] 6.22 Implementar generación de código para promoción y conversión de tipos.
- [ ] 6.23 Escribir pruebas unitarias para el generador de código.
- [ ] 6.24 Verificar que el código ensamblador generado compila y ejecuta correctamente.

---

## 7. Optimización de código

- [ ] 7.1 Identificar y documentar las técnicas de optimización a implementar.
- [ ] 7.2 Implementar plegado de constantes (constant folding).
- [ ] 7.3 Implementar propagación de constantes (constant propagation).
- [ ] 7.4 Implementar eliminación de código muerto.
- [ ] 7.5 Implementar eliminación de expresiones comunes (common subexpression elimination).
- [ ] 7.6 Implementar optimización de accesos a memoria (registros en lugar de stack cuando sea posible).
- [ ] 7.7 Implementar al menos una optimización a nivel de ensamblador (peephole).
- [ ] 7.8 Escribir pruebas que demuestren las mejoras de cada optimización.
- [ ] 7.9 Documentar resultados de optimización (tamaño de código, velocidad).

---

## 8. Benchmarks

- [ ] 8.1 Diseñar conjunto de programas de prueba representativos (diferentes características del lenguaje).
- [ ] 8.2 Implementar programa benchmark: cómputo intensivo (ej: fibonacci, factorial).
- [ ] 8.3 Implementar programa benchmark: manejo de memoria dinámica.
- [ ] 8.4 Implementar programa benchmark: recursión y llamadas a funciones.
- [ ] 8.5 Implementar programa benchmark: estructuras de datos (structs, arreglos).
- [ ] 8.6 Implementar programa benchmark: operaciones con punteros.
- [ ] 8.7 Implementar programa benchmark: templates/genéricos.
- [ ] 8.8 Implementar programa benchmark: lambdas.
- [ ] 8.9 Automatizar ejecución de benchmarks (script de medición de tiempos).
- [ ] 8.10 Medir y registrar tiempos de ejecución y tamaño de binario para cada benchmark.

---

## 9. Comparación con compiladores comerciales

- [ ] 9.1 Definir métricas de comparación (tiempo de ejecución, tamaño de binario, tiempo de compilación).
- [ ] 9.2 Traducir los benchmarks del lenguaje propio a C (o C++, Rust, Go).
- [ ] 9.3 Compilar y ejecutar benchmarks con GCC.
- [ ] 9.4 Compilar y ejecutar benchmarks con Clang/LLVM.
- [ ] 9.5 Compilar y ejecutar benchmarks con al menos un compilador adicional (Rustc, Go, MSVC).
- [ ] 9.6 Recolectar métricas para cada compilador.
- [ ] 9.7 Elaborar tablas comparativas con los resultados.
- [ ] 9.8 Elaborar gráficos comparativos (barras, líneas) con los resultados.
- [ ] 9.9 Redactar análisis y discusión técnica de los resultados.

---

## 10. Documentación técnica (Reporte)

- [ ] 10.1 Redactar introducción y objetivos del proyecto.
- [ ] 10.2 Documentar la especificación formal del lenguaje (gramática, tipos, características).
- [ ] 10.3 Documentar la arquitectura general del compilador (diagrama de componentes).
- [ ] 10.4 Documentar el diseño del lexer (autómata, tokens, manejo de errores).
- [ ] 10.5 Documentar el diseño del parser (estrategia, gramática, AST).
- [ ] 10.6 Documentar la tabla de símbolos (estructura, scopes, operaciones).
- [ ] 10.7 Documentar el análisis semántico (reglas de verificación de tipos).
- [ ] 10.8 Documentar la generación de código x86 (convenciones, estrategias por constructo).
- [ ] 10.9 Documentar las técnicas de optimización implementadas.
- [ ] 10.10 Documentar la metodología y resultados de los benchmarks.
- [ ] 10.11 Documentar la metodología y resultados de la comparación comercial.
- [ ] 10.12 Documentar las decisiones de diseño y justificación técnica.
- [ ] 10.13 Documentar las tecnologías y herramientas utilizadas.
- [ ] 10.14 Incluir conclusiones y trabajos futuros.
- [ ] 10.15 Incluir referencias bibliográficas.

---

## 11. Exposición técnica y defensa

- [ ] 11.1 Preparar presentación (slides) con la estructura del proyecto.
- [ ] 11.2 Incluir demostración en vivo del compilador.
- [ ] 11.3 Demostrar compilación y ejecución de al menos un programa representativo.
- [ ] 11.4 Preparar explicación de cada fase del compilador (léxico, sintáctico, semántico, generación).
- [ ] 11.5 Preparar explicación de optimizaciones implementadas.
- [ ] 11.6 Preparar discusión de resultados de comparación comercial.
- [ ] 11.7 Ensayar la presentación en grupo.
- [ ] 11.8 Verificar que todos los integrantes pueden explicar cualquier parte del código.

---

## 12. Aplicación funcional (Puntos extra: +3)

### 12.1 Editor de código

- [ ] 12.1.1 Diseñar la interfaz del editor de código.
- [ ] 12.1.2 Implementar resaltado de sintaxis para el lenguaje diseñado.
- [ ] 12.1.3 Implementar numeración de líneas.
- [ ] 12.1.4 Implementar funcionalidad de abrir/guardar archivos de código fuente.

### 12.2 Visualización del AST

- [ ] 12.2.1 Diseñar la representación visual del AST.
- [ ] 12.2.2 Implementar generación gráfica del AST (árbol o grafo) a partir del parser.
- [ ] 12.2.3 Implementar navegación interactiva del AST (expandir/colapsar nodos).

### 12.3 Generación de código ensamblador x86

- [ ] 12.3.1 Integrar el generador de código en la aplicación.
- [ ] 12.3.2 Mostrar el código ensamblador generado en un panel de la UI.

### 12.4 Ejecución o simulación del programa compilado

- [ ] 12.4.1 Integrar el assembler y linker (NASM, GAS + ld) para producir un ejecutable.
- [ ] 12.4.2 Implementar botón de "Compilar y Ejecutar" en la UI.
- [ ] 12.4.3 Capturar el código de salida del proceso ejecutado.

### 12.5 Visualización de resultados de ejecución

- [ ] 12.5.1 Implementar panel de salida estándar (stdout) del programa ejecutado.
- [ ] 12.5.2 Implementar panel de errores de compilación/ejecución.
- [ ] 12.5.3 Implementar medición de tiempo de ejecución mostrada en la UI.

---

## 13. Participación del equipo

- [ ] 13.1 Definir distribución de tareas entre integrantes por escrito.
- [ ] 13.2 Mantener control de versiones con contribuciones individuales verificables (commits).
- [ ] 13.3 Realizar reuniones periódicas de seguimiento.
- [ ] 13.4 Cada integrante debe tener contribuciones en al menos 2 fases del compilador.
- [ ] 13.5 Verificar que cada integrante entiende y puede explicar todas las fases.

---

## 14. Entrega final

- [ ] 14.1 Verificar que el código fuente del compilador está completo y funcional.
- [ ] 14.2 Verificar que todos los tests pasan exitosamente.
- [ ] 14.3 Verificar que el reporte técnico está completo y bien formateado.
- [ ] 14.4 Verificar que los benchmarks son reproducibles.
- [ ] 14.5 Verificar que la presentación está lista.
- [ ] 14.6 Entregar en la plataforma/fecha indicada por el docente.
