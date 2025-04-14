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

template <typename T>
void print_binary(T x, int width = sizeof(T) * 8) {
	for (int i = width; i >= 0; --i)
		putchar(((x >> i) & 1) ? '1' : '0');
	putchar('\n');
}

uint64_t popcount128(u128 x) {
	return __builtin_popcountll((uint64_t)x) + __builtin_popcountll((uint64_t)(x >> 64));
}

u128 random_set_bit(u128 mask) {
	uint64_t low = (uint64_t)mask;
	uint64_t high = (uint64_t)(mask >> 64);

	uint64_t pop = popcount128(mask);
	uint64_t pop_low = popcount128(low);
	if (pop == 0) return 0;

	uint64_t r = rand() % pop;

	if (pop_low > 0 && r < pop_low)
	{
		return (u128)_pdep_u64(1ULL << r, low);
	}
	else
	{
		return (u128)_pdep_u64(1ULL << (r - pop_low), high) << 64;
	}
}

int main() {
	srand(time(NULL));

	u128 mask = ((u128)1 << 0) | ((u128)1 << 63) | ((u128)1 << 64) | ((u128)1 << 81);
	print_binary<u128>(mask, 81);

	u128 chosen = random_set_bit(mask);
	print_binary<u128>(chosen, 81);
}
