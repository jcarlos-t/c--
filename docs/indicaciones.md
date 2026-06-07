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



