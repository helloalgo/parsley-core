#include <cstdio>
#include <cstring>
#include "box.hpp"

int main() {
    printf("Starting parsley\n");
    char command[30] = "/src/tests/ls";
    FILE* fout = fopen("test-out", "w");
    FILE* ferr = fopen("test-err", "w");
    ProcArgs* args = new ProcArgs(1, new char*[1]{command});
    args->fdOut = fileno(fout);    
    args->fdErr = fileno(ferr);
    args->log();

    ProcLimit* limit = new ProcLimit(100000000, 1000000000, 1000000000, "basic");
    auto result = runProcess(*args, *limit);
    result->log();
}
