#include <stdio.h>
#include <time.h>

int main() {
    struct timespec ts;
    ts.tv_nsec = 500000;
    printf("Sleeping.. ");
    nanosleep(&ts, &ts);
    printf("Hi!");
}