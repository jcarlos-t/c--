# Plan: Frontend Web para el Compilador C--

## 1. Descripción General

Aplicación web donde el usuario escribe código en **C--** (subconjunto de C), lo envía a un backend que ejecuta el intérprete/compilador (binario C++), y visualiza el resultado (stdout, stderr, tokens, AST, etc.).

**Stack tecnológico:**
- Frontend: **Next.js 14+ (App Router)**
- Backend: **Next.js API Routes** (Node.js runtime)
- Compilador: Binario `c--` compilado desde C++
- Editor: **Monaco Editor** (`@monaco-editor/react`)
- Comunicación: REST API (JSON)

---

## 2. Requisitos del Sistema

- Node.js >= 18
- npm / yarn / pnpm
- El binario `c--` compilado (ver sección 13)
- Sistema operativo: Linux (por ejecución de subprocess)

---

## 3. Fases del Compilador Visibles desde el Frontend

Basado en la especificación del lenguaje (`docs/lenguaje.md`), el compilador tiene 4 fases que pueden exponerse al usuario:

| Fase | Clase | Input | Output visible | Formato |
|------|-------|-------|----------------|---------|
| **Tokens** (scanner) | `Scanner::nextToken()` | Código fuente | Lista de tokens | `TOKEN(TYPE, "text", line:col)` |
| **AST** (parser) | `Parser::parseProgram()` | Scanner | Árbol sintáctico | No serializado actualmente (se puede imprimir con visitor) |
| **Type Check** | `TypeChecker::typecheck()` | `Program*` | Errores semánticos | `"Error semántico: line N:M - ..."` |
| **Ejecución** | `EVALVisitor::interprete()` | `Program*` | Output del programa | `"Interprete:\n<valores>"` |
| **Código** (codegen) | `GenCodeVisitor::generate()` | `Program*` | Assembly x86-64 AT&T | Texto plano |

**Ideal a futuro:** el frontend debe permitir ver cada fase por separado (similar a Godbolt / Compiler Explorer).

---

## 4. Características del Lenguaje C-- (Relevantes para el Frontend)

### 4.1 Sistema de Tipos

| Tipo | Declaración | Uso en frontend |
|------|-------------|-----------------|
| `void` | `void func()` | Solo retorno de función |
| `int` | `int x;` | Entero 32 bits |
| `char` | `char c;` | Carácter (1 byte, tratado como int) |
| `float` | `float f;` | 32 bits, compatible con double |
| `double` | `double d;` | 64 bits, semánticamente = float |
| `bool` | `bool b;` | `true` / `false` (1 = true, 0 = false) |
| `auto` | `auto x = expr;` | Inferido del inicializador |
| `T*` | `int* p;` | Puntero (8 bytes) |
| `struct` | `struct S { int x; };` | Tipo definido por usuario |

### 4.2 Constructos del Lenguaje

**Control de flujo:**
```c
if (cond) { ... } else { ... }
switch (expr) { case 1: ... default: ... }
while (cond) { ... }
do { ... } while (cond);
for (init; cond; inc) { ... }
break;
continue;
return expr;
```

**Operadores:**
```c
// Aritméticos: + - * / % ** (potencia)
// Comparación: == != < > <= >=
// Lógicos: && || !
// Asignación: = += -= *= /=
// Ternario: cond ? then : else
// Unarios: ++ -- & * -
// Cast: (tipo) expr
// Memoria: malloc(size)  free(ptr)
// sizeof: sizeof(tipo)
// Output: printf(expr1, expr2, ...)
```

**Punteros y structs:**
```c
int x = 42;
int* p = &x;
*p = 10;

struct Punto { int x; int y; };
struct Punto pt;
pt.x = 1;
pt.y = 2;
struct Punto* pp = &pt;
pp->x = 3;
```

**Lambdas:**
```c
auto sum = [=](int a, int b) -> int { return a + b; };
printf(sum(1, 2));
```

**Templates:**
```c
template <typename T>
T max(T a, T b) {
  return a > b ? a : b;
}
```

### 4.3 Reglas Semánticas Clave

| Regla | Implicación en frontend |
|-------|------------------------|
| Condiciones (`if`, `while`, `for`, `?:`) deben ser `bool` | Mostrar error si se pasa `int` |
| `&&` `||` `!` solo para `bool` | Error semántico si se usa con números |
| Comparaciones devuelven `bool` | Correcto |
| Asignación compuesta (`+=`, etc.) sobre lvalue | Validar que el target sea modificable |
| `break`/`continue` solo dentro de loops/switch | Error semántico |
| `malloc` retorna `void*` | Requiere cast explícito |
| `printf` acepta múltiples argumentos separados por coma | Única forma de output |

### 4.4 Manejo de Errores

| Fase | Cómo se reporta | Ejemplo |
|------|-----------------|---------|
| Léxico | Token `ERR` | `Caracter invalido en linea 1:5` |
| Sintáctico | Excepción con `line N:M - ...` | `line 3:10 - Error sintáctico: se esperaba ';' pero se encontró '}'` |
| Semántico | `Error semántico: line N:M - ...` | `Error semántico: line 5:12 - variable 'x' no declarada` |
| Runtime | `cerr << "Error: ..."` | `Error: división por cero`, `Error: índice fuera de rango` |

---

## 6. Estructura del Proyecto

```
c--/
├── app/
│   ├── layout.tsx
│   ├── page.tsx                  # Página principal: editor + output
│   ├── api/
│   │   └── run/
│   │       └── route.ts          # API que ejecuta el binario c--
│   │   └── examples/
│   │       └── route.ts          # API que devuelve ejemplos de código
│   │   └── phases/
│   │       └── route.ts          # API que ejecuta fases individuales
│   ├── globals.css
├── components/
│   ├── CodeEditor.tsx            # Wrapper de Monaco Editor
│   ├── OutputPanel.tsx           # Muestra stdout/stderr
│   ├── Toolbar.tsx               # Botón Run, selector de ejemplos
│   ├── PhaseTabs.tsx             # Pestañas: Output | Tokens | AST | Errores
│   └── ExamplesDropdown.tsx      # Dropdown con ejemplos precargados
├── lib/
│   ├── run.ts                    # Lógica para llamar al API
│   ├── examples.ts               # Lista de ejemplos de código
│   └── types.ts                  # Tipos TypeScript compartidos
├── public/
│   └── examples/                 # Archivos .cnn de ejemplo (opcional)
├── package.json
├── tsconfig.json
├── next.config.js
└── c--                           # Binario compilado (se genera con make)
```

---

## 7. API Routes

### 7.1 `POST /api/run`

Ejecuta el código C-- completo (scanner + parser + typechecker + intérprete).

**Request:**
```json
{
  "code": "int main() { printf(42); }"
}
```

**Response (éxito):**
```json
{
  "ok": true,
  "stdout": "Interprete:\n42\n",
  "stderr": "",
  "exitCode": 0
}
```

**Response (error de compilación):**
```json
{
  "ok": false,
  "stdout": "",
  "stderr": "Error al parsear: line 1:10 - ...",
  "exitCode": 1
}
```

**Implementación (`app/api/run/route.ts`):**
```typescript
import { NextRequest, NextResponse } from 'next/server';
import { execFile } from 'child_process';
import { promisify } from 'util';
import { writeFile, unlink } from 'fs/promises';
import path from 'path';
import os from 'os';

const execFileAsync = promisify(execFile);

export async function POST(request: NextRequest) {
  const { code } = await request.json();
  if (!code || typeof code !== 'string') {
    return NextResponse.json({ ok: false, error: 'No code provided' }, { status: 400 });
  }

  const tmpFile = path.join(os.tmpdir(), `cnn_${Date.now()}.cnn`);
  await writeFile(tmpFile, code);

  try {
    const binPath = path.join(process.cwd(), 'c--');
    const { stdout, stderr } = await execFileAsync(binPath, [tmpFile], {
      timeout: 5000,
      maxBuffer: 1024 * 1024,
    });
    return NextResponse.json({ ok: true, stdout, stderr, exitCode: 0 });
  } catch (e: any) {
    return NextResponse.json({
      ok: false,
      stdout: e.stdout || '',
      stderr: e.stderr || e.message,
      exitCode: e.code || 1,
    });
  } finally {
    await unlink(tmpFile).catch(() => {});
  }
}
```

### 7.2 `GET /api/examples`

Devuelve una lista de ejemplos precargados. Todos los ejemplos deben ser código C-- válido según la especificación del lenguaje.

**Response:**
```json
{
  "examples": [
    { "name": "Hola Mundo", "code": "int main() { printf(42); }" },
    { "name": "Suma", "code": "int main() { printf(1 + 2); }" },
    { "name": "Factorial", "code": "int fact(int n) { if (n <= 1) { return 1; } else { return n * fact(n - 1); } }\nint main() { printf(fact(5)); }" },
    { "name": "Switch", "code": "int main() { int x = 2; switch (x) { case 1: { printf(10); } case 2: { printf(20); } default: { printf(30); } } }" },
    { "name": "Punteros", "code": "int main() { int x = 42; int* p = &x; printf(*p); }" },
    { "name": "Struct", "code": "struct Punto { int x; int y; };\nint main() { struct Punto pt; pt.x = 10; pt.y = 20; printf(pt.x + pt.y); }" },
    { "name": "Arreglos", "code": "int main() { int arr[3]; arr[0] = 1; arr[1] = 2; arr[2] = 3; printf(arr[0] + arr[1] + arr[2]); }" },
    { "name": "Lambda", "code": "int main() { auto sum = [=](int a, int b) -> int { return a + b; }; printf(sum(3, 4)); }" },
    { "name": "Malloc", "code": "int main() { int* p = (int*) malloc(4); *p = 99; printf(*p); free(p); }" },
    { "name": "Template", "code": "template <typename T>\nT max(T a, T b) { return a > b ? a : b; }\nint main() { printf(max(3, 7)); }" }
  ]
}
```

**Ejemplos que NO funcionan** (para referencia de lo que el lenguaje NO soporta aún):
- Strings como variables (`char* s = "hola"`) — solo en printf como literales
- Arrays multidimensionales con tamaño dinámico
- I/O de archivos
- Comentarios de línea (`//`) o bloque (`/* */`) — no están en la gramática

---

## 8. Componentes Principales

### 8.1 `page.tsx` (Página principal)

```
┌──────────────────────────────────────────────┐
│  Toolbar: [▶ Run]  [Examples ▾]  [Clear]     │
├───────────────────────┬──────────────────────┤
│                       │                      │
│   CodeEditor          │   PhaseTabs           │
│   (Monaco Editor)     │   ┌────────────────┐ │
│                       │   │ [Output] [Tokens]│ │
│                       │   │ [AST] [Errores] │ │
│                       │   │ [Assembly]      │ │
│                       │   ├────────────────┤ │
│                       │   │                │ │
│                       │   │  Panel de      │ │
│                       │   │  resultados    │ │
│                       │   │                │ │
│                       │   └────────────────┘ │
└───────────────────────┴──────────────────────┘
```

**Estado del componente `page.tsx`:**
```typescript
const [code, setCode] = useState<string>(DEFAULT_EXAMPLE);
const [output, setOutput] = useState<RunResponse | null>(null);
const [loading, setLoading] = useState(false);
const [activeTab, setActiveTab] = useState<'output' | 'tokens' | 'ast' | 'errors' | 'assembly'>('output');
```

### 8.2 `CodeEditor.tsx`

```typescript
'use client';
import Editor from '@monaco-editor/react';

interface Props {
  value: string;
  onChange: (value: string) => void;
}

export default function CodeEditor({ value, onChange }: Props) {
  return (
    <Editor
      height="500px"
      defaultLanguage="c"
      theme="vs-dark"
      value={value}
      onChange={(val) => onChange(val ?? '')}
      options={{
        minimap: { enabled: false },
        fontSize: 14,
        fontFamily: 'JetBrains Mono, monospace',
      }}
    />
  );
}
```

**Nota:** Se usa `defaultLanguage="c"` por ahora, que cubre la mayoría de keywords y syntax de C--. Si se desea resaltado más preciso, se puede definir una gramática personalizada:

### 8.2.1 Gramática personalizada Monaco para C-- (opcional)

```typescript
// lib/c--language.ts
import { languages } from 'monaco-editor';

export const C__LANGUAGE: languages.IMonarchLanguage = {
  defaultToken: '',
  tokenPostfix: '.c--',

  keywords: [
    'auto', 'break', 'case', 'char', 'continue', 'default', 'do', 'double',
    'else', 'float', 'for', 'if', 'int', 'malloc', 'free', 'printf',
    'return', 'sizeof', 'struct', 'switch', 'template', 'typename',
    'void', 'while', 'bool', 'true', 'false',
  ],

  typeKeywords: ['int', 'char', 'float', 'double', 'void', 'bool', 'auto', 'struct'],

  operators: [
    '=', '>', '<', '!', '~', '?', ':',
    '==', '<=', '>=', '!=', '&&', '||', '++', '--',
    '+', '-', '*', '/', '%', '&', '|', '^', '**',
    '+=', '-=', '*=', '/=',
    '->', '.',
  ],

  // ... resto de la definición Monarch
  // Ver: https://microsoft.github.io/monaco-editor/monarch.html
};
```

Registrarla en el editor:
```typescript
import { loader } from '@monaco-editor/react';
import { C__LANGUAGE } from '@/lib/c--language';

loader.init().then((monaco) => {
  monaco.languages.register({ id: 'c--' });
  monaco.languages.setMonarchTokensProvider('c--', C__LANGUAGE);
});
```

### 8.3 `OutputPanel.tsx`

```typescript
'use client';

interface Props {
  stdout: string;
  stderr: string;
  loading: boolean;
}

export default function OutputPanel({ stdout, stderr, loading }: Props) {
  if (loading) return <div className="p-4 text-gray-400">Ejecutando...</div>;
  if (!stdout && !stderr) return <div className="p-4 text-gray-400">Presiona Run para ejecutar</div>;

  return (
    <div className="font-mono text-sm p-4 whitespace-pre-wrap">
      {stdout && <div className="text-green-400">{stdout}</div>}
      {stderr && <div className="text-red-400">{stderr}</div>}
    </div>
  );
}
```

### 8.4 `PhaseTabs.tsx`

Muestra el resultado de diferentes fases del compilador según la pestaña activa.

```typescript
'use client';

interface Props {
  activeTab: string;
  onTabChange: (tab: string) => void;
  output: RunResponse | null;
  loading: boolean;
}

const TABS = [
  { id: 'output', label: 'Output' },
  { id: 'tokens', label: 'Tokens' },
  { id: 'ast', label: 'AST' },
  { id: 'errors', label: 'Errores' },
  { id: 'assembly', label: 'Assembly' },
];

export default function PhaseTabs({ activeTab, onTabChange, output, loading }: Props) {
  const content = () => {
    if (loading) return <div className="p-4 text-gray-400">Ejecutando...</div>;
    if (!output) return <div className="p-4 text-gray-400">Presiona Run para ejecutar</div>;

    switch (activeTab) {
      case 'output':
        return (
          <div className="font-mono text-sm p-4 whitespace-pre-wrap">
            {output.stdout && <div className="text-green-400">{output.stdout}</div>}
            {output.stderr && <div className="text-red-400">{output.stderr}</div>}
          </div>
        );
      case 'tokens':
        return <div className="font-mono text-sm p-4 whitespace-pre-wrap text-yellow-300">{output.stderr}</div>;
      case 'errors':
        return output.stderr ? (
          <div className="font-mono text-sm p-4 whitespace-pre-wrap text-red-400">{output.stderr}</div>
        ) : (
          <div className="p-4 text-green-400">Sin errores</div>
        );
      default:
        return <div className="p-4 text-gray-500">No disponible para esta fase</div>;
    }
  };

  return (
    <div>
      <div className="flex border-b border-gray-700">
        {TABS.map((tab) => (
          <button
            key={tab.id}
            onClick={() => onTabChange(tab.id)}
            className={`px-4 py-2 text-sm font-medium ${
              activeTab === tab.id
                ? 'border-b-2 border-blue-500 text-blue-400'
                : 'text-gray-400 hover:text-gray-200'
            }`}
          >
            {tab.label}
          </button>
        ))}
      </div>
      <div className="h-full overflow-auto">{content()}</div>
    </div>
  );
}
```

**Nota sobre disponibilidad de fases:** El compilador actualmente no produce salida para tokens, AST o assembly en stdout. Esas pestañas requerirán modificar el binario `c--` para que acepte flags como `--tokens`, `--ast`, `--codegen`. Hasta entonces, solo `output` y `errors` (desde stderr) estarán funcionales.

### 8.5 `lib/examples.ts`

Catálogo de ejemplos de código C-- que se muestran en el dropdown. Cada ejemplo debe ser código válido según la gramática de C--.

```typescript
// lib/examples.ts
import { Example } from './types';

export const EXAMPLES: Example[] = [
  {
    name: 'Hola Mundo',
    code: `int main() {
  printf(42);
}`,
  },
  {
    name: 'Suma',
    code: `int main() {
  int a = 5;
  int b = 3;
  printf(a + b);
}`,
  },
  {
    name: 'Factorial',
    code: `int fact(int n) {
  if (n <= 1) {
    return 1;
  } else {
    return n * fact(n - 1);
  }
}

int main() {
  printf(fact(5));
}`,
  },
  {
    name: 'Switch',
    code: `int main() {
  int x = 2;
  switch (x) {
    case 1: { printf(10); }
    case 2: { printf(20); }
    default: { printf(30); }
  }
}`,
  },
  {
    name: 'Punteros',
    code: `int main() {
  int x = 42;
  int* p = &x;
  printf(*p);
}`,
  },
  {
    name: 'Struct',
    code: `struct Punto {
  int x;
  int y;
};

int main() {
  struct Punto pt;
  pt.x = 10;
  pt.y = 20;
  printf(pt.x + pt.y);
}`,
  },
  {
    name: 'Arreglos',
    code: `int main() {
  int arr[3];
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  printf(arr[0] + arr[1] + arr[2]);
}`,
  },
  {
    name: 'Lambda',
    code: `int main() {
  auto sum = [=](int a, int b) -> int {
    return a + b;
  };
  printf(sum(3, 4));
}`,
  },
  {
    name: 'Malloc',
    code: `int main() {
  int* p = (int*) malloc(4);
  *p = 99;
  printf(*p);
  free(p);
}`,
  },
  {
    name: 'Template',
    code: `template <typename T>
T max(T a, T b) {
  return a > b ? a : b;
}

int main() {
  printf(max(3, 7));
}`,
  },
  {
    name: 'While loop',
    code: `int main() {
  int i = 0;
  while (i < 5) {
    printf(i);
    i = i + 1;
  }
}`,
  },
  {
    name: 'For loop',
    code: `int main() {
  for (int i = 0; i < 3; i = i + 1) {
    printf(i);
  }
}`,
  },
];
```

### 8.6 `Toolbar.tsx`

```typescript
'use client';

interface Props {
  onRun: () => void;
  loading: boolean;
  onExampleSelect: (code: string) => void;
}

export default function Toolbar({ onRun, loading, onExampleSelect }: Props) {
  return (
    <div className="flex items-center gap-4 p-3 bg-gray-900 border-b border-gray-700">
      <button
        onClick={onRun}
        disabled={loading}
        className="px-5 py-2 bg-green-600 hover:bg-green-700 disabled:bg-gray-600 rounded font-medium"
      >
        {loading ? 'Ejecutando...' : '▶ Run'}
      </button>
      <select
        onChange={(e) => {
          if (e.target.value) onExampleSelect(e.target.value);
        }}
        className="bg-gray-800 border border-gray-600 rounded px-3 py-2 text-sm"
      >
        <option value="">Ejemplos...</option>
        <option value="Hola Mundo">Hola Mundo</option>
        <option value="Suma">Suma</option>
        <option value="Factorial">Factorial</option>
        <option value="Punteros">Punteros</option>
        <option value="Struct">Struct</option>
        <option value="Lambda">Lambda</option>
      </select>
    </div>
  );
}
```

### 7.3 `POST /api/phases` (opcional, futuro)

Ejecuta una fase específica del compilador. Requiere modificar el binario `c--` para aceptar flags.

**Request:**
```json
{
  "code": "int main() { printf(42); }",
  "phase": "tokens"   // "tokens" | "ast" | "check" | "run" | "codegen"
}
```

---

## 9. Ejemplos de Código C-- para Probar

Basados en la especificación del lenguaje (`docs/lenguaje.md`):

### 9.1 Programa mínimo
```c
int main() {
  printf(42);
}
// Output: Interprete:\n42
```

### 9.2 Variables y aritmética
```c
int main() {
  int a = 10;
  int b = 3;
  printf(a + b * 2);
}
```

### 9.3 Condicionales
```c
int main() {
  bool cond = true;
  if (cond) {
    printf(1);
  } else {
    printf(0);
  }
}
```

### 9.4 Loops
```c
int main() {
  int i = 0;
  while (i < 3) {
    printf(i);
    i = i + 1;
  }
}
```

### 9.5 Punteros y malloc
```c
int main() {
  int* p = (int*) malloc(4);
  *p = 42;
  printf(*p);
  free(p);
}
```

### 9.6 Structs
```c
struct Vec2 { int x; int y; };

int main() {
  struct Vec2 v;
  v.x = 1;
  v.y = 2;
  printf(v.x + v.y);
}
```

### 9.7 Lambdas
```c
int main() {
  auto f = [=](int x) -> int { return x * 2; };
  printf(f(21));
}
```

### 9.8 Templates
```c
template <typename T>
T ident(T x) { return x; }

int main() {
  printf(ident(99));
}
```

---

## 10. Limitaciones del Lenguaje C-- (para la UI)

| No soportado | Alternativa |
|-------------|-------------|
| `//` comentarios, `/* */` | No hay comentarios en la gramática |
| Strings como variables (`char* s = "hola"`) | Solo literales en `printf` |
| `printf` con formato (`%d`, etc.) | `printf(expr1, expr2, ...)` imprime cada expr |
| I/O de archivos | No existe |
| Arrays multidimensionales grandes | Solo estáticos o con malloc |
| `else if` (no hay syntactic sugar) | Usar `else { if (...) { ... } }` anidado |

---

## 11. Instalación y Configuración

### 11.1 Crear proyecto Next.js

```bash
npx create-next-app@latest . --typescript --tailwind --eslint --app
```

### 11.2 Instalar dependencias

```bash
npm install @monaco-editor/react
```

### 11.3 Compilar el binario c--

```bash
g++ -std=c++17 -o c-- main.cpp scanner.cpp token.cpp parser.cpp ast.cpp visitor.cpp TypeChecker.cpp Gencode.cpp
```

Ubicar el binario en la raíz del proyecto Next.js (o en `bin/` y ajustar la ruta en el API route).

### 11.4 Configurar `next.config.js`

```javascript
/** @type {import('next').NextConfig} */
const nextConfig = {
  // Necesario para que el API route pueda ejecutar binarios
  serverExternalPackages: [],
};

module.exports = nextConfig;
```

### 11.5 Ejecutar en desarrollo

```bash
npm run dev
```

---

## 12. Posibles Mejoras Futuras

| Mejora | Descripción |
|--------|-------------|
| **Gramática Monaco personalizada** | Syntax highlighting específico para C-- (keywords: `int`, `bool`, `struct`, `template`, `malloc`, `printf`; operadores: `**`, `->`) |
| **Flags en el binario** | Modificar `main.cpp` para aceptar `--tokens`, `--ast`, `--check`, `--run`, `--codegen` y devolver la salida de cada fase por separado |
| **Serialización del AST** | Imprimir el AST en formato JSON o texto legible para mostrarlo en la pestaña "AST" |
| **Botón "Copy error"** | Copiar el mensaje de error al portapapeles |
| **WebAssembly** | Compilar el intérprete a WASM con Emscripten y ejecutarlo 100% en el navegador (sin backend) |
| **Tests de integración** | Playwright o Cypress para probar el flujo completo |
| **Docker** | Contenedor con el binario y Node.js para despliegue reproducible (usar `node:20-slim` + binario compilado) |
| **Sandbox más seguro** | Ejecutar el binario dentro de un contenedor Docker por request con timeout y límite de memoria |
| **Persistencia** | Guardar código del usuario en localStorage |
| **Compartir código** | Compartir snippet via URL codificada en base64 |
| **Linter en tiempo real** | Ejecutar type checking al escribir (debounce 500ms) y mostrar errores en el editor (Monaco markers) |
| **Dark/Light theme** | Alternar entre tema oscuro y claro tanto en el editor como en la UI |

---

## 13. Dependencia del Binario C--

El API route `app/api/run/route.ts` depende del binario `c--` compilado. El binario debe:
- Aceptar como argumento la ruta a un archivo `.cnn`
- Imprimir el resultado del programa en **stdout** (lo que sea que produzca `cout`)
- Imprimir errores en **stderr** (lo que sea que produzca `cerr`)
- Retornar exit code 0 en éxito, != 0 en error

**Comportamiento actual del binario:**

| Salida | Contenido | Propósito en frontend |
|--------|-----------|----------------------|
| `cout` | `"Parseo exitoso"`, `"Revisión exitosa"`, `"Interprete:\n<output>"` | Pestaña "Output" (stdout) |
| `cerr` | Errores sintácticos, semánticos y runtime | Pestaña "Errores" (stderr) |

Si el binario aún no está terminado, el API route igual funciona — devolverá el error del binario (ej: "Segmentation fault", "No such file", etc.) y la UI lo mostrará correctamente.

**Modificación futura recomendada:** Agregar un flag `--stdin` al binario para leer el código desde stdin en vez de archivo, y flags para fases individuales.

---

## 14. Arquitectura de Datos (TypeScript Types)

```typescript
// lib/types.ts

export interface RunRequest {
  code: string;
}

export interface RunResponse {
  ok: boolean;
  stdout: string;
  stderr: string;
  exitCode: number;
}

export interface PhaseRequest {
  code: string;
  phase: 'tokens' | 'ast' | 'check' | 'run' | 'codegen';
}

export interface PhaseResponse {
  ok: boolean;
  output: string;
  error: string;
}

export interface Example {
  name: string;
  code: string;
}

export interface ExamplesResponse {
  examples: Example[];
}
```

---

## 15. Flujo de Usuario

1. Usuario abre la página `http://localhost:3000`
2. Ve un editor de código con el ejemplo "Hola Mundo" precargado
3. Modifica el código o selecciona otro ejemplo del dropdown
4. Presiona **▶ Run**
5. El frontend muestra estado "Ejecutando..." y deshabilita el botón
6. `fetch('/api/run', { method: 'POST', body: JSON.stringify({ code }) })`
7. El API route escribe el código a un archivo temporal en `/tmp/cnn_<timestamp>.cnn`
8. El API ejecuta `./c-- /tmp/cnn_<timestamp>.cnn` con timeout de 5s
9. El API captura stdout/stderr/exitCode
10. El API elimina el archivo temporal
11. El frontend recibe `{ ok, stdout, stderr, exitCode }`
12. El frontend muestra el resultado:
    - Pestaña "Output" → stdout en verde
    - Pestaña "Errores" → stderr en rojo
    - Si `exitCode !== 0` → el borde del output panel se pone rojo
13. El usuario cambia entre pestañas para ver diferentes fases
14. El usuario modifica el código y vuelve a presionar Run

---

## 16. Checklist de Implementación

### Fase 1 — Base (mínimo viable)

- [ ] `npx create-next-app@latest . --typescript --tailwind --eslint --app`
- [ ] `npm install @monaco-editor/react`
- [ ] Crear `lib/types.ts` con interfaces `RunRequest`, `RunResponse`, `Example`
- [ ] Crear `lib/run.ts` — función `runCode(code: string): Promise<RunResponse>`
- [ ] Crear `lib/examples.ts` — array de ejemplos de código C-- (ver sección 8.5)
- [ ] Crear `app/api/run/route.ts` — `POST` que ejecuta el binario
- [ ] Crear `app/api/examples/route.ts` — `GET` que devuelve los ejemplos
- [ ] Crear `components/CodeEditor.tsx` — editor Monaco con lenguaje "c"
- [ ] Crear `components/PhaseTabs.tsx` — tabs con Output y Errores funcionales
- [ ] Crear `components/Toolbar.tsx` — botón Run y dropdown de ejemplos
- [ ] Crear `app/page.tsx` — integración de todos los componentes
- [ ] Compilar `c--` y copiar a la raíz del proyecto: `g++ -std=c++17 -o c-- *.cpp`

### Fase 2 — Validación y robustez

- [ ] Probar flujo completo: editar código → Run → ver output
- [ ] Probar con código inválido (sintaxis incorrecta → error en rojo)
- [ ] Probar timeout (código con loop infinito → el API corta a los 5s)
- [ ] Probar con código semánticamente incorrecto (tipos, undeclared vars)
- [ ] Manejar error si el binario `c--` no existe: `{ ok: false, stderr: "Binary not found" }`
- [ ] Agregar estado de carga visual en el botón Run

### Fase 3 — Experiencia de usuario

- [ ] Tema oscuro global (clase `dark` en Tailwind)
- [ ] Layout responsivo (sidebar colapsable en mobile)
- [ ] Atajo de teclado `Ctrl+Enter` para ejecutar
- [ ] Al cambiar de ejemplo, reemplazar el código en el editor
- [ ] Mostrar número de línea actual en la esquina inferior
- [ ] Borrar output anterior cuando se presiona Run

### Fase 4 — Avanzado (futuro)

- [ ] Modificar `main.cpp` para flags `--tokens`, `--ast`, `--codegen` y conectar las pestañas
- [ ] Gramática Monarch personalizada para Monaco (registrar lenguaje `c--`)
- [ ] `POST /api/phases` para ejecutar fases individuales
- [ ] Botón "Copy error"
- [ ] localStorage para persistir el código
- [ ] Compartir código via URL
