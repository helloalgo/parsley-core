// #include <stdlib.h>

// int main() {
//     int* arr = (int*)malloc(1UL*1024*1024*64); // 64MB
//     for(int i=1;i<=10000000;i*=10) {
//         arr[i]=i+arr[i/10];
//     }
// }

#include <stdio.h>

int arr[16777216];
int main() {
    for(int i=1;i<=10000000;i*=10) {
        arr[i]=i+arr[i/10];
    }
    for(int i=10000000;i>0;i/=10) {
        printf("%d", arr[i]);
    }
}

