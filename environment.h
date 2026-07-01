#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

// ============================================================
// Environment<T> — Tabla de símbolos con ámbitos anidados
// ============================================================
// Implementa el patrón de "ribs" (costillas): cada nivel de scope
// es un unordered_map nombre → valor. La búsqueda va del scope más
// interno al más externo (shadowing: el nombre interno oculta al externo).
//
// Uso principal en TypeChecker (visitor.h):
//   Environment<Type*>   env    — nombre de variable → tipo semántico
//   Environment<VarDecl*> varEnv — nombre → nodo VarDecl del AST (binding)
//
// Patrón típico al entrar/salir de un bloque o función:
//   env.add_level();
//   varEnv.add_level();
//   ... declarar variables con add_var ...
//   varEnv.remove_level();
//   env.remove_level();
//
// API resumida:
//   add_level()      — abre scope (push de mapa vacío)
//   add_var(n, v)    — define en el scope actual
//   remove_level()   — cierra scope (pop; variables locales desaparecen)
//   check_current(n) — ¿existe en el scope actual? (evitar redeclaración)
//   check(n)         — ¿existe en cualquier scope visible?
//   lookup(n)        — valor o T() si no existe (cuidado con punteros)
//   lookup(n, out)   — bool + valor por referencia (preferido en TypeChecker)
//   update(n, v)     — modifica en el rib donde fue declarada
//
// Limitaciones:
//   - add_var sin add_level previo → exit(EXIT_FAILURE) (error fatal)
//   - lookup() sin overload de ref devuelve T() si no hay símbolo;
//     con punteros eso puede parecer “encontrado” (nullptr válido)

template <typename T>
class Environment {
private:
    // ribs[0] = scope más externo (global o primer nivel abierto)
    // ribs.back() = scope actual (más interno)
    vector<unordered_map<string, T>> ribs;

    // Busca desde el rib más interno hacia afuera.
    // Retorna índice del rib o -1 si no existe.
    int search_rib(const string& var) const {
        for (int idx = static_cast<int>(ribs.size()) - 1; idx >= 0; --idx) {
            auto it = ribs[idx].find(var);
            if (it != ribs[idx].end())
                return idx;
        }
        return -1;
    }

public:
    Environment() = default;

    // Elimina todos los scopes; reinicio completo del entorno.
    void clear() {
        ribs.clear();
    }

    // Abre un nuevo ámbito. Las siguientes add_var van a ribs.back().
    void add_level() {
        ribs.emplace_back();
    }

    // Define una variable en el scope actual con valor inicial.
    // Requiere al menos un add_level() previo.
    void add_var(const string& var, const T& value) {
        if (ribs.empty()) {
            cerr << "[Error] Environment sin niveles: no se pueden agregar variables.\n";
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = value;
    }

    // Sobrecarga: valor por defecto T() (útil si T es puntero o numérico).
    void add_var(const string& var) {
        if (ribs.empty()) {
            cerr << "[Error] Environment sin niveles: no se pueden agregar variables.\n";
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = T();
    }

    // Cierra el scope más interno; las variables de ese nivel dejan de existir.
    bool remove_level() {
        if (!ribs.empty()) {
            ribs.pop_back();
            return true;
        }
        return false;
    }

    // Actualiza el valor en el rib donde se declaró originalmente
    // (puede ser un scope externo si hay shadowing en niveles internos).
    bool update(const string& x, const T& v) {
        int idx = search_rib(x);
        if (idx < 0) return false;
        ribs[idx][x] = v;
        return true;
    }

    // ¿Existe en algún scope visible (búsqueda completa)?
    bool check(const string& x) const {
        return (search_rib(x) >= 0);
    }

    // ¿Existe solo en el scope actual? Usado para detectar redeclaración
    // en el mismo bloque sin confundir con variables del scope padre.
    bool check_current(const string& x) const {
        if (ribs.empty()) return false;
        return ribs.back().count(x) > 0;
    }

    // Busca y devuelve el valor, o T() si no existe.
    // Para Type*/VarDecl* preferir lookup(x, v) que distingue “no encontrado”.
    T lookup(const string& x) const {
        int idx = search_rib(x);
        if (idx < 0) {
            return T();
        }
        return ribs[idx].at(x);
    }

    // Busca y asigna en v. Retorna true si el símbolo existe.
    // TypeChecker usa esto en visit(IdentifierNode) y capturas de lambda.
    bool lookup(const string& x, T& v) const {
        int idx = search_rib(x);
        if (idx < 0) return false;
        v = ribs[idx].at(x);
        return true;
    }
};

#endif // ENVIRONMENT_H
