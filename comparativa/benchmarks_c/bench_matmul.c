// Matrix multiplication benchmark - tests multidimensional arrays and nested loops
#include <stdio.h>

int main() {
    int n = 80;
    
    int A[80][80];
    int B[80][80];
    int C[80][80];
    
    int i, j, k;
    
    // Initialize matrices
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
            C[i][j] = 0;
        }
    }
    
    // Matrix multiplication
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            for (k = 0; k < n; k++) {
                C[i][j] = C[i][j] + A[i][k] * B[k][j];
            }
        }
    }
    
    printf("%ld\n", (long)C[79][79]);
    return 0;
}
