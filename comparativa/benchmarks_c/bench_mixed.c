// Mixed operations benchmark - combines all features
#include <stdio.h>

struct Data {
    int value;
    float weight;
};

int main() {
    int n = 150000;
    
    struct Data d;
    d.value = 0;
    d.weight = 0.0;
    
    int sum = 0;
    float fsum = 0.0;
    
    int i;
    for (i = 0; i < n; i++) {
        d.value = d.value + i;
        d.weight = d.weight + i * 0.1;
        sum = sum + d.value;
        fsum = fsum + d.weight;
    }
    
    printf("%ld\n", sum);
    return 0;
}
