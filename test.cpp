#include <iostream>
#include <bitset>

using namespace std;

#define int128(x) static_cast<__int128_t>(x)

template<class Mask>
string mtos(Mask mask, size_t size) {
	string str;
	for (size_t i = 0; i < size; i++) {
		str.insert(0, to_string(int(mask & 1)));
		mask >>= 1;
	}
	return str;
}

uint_fast8_t int128_ffs(__int128_t n) {
	const __int64_t lo = n >> 64;
	const __int64_t hi = n;
	const uint_fast8_t cnt_lo = __builtin_ffsll(lo);
	const uint_fast8_t cnt_hi = __builtin_ffsll(hi);
	return cnt_lo ? cnt_lo : cnt_hi;
}

int main() {

	// __uint128_t n = ((int128(0x1ffffffffff) << 40) | int128(0xffffffffff));
	__uint128_t n = ~(int128(0x7fffffffffff) << 81);

	cerr << mtos(n, 128) << endl;

	return 0;
}