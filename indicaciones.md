## Compiladores

## 2026-

# Proyecto 2

## Indicaciones:

## Los estudiantes deberán trabajar en grupos y desarrollar uno de los proyectos propuestos

## en el curso.

## Cada proyecto deberá ser implementado de manera funcional, evidenciando la correcta apli-

## cación de los conceptos de compiladores y lenguajes de programación desarrollados durante

## el curso.

## El desarrollo deberá incluir documentación técnica clara y estructurada, describiendo la

## arquitectura del sistema, las decisiones de diseño, las tecnologías utilizadas y los principales

## componentes implementados.

## Cada grupo deberá realizar una exposición técnica del proyecto, demostrando el funciona-

## miento del sistema y explicando adecuadamente los componentes desarrollados, las decisio-

## nes de implementación y los resultados obtenidos.

## Se recomienda el uso de herramientas modernas y buenas prácticas de ingeniería de software

## para la organización, implementación y documentación del proyecto.

## Todos los integrantes del equipo deberán participar activamente en el desarrollo del proyecto,

## demostrando contribuciones reales, verificables y consistentes con el trabajo presentado.

## Todos los integrantes del grupo deberán demostrar capacidad de análisis, investigación,

## implementación y comprensión profunda de los conceptos de compiladores y lenguajes de

## programación involucrados en el proyecto.

## El uso de herramientas de inteligencia artificial estará permitido únicamente como apoyo

## complementario durante el desarrollo del proyecto. Sin embargo, el trabajo deberá reflejar

## comprensión técnica real por parte de todos los miembros del grupo. En caso de evidenciarse

## dependencia excesiva de herramientas de IA, incapacidad para explicar el funcionamiento

## del código presentado o desconocimiento de los componentes implementados por parte de

## uno o más integrantes, el proyecto será calificado con cero (0).

## Los grupos deberán cumplir con los objetivos mínimos establecidos en el desarrollo del

## proyecto. En caso de no alcanzarlos, el proyecto será calificado con cero (0).

## En caso de detectarse que uno o más integrantes no han participado de manera efectiva en

## el desarrollo del trabajo, el proyecto será calificado con cero (0).


#### Compiladores

#### 2026-

## Estudio de Runtimes y Backends de Compilación Modernos con Wasmtime y

## Cranelift

### El presente proyecto tiene como objetivo comprender los fundamentos de los runtimes y backends

### de compilación modernos mediante el estudio del ecosistema de Wasmtime y Cranelift, explo-

### rando tecnologías actuales utilizadas en sistemas de ejecución basados en WebAssembly y en

### procesos modernos de generación de código. A lo largo del desarrollo del proyecto, los estudian-

### tes analizarán la arquitectura y funcionamiento interno de herramientas utilizadas en entornos

### reales de compilación, familiarizándose además con dinámicas y metodologías propias de comu-

### nidades open source vinculadas al desarrollo de compiladores, runtimes y máquinas virtuales

### modernas. Los estudiantes deberán:

### Comprender la arquitectura general de WebAssembly, Wasmtime y Cranelift.

### Analizar el funcionamiento de runtimes modernos y sistemas de generación de código JI-

### T/AOT.

### Estudiar el flujo completo de compilación y ejecución de programas basados en WebAs-

### sembly.

### Explorar repositorios, documentación técnica, issues y herramientas del ecosistema open

### source.

### Familiarizarse con prácticas de desarrollo colaborativo utilizadas en proyectos modernos de

### compiladores y runtimes.

### Investigar componentes internos relacionados con representación intermedia, optimización

### y generación de código.

### Desarrollar capacidades de investigación, análisis técnico y lectura de código fuente de gran

### escala.

### Elaborar un reporte técnico que documente los conceptos estudiados, experimentos reali-

### zados y hallazgos obtenidos.

### Comunicar de manera técnica y estructurada los resultados y aprendizajes del proyecto

### mediante una exposición final.

### El máximo logro del proyecto consistirá en que el grupo realice una contribución significativa

### dentro del ecosistema de Wasmtime, Cranelift o proyectos relacionados, que sea reconocida,

### aceptada o incorporada por la comunidad open source correspondiente. En este sentido, aquellos

### grupos que logren una contribución validada podrán obtener hasta 3 puntos adicionales en el

### Examen final del curso. Se considerarán como aportes válidos:

### Pull Requests aceptados: Cambios incorporados oficialmente al proyecto por los mante-

### nedores. Incluye corrección de código, mejoras pequeñas, refactorización, documentación

### integrada, nuevos ejemplos y mejoras de tooling.

### Corrección de bugs: Identificación y solución de errores existentes en el proyecto. Incluye

### errores de compilación, fallos en ejecución, problemas de compatibilidad, errores en tests y

### comportamiento incorrecto del runtime o backend.


#### Compiladores

#### 2026-

### Mejoras de documentación: Contribuciones orientadas a mejorar la comprensión del pro-

### yecto. Incluye tutoriales, corrección de documentación, ejemplos explicativos, guías de ins-

### talación, diagramas de arquitectura y aclaración de APIs.

### Tooling: Desarrollo de herramientas auxiliares para el ecosistema. Incluye scripts, herra-

### mientas CLI, analizadores, visualizadores, automatización de pruebas, debugging tools e

### integración con editores.

### Mejoras técnicas: Optimización o mejora de componentes existentes. Incluye mejoras de

### rendimiento, simplificación de código, optimización interna y reducción de consumo de

### recursos.

### Issues relevantes reconocidos por mantenedores: Reportes o análisis técnicos útiles para

### la comunidad. Incluye detección de errores reproducibles, análisis técnico, propuestas de

### mejora y reportes bien documentados.

### Participación técnica significativa en discusiones: Intervenciones técnicas relevantes den-

### tro de la comunidad. Incluye discusión de arquitectura, revisión de propuestas, ayuda en

### resolución de errores y aportes técnicos en foros oficiales.

### La rúbrica de calificación del proyecto se basa en los siguientes criterios:

```
Criterio Excelente Bueno Regular Deficiente Pts
```
```
Comprensión
Wasm
```
```
Explica claramen-
te la arquitectura
y relación entre
runtime y WebAs-
sembly.
```
```
Comprende ade-
cuadamente los
conceptos con pe-
queñas imprecisio-
nes.
```
```
Presenta com-
prensión parcial o
superficial.
```
```
No demuestra com-
prensión técnica.
```
##### 5

```
Comunidad Open
Source
```
```
Participación acti-
va mediante revi-
sión de repositorios
y discusiones.
```
```
Explora adecuada-
mente repositorios
y documentación
principal.
```
```
Exploración limita-
da de la comunidad
y sus herramientas.
```
```
No evidencia in-
teracción con la
comunidad.
```
##### 6

```
Análisis Arquitec-
tura
```
```
Analiza correcta-
mente componentes
y flujo de compila-
ción.
```
```
Presenta un análi-
sis adecuado aun-
que limitado en
profundidad.
```
```
El análisis es su-
perficial o incom-
pleto.
```
```
No presenta análi-
sis técnico relevan-
te.
```
##### 4

```
Reporte Técnico
```
```
Reporte bien es-
tructurado, claro y
con buenas referen-
cias.
```
```
Reporte organizado
con algunos proble-
mas menores.
```
```
Reporte limitado,
poco claro o incom-
pleto.
```
```
Reporte desordena-
do o insuficiente.
```
##### 2

```
Exposición
```
```
Expone con cla-
ridad, dominio
conceptual y buena
organización.
```
```
Presenta adecuada-
mente con algunas
dificultades meno-
res.
```
```
Presentación poco
clara o con vacíos.
```
```
No logra comunicar
el trabajo.
```
##### 3


#### Compiladores

#### 2026-

## Implementación de Lenguajes Extensibles mediante Adaptable Parsing Ex-

## pression Grammars en Haskell

### El presente proyecto tiene como objetivo comprender los fundamentos de las Parsing Expression

### Grammars (PEG) y de las Adaptable Parsing Expression Grammars (APEG), analizando su uso

### como base formal para el diseño de lenguajes extensibles y sistemas de parsing modernos. A lo

### largo del desarrollo del proyecto, los estudiantes estudiarán mecanismos de extensión dinámica

### de gramáticas, así como el modelo formal propuesto en la literatura especializada, con énfasis en

### su implementación práctica. Se abordará además la construcción de parsers basados en combi-

### nadores, así como el uso de estructuras funcionales avanzadas propias del lenguaje Haskell. Los

### estudiantes deberán:

### Comprender los fundamentos de las Parsing Expression Grammars (PEG) y las Adaptable

### Parsing Expression Grammars (APEG).

### Analizar mecanismos de extensión dinámica de gramáticas y lenguajes de programación.

### Estudiar el modelo formal y la implementación presentada en la literatura especializada

### (paper base del curso).

### Comprender el uso de parsing combinators y State Monads en Haskell.

### Analizar la construcción dinámica de árboles de sintaxis abstracta (AST) y reglas grama-

### ticales.

### Implementar componentes de un lenguaje extensible utilizando Haskell.

### Explorar técnicas modernas de metaprogramación y language-oriented programming.

### Desarrollar capacidades de investigación, lectura de papers e implementación de sistemas

### de compilación modernos.

### Elaborar un reporte técnico que documente el sistema desarrollado y el análisis realizado.

### Comunicar de manera técnica y estructurada los resultados y aprendizajes obtenidos me-

### diante una exposición final.

### El máximo logro del proyecto consistirá en el desarrollo de una extensión para Visual Studio Code

### orientada a lenguajes extensibles, incorporando funcionalidades avanzadas de análisis, asistencia

### de código y visualización del lenguaje. En este sentido, los grupos que logren una implementar una

### extensión podrán obtener hasta 3 puntos adicionales en el Examen final del curos. Se considerarán

### extensiones válidas:

### Syntax Highlighting: Implementación de resaltado sintáctico para un DSL, incluyendo co-

### loreado de keywords, operadores, comentarios, tipos y bloques.

### Parser integrado: Desarrollo de un parser funcional dentro de la extensión, con detección

### de errores sintácticos, validación de estructuras y análisis de tokens.

### Autocompletado: Implementación de sugerencias automáticas en el editor, incluyendo key-

### words, funciones, variables, tipos y snippets inteligentes.

### AST Viewer: Visualización de árboles sintácticos generados por el parser, de forma gráfica

### o estructurada.


#### Compiladores

#### 2026-

### Semantic Analysis: Implementación de validaciones semánticas, como variables no declara-

### das, incompatibilidad de tipos, redefiniciones o validación de unidades.

### La rúbrica de calificación del proyecto se basa en los siguientes criterios:

```
Criterio Excelente Bueno Regular Deficiente Pts
```
```
Comprensión PE-
G/APEG
```
```
Explica claramente
los fundamentos
de PEG, APEG y
extensibilidad de
gramáticas.
```
```
Comprende ade-
cuadamente los
conceptos princi-
pales con pequeñas
imprecisiones.
```
```
Presenta com-
prensión parcial o
superficial de los
conceptos.
```
```
No demuestra com-
prensión suficiente
de PEG y APEG.
```
##### 5

```
Análisis del Paper
```
```
Analiza correcta-
mente el modelo
formal, parsing
combinators, mo-
nads y arquitectura
propuesta.
```
```
Presenta un análi-
sis adecuado aun-
que limitado en
profundidad.
```
```
El análisis es su-
perficial o incom-
pleto.
```
```
No presenta análi-
sis técnico relevan-
te del paper.
```
##### 5

```
Implementación
Técnica
```
```
Implementa correc-
tamente compo-
nentes funcionales
del parser, DSL
o mecanismos de
extensibilidad.
```
```
Implementación
funcional con al-
gunos problemas
menores.
```
```
Implementación
incompleta o limi-
tada.
```
```
No logra implemen-
tar componentes
funcionales.
```
##### 5

```
Reporte Técnico
```
```
Reporte bien es-
tructurado, técnico,
claro y con análisis
profundo.
```
```
Reporte organizado
con algunos pro-
blemas menores de
claridad o profun-
didad.
```
```
Reporte limitado,
poco claro o incom-
pleto.
```
```
Reporte desordena-
do o insuficiente.^2
```
```
Exposición
```
```
Expone con cla-
ridad, dominio
conceptual y buena
organización técni-
ca.
```
```
Presenta adecuada-
mente el tema con
algunas dificultades
menores.
```
```
Presentación poco
clara o con vacíos
conceptuales.
```
```
No logra comunicar
adecuadamente el
trabajo realizado.
```
##### 3


#### Compiladores

#### 2026-

## Diseño e Implementación de un Compilador x86 con Optimización y Evalua-

## ción Comparativa

### El objetivo del presente proyecto es diseñar e implementar un compilador completo para un len-

### guaje de programación sobre arquitectura x86, integrando las etapas fundamentales del proceso

### de compilación, desde el análisis léxico y sintáctico hasta la generación de código ensamblador

### y la optimización básica. Finalmente, el proyecto tiene como propósito fortalecer la capacidad

### de análisis, diseño e implementación de sistemas de software complejos, así como promover la

### evaluación experimental mediante la comparación del compilador desarrollado con herramientas

### de uso extendido en la industria. Los estudiantes deberán:

### Seleccionar un lenguaje de programación con características bien definidas y documentación

### formal. Este debe incluir, como mínimo, las siguientes características:

- Básicas:

### ◦ Tipos de datos básicos y definidos por el usuario.

### ◦ Variables y manejo de alcance (scope).

### ◦ Funciones.

### ◦ Estructuras de control

### ◦ Struct, arreglos y cadenas de caracteres (strings).

- Avanzadas:

### ◦ Punteros, direccionamiento de memoria y manejo de memoria dinámica.

### ◦ Tipos genéricos y plantillas (templates).

### ◦ Inferencia, conversión y promoción automática de tipos.

### ◦ Arreglos multidimensionales y funciones lambda.

### Implementar un compilador completo que incluya las fases de análisis léxico, sintáctico y

### semántico.

### Implementar un sistema de verificación de tipos y manejo de errores léxicos, sintácticos y

### semánticos.

### Generar código ensamblador para arquitectura x86.

### Aplicar técnicas básicas de optimización sobre el código generado.

### Desarrollar un conjunto de benchmarks para evaluar el rendimiento del compilador.

### Realizar una comparación experimental con compiladores de uso extendido como GCC,

### Clang/LLVM, MSVC, Rustc o Go Compiler.

### Analizar y documentar los resultados obtenidos mediante tablas, gráficos y discusión téc-

### nica.

### Presentar y defender el proyecto mediante una exposición técnica.

### El máximo logro del proyecto consistirá en la implementación de una aplicación funcional del

### compilador desarrollado que permita demostrar, de manera integrada, las distintas fases del

### proceso de compilación y las principales características del lenguaje implementado. Los grupos

### que logren desarrollar dicha aplicación podrán obtener hasta 3 puntos adicionales en la evaluación

### final del curso. La aplicación deberá incluir, como mínimo, los siguientes componentes:


#### Compiladores

#### 2026-

### Editor de código para el lenguaje diseñado.

### Visualización del AST (Abstract Syntax Tree).

### Generación de código ensamblador x86.

### Ejecución o simulación del programa compilado.

### Visualización de resultados de ejecución.

### La rúbrica de calificación del proyecto se basa en los siguientes criterios:

```
Criterio Excelente Bueno Regular Deficiente Pts
```
```
Diseño y Lexer
```
```
Lenguaje consisten-
te y lexer robusto
con manejo adecua-
do de errores.
```
```
Diseño adecuado y
lexer funcional con
pocos errores.
```
```
Diseño limitado
o lexer con fallos
ocasionales.
```
```
Diseño incompleto
o lexer incorrecto.^2
```
```
Sintaxis y AST
```
```
Parser correcto,
AST bien estruc-
turado y tabla de
símbolos organiza-
da.
```
```
Implementación
funcional con leves
limitaciones.
```
```
Implementación
parcial o poco
organizada.
```
```
Implementación
incorrecta o incom-
pleta.
```
##### 2

```
Análisis Semántico
Verificación sólida
de tipos y errores.
```
```
Buen manejo se-
mántico. Validación parcial. Escasa validación.^1
```
```
Generación x
```
```
Código eficiente,
correcto y optimi-
zado.
```
```
Código funcional
con leves limitacio-
nes.
```
```
Código poco efi-
ciente.
```
```
Código incorrecto o
incompleto.
```
##### 3

```
Características
Avanzadas
```
```
Implementa correc-
tamente la mayoría
de funcionalidades
avanzadas.
```
```
Implementa varias
funcionalidades
relevantes.
```
```
Implementación
parcial.
```
```
Muy pocas funcio-
nalidades imple-
mentadas.
```
##### 2

```
Optimización
```
```
Implementa opti-
mizaciones rele-
vantes y demuestra
mejoras medibles.
```
```
Implementa algu-
nas optimizaciones
funcionales.
```
```
Optimización míni-
ma.
```
```
No implementa
optimizaciones.
```
##### 3

```
Comparación Co-
mercial
```
```
Benchmark riguro-
so y análisis sólido
frente a compilado-
res comerciales.
```
```
Comparación ade-
cuada con métricas
parciales.
```
```
Comparación su-
perficial.
```
```
No realiza compa-
ración.
```
##### 2

```
Reporte Técnico
```
```
Documentación
técnica completa,
clara y bien estruc-
turada.
```
```
Documentación
adecuada con algu-
nos vacíos menores.
```
```
Documentación
parcial o poco
clara.
```
```
Documentación
insuficiente.
```
##### 2

```
Exposición
```
```
Presentación clara,
dominio técnico y
demostración sólida
del compilador.
```
```
Buena presenta-
ción y explicación
adecuada del fun-
cionamiento.
```
```
Presentación limi-
tada o con poco
dominio del proyec-
to.
```
```
Escaso dominio o
incapacidad de co-
municar el trabajo.
```
##### 3



