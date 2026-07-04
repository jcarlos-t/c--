// Stack frame benchmark - many small scalar locals (12 int + 12 char).
// Meant to stress bin packing of stack-frame offsets: a compiler that
// packs small variables into shared 8-byte blocks needs a much smaller
// frame than one that gives every variable its own 8-byte slot.
#include <stdio.h>

int main() {
    int v0 = 1, v1 = 2, v2 = 3, v3 = 4, v4 = 5, v5 = 6;
    int v6 = 7, v7 = 8, v8 = 9, v9 = 10, v10 = 11, v11 = 12;
    char c0 = 'a', c1 = 'b', c2 = 'c', c3 = 'd', c4 = 'e', c5 = 'f';
    char c6 = 'g', c7 = 'h', c8 = 'i', c9 = 'j', c10 = 'k', c11 = 'l';

    long long sum = 0;
    int i;
    for (i = 0; i < 1500000; i++) {
        int partial = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11;
        partial = partial + c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9 + c10 + c11;
        sum = sum + partial + i;
    }
    printf("%ld\n", sum);
    return 0;
}
