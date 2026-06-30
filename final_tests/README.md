# Tests Finales

Este directorio contiene tests para verificar todas las características requeridas del compilador C--. **Los tests usan el código generado por GenCode (ensamblador x86-64), no el intérprete EVALVisitor.**

## Características Testeadas

- **test01_funciones.cnn**: Funciones, operaciones aritméticas y recursividad
- **test02_control_flujo.cnn**: Estructuras de control (do-while, for, if, break, continue)
- **test03_variables_ops.cnn**: Variables, operaciones aritméticas y condicionales
- **test04_operadores_unarios.cnn**: Operadores unarios (++/--) y lógicos
- **test05_tipos_char.cnn**: Tipo `char` y literales de carácter
- **test06_structs.cnn**: Structs y acceso a miembros
- **test07_arrays.cnn**: Arreglos unidimensionales
- **test08_malloc_free.cnn**: `malloc` y `free`
- **test09_switch.cnn**: `switch` / `case` / `default`
- **test10_expresiones_complejas.cnn**: Expresiones compuestas y condicionales
- **test11_pointers.cnn**: Punteros, desreferencia y punteros a struct
- **test12_sizeof.cnn**: Operador `sizeof`
- **test13_type_inference.cnn**: Inferencia de tipos con `auto`
- **test14_lambda.cnn**: Funciones lambda y captura de variables
- **test15_templates.cnn**: Plantillas (templates) de funciones y structs
- **test16_scope.cnn**: Alcance (scope) y shadowing de variables
- **test17_multidim_arrays.cnn**: Arreglos multidimensionales
- **test18_type_promotion.cnn**: Conversión y promoción automática de tipos
- **test19_strings.cnn**: Cadenas de caracteres y literales `char`
- **test20_float_double.cnn**: Aritmética con `float` y `double`

## Tests con fallos conocidos (bugs del compilador)

Algunos tests documentan comportamiento correcto que el compilador aún no implementa del todo:

| Test | Característica | Problema actual |
|------|----------------|-----------------|
| `test16_scope` | Shadowing de variables | El codegen no maneja scopes anidados en bloques |
| `test17_multidim_arrays` | Arreglos 2D | Indexación multidimensional genera código incorrecto |
| `test18_type_promotion` | Promoción `int`↔`float` | Comparaciones con literales float fallan en codegen |
| `test20_float_double` | Aritmética flotante | Operaciones y comparaciones float usan instrucciones enteras |

Estos tests sirven como regresión: deben pasar cuando se corrijan las fases correspondientes.

## Cómo Ejecutar los Tests

1. Ejecuta el script de tests (se encarga de compilar todo automáticamente):
```bash
cd final_tests
./run_tests.py
```

O directamente desde la raíz del proyecto:
```bash
python3 final_tests/run_tests.py
```

## Resultados Esperados

Cada test tiene comentarios que indican los valores que deberían imprimirse. El script de tests:
1. Compila el compilador C--
2. Genera código ensamblador para cada test
3. Compila el ensamblador con GCC a un ejecutable
4. Ejecuta el ejecutable y captura su salida
5. Compara automáticamente la salida con los valores esperados
6. Muestra un resumen colorido de resultados

## Directorios Generados

- `assembly/`: Código ensamblador generado (.s)
- `final_tests/build/`: Ejecutables compilados
