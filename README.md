# c--

Implementación de compilador para un subset de C.

## Requisitos

- g++ con soporte para C++17
- gcc (para generar ejecutables con `--exec`)
- make

## Compilar

```bash
make
```

Genera el ejecutable `c--`. Los archivos `.o` intermedios se eliminan automáticamente.

## Uso

```
./c-- <archivo.c--> [opciones]
```

### Flags

| Corta | Larga         | Descripción                                                  |
|-------|---------------|--------------------------------------------------------------|
| `-t`  | `--tokens`    | Imprime los tokens y termina                                 |
| `-a`  | `--ast`       | Imprime el AST y termina                                     |
| `-c`  | `--codegen`   | Imprime el assembly x86-64 y termina                         |
| `-A`  | `--all`       | Imprime tokens, AST y assembly                               |
| `-e`  | `--exec`      | Genera ejecutable a partir del assembly                      |
|       | `-o <archivo>`| Guarda texto de salida (sin `--exec`); nombre del binario (con `--exec`) |

### Comportamiento de `-o`

- **Sin `--exec`**: guarda la salida del modo en el archivo (tokens, AST o assembly).
- **Con `--exec`**: define el nombre del ejecutable generado; la salida de texto va a stdout.
- **Sin `-o`**: la salida de texto va a stdout.

### Ejemplos

```bash
./c-- programa.c--                # Assembly por stdout
./c-- programa.c-- -o salida.s    # Assembly a salida.s
./c-- programa.c-- -c             # Solo assembly por stdout
./c-- programa.c-- -c -o salida.s # Solo assembly a salida.s
./c-- programa.c-- -t             # Solo tokens por stdout
./c-- programa.c-- -t -o salida.txt # Solo tokens a salida.txt
./c-- programa.c-- -a             # Solo AST por stdout
./c-- programa.c-- -A             # Tokens + AST + assembly por stdout

./c-- programa.c-- --exec                  # Assembly por stdout + binario programa.out
./c-- programa.c-- --exec -o ejecutable    # Assembly por stdout + binario ejecutable
./c-- programa.c-- -A --exec               # Tokens + AST + assembly + binario programa.out
./c-- programa.c-- -A --exec -o ejecutable # Todo por stdout + binario ejecutable
```

## Pipeline

1. **Scanner** → tokeniza el fuente
2. **Parser** → construye el AST
3. **TypeChecker** → verifica tipos y resuelve tipos de expresiones
4. **ConstantFolding** → evalua expresiones constantes en tiempo de compilación
5. **GenCodeVisitor** → genera assembly x86-64
6. **gcc** (con `--exec`) → ensambla y linkea a ejecutable
