#include <stdio.h>
#include <stdint.h>

int main(void) {
    uint64_t x = 1 << 19;

    printf("%d\n", 64 - __builtin_clzll(x - 1));
}
