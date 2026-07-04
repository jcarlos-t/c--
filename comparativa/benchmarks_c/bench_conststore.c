#include <stdio.h>

int main() {
    int a, b, c, d, e, f;
    long long sum = 0;
    int i;
    for (i = 0; i < 15000000; i++) {
        a = 1;
        b = 2;
        c = 3;
        d = 4;
        e = 5;
        f = 6;
        sum = sum + a + b + c + d + e + f + i;
    }
    printf("%ld\n", sum);
    return 0;
}
