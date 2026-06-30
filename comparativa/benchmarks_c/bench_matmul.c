// Matrix multiplication benchmark - tests multidimensional arrays and nested loops
#include <stdio.h>

int main() {
    int n = 50;
    
    int A[50][50];
    int B[50][50];
    int C[50][50];
    
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
    
    printf("%ld\n", C[50][50]);
    return 0;
}
