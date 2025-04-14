#undef _GLIBCXX_DEBUG
#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
//ifndef POPCNT
#pragma GCC target("movbe,aes,pclmul,avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2,rdrnd,popcnt,bmi,bmi2,lzcnt")
//#endif

#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>  // For _pext_u64
#include <stdlib.h>
#include <time.h>

typedef unsigned __int128 u128;

uint64_t popcount128(u128 x) {
    return __builtin_popcountll((uint64_t)x) + __builtin_popcountll((uint64_t)(x >> 64));
}

u128 random_set_bit(u128 mask) {
    uint64_t low = (uint64_t)mask;
    uint64_t high = (uint64_t)(mask >> 64);

    uint64_t pop = popcount128(mask);
    if (pop == 0) return 0;

    // Pick a random index
    uint64_t r = rand() % pop;
	printf("r = %lu\n", r);

    // Create a bitmask with a single bit set at position r in compressed space
    uint64_t one_hot = 1ULL << r;

    // Use PDEP (reverse of PEXT) to "spread" the bit into the original mask space
    // PDEP is the inverse of PEXT, and places bits in positions specified by mask
    // if (low) {
    //     return (u128)_pdep_u64(one_hot, low);
    // } else {
    //     return (u128)_pdep_u64(one_hot, high) << 64;
    // }
	u128 result = (u128)_pdep_u64(one_hot, low);
	result |= (u128)_pdep_u64(one_hot, high) << 64;

	// Return the result
	return result;
}

void print128(u128 x) {
    for (int i = 127; i >= 0; --i)
        putchar(((x >> i) & 1) ? '1' : '0');
    putchar('\n');
}

int main() {
    srand(time(NULL));

    u128 mask = ((u128)1 << 5) | ((u128)1 << 42) | ((u128)1 << 80);
    print128(mask);

    u128 chosen = random_set_bit(mask);
    print128(chosen);
}
