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

int main() {
	int x = 0b0110010101110010;

	print_binary(x, 16);
	while (x) {
		print_binary(x & -x, 16);
		x = x & (x - 1);
	}
}
