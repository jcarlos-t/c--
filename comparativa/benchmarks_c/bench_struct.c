// Struct and pointer benchmark - tests struct access and pointer operations
#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main() {
    int n = 100000;
    
    struct Point p;
    p.x = 1;
    p.y = 2;
    
    struct Point* ptr = &p;
    
    int sum = 0;
    
    int i;
    for (i = 0; i < n; i++) {
        ptr->x = ptr->x + i;
        ptr->y = ptr->y + i * 2;
        sum = sum + ptr->x + ptr->y;
    }
    
    printf("%ld\n", sum);
    return 0;
}
