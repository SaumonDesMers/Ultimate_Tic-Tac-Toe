#undef _GLIBCXX_DEBUG                // disable run-time bound checking, etc
#pragma GCC optimize("Ofast,inline") // Ofast = O3,fast-math,allow-store-data-races,no-protect-parens

#ifndef __POPCNT__ // not march=generic

#pragma GCC target("bmi,bmi2,lzcnt,popcnt")                      // bit manipulation
#pragma GCC target("movbe")                                      // byte swap
#pragma GCC target("aes,pclmul,rdrnd")                           // encryption
#pragma GCC target("avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2") // SIMD

#endif // end !POPCNT

#include <iostream>
#include <bitset>
#include <cstdlib>
#include <iomanip>
#include <cmath>

using namespace std;

// #define randomMACRO(min, max) min + rand() % (max - min)

// inline uint64_t random(int min, int max) {
//     return min + rand() % (max - min);
// }

// uint64_t n = time(NULL);
// inline uint64_t hash1(int min, int max) {
//     n = (n ^ 61) ^ (n >> 16);
//     n = n + (n << 3);
//     n = n ^ (n >> 4);
//     n = n * 0x27d4eb2d;
//     n = n ^ (n >> 15);
//     return min + n % (max - min);
// }

// uint64_t u = time(NULL);
// uint64_t v = u >> 8;
// inline uint64_t hash2(int min, int max) {
// 	v = 36969*(v & 65535) + (v >> 16);
// 	u = 18000*(u & 65535) + (u >> 16);
// 	return min + ((v << 16) + (u & 65535)) % (max - min);
// }

// const uint64_t mod = static_cast<unsigned long>(pow(2, 32));
// // const uint64_t multiplier = 1664525;
// // const uint64_t increment = 1013904223;
// const uint64_t multiplier = time(NULL);
// const uint64_t increment = 1013904223;
// uint64_t x;
// inline uint64_t LCG(int min, int max) {
// 	x = (multiplier * x + increment) % mod;
// 	return min + x % (max - min);
// }

uint64_t shuffle_table[2] = {static_cast<uint64_t>(time(NULL)), static_cast<uint64_t>(time(NULL)) >> 16};
uint64_t Xoroshiro128(int min, int max) {
	uint64_t s1 = shuffle_table[0];
	uint64_t s0 = shuffle_table[1];
	uint64_t result = s0 + s1;
	shuffle_table[0] = s0;
	s1 ^= s1 << 23;
	shuffle_table[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
	return min + result % (max - min);
}

// uint64_t t = time(NULL);
// uint64_t SplitMix64(int min, int max) {
// 	uint64_t z = (t += UINT64_C(0x9E3779B97F4A7C15));
// 	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
// 	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
// 	return min + (z ^ (z >> 31)) % (max - min);
// }

void test() {
	float result[100] = {0};
	uint64_t size = 1000000000;

	uint64_t n;
	for (size_t i = 0; i < size; i++) {
		n = Xoroshiro128(0, 100);
		result[n]++;
	}

	for (size_t i = 0; i < 100; i++) {
		cerr << i << " = " << setprecision(3) << result[i] / size * 100 << endl;
	}
}

int main() {

	srand(time(NULL));

	test();

	return 0;
}