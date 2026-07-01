// Prime numbers benchmark - tests loops, modulo, and conditionals
#include <stdio.h>

int main() {
    int n = 40000;
    
    int count = 0;
    
    int i;
    for (i = 2; i <= n; i++) {
        int is_prime = 1;
        
        int j;
        for (j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                is_prime = 0;
                break;
            }
        }
        
        if (is_prime == 1) {
            count = count + 1;
        }
    }
    
    printf("%ld\n", count);
    return 0;
}
