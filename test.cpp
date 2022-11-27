#include <iostream>
#include <bitset>

using namespace std;

int main() {

	__int128_t n = 0;

	for (size_t i = 0; i < 128; i++) {
		bitset<128> bit(n | ((static_cast<__int128>(1)) << i));
		cout << bit << endl;
	}

	return 0;
}