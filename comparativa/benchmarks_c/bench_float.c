// Float arithmetic benchmark - tests intensive float/double operations
#include <stdio.h>

int main() {
    int n = 500000;
    
    float sum = 0.0;
    
    int i;
    for (i = 0; i < n; i++) {
        float x = i * 0.001;
        sum = sum + x * x;
    }
    
    printf("%f\n", sum);
    return 0;
}
