// Constant folding benchmark - a literal-only expression is re-evaluated
// every iteration; a compiler that folds it at compile time turns it into
// a single constant load instead of repeating the arithmetic at runtime.
#include <stdio.h>

int main() {
    long long sum = 0;
    int i;
    for (i = 0; i < 12000000; i++) {
        int c = (123 * 457 - 89 * 3 + 7777 / 11 - 42 % 5) * 2 + 17 - 9 * 2;
        sum = sum + c + i;
    }
    printf("%ld\n", sum);
    return 0;
}
