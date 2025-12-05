#include "memoization.hpp"
#include <chrono>
#include <iostream>
#include <vector>

using namespace opt_utils;
using namespace std::chrono;
typedef unsigned long long ull;
unsigned long long sum(unsigned long long x) {
	unsigned long long r = 0;
	for (unsigned long long i = 0; i <= x; i++) {
		r += i;
	}
	return r;
}

int main() {

	std::vector<ull> vec;
	memoization<ull(ull), unordered> foo = {sum};

	const ull i = 10000000000;
	{
		auto start = steady_clock::now();
		std::cout << i << " : " << foo(i) << "\n";
		auto end = steady_clock::now();
		auto elapsed = duration_cast<milliseconds>(end - start).count();
		std::cout << "최초 실행 :::: " << elapsed << std::endl;
	}
	{
		auto start = steady_clock::now();
		std::cout << i << " : " << foo(i) << "\n";
		auto end = steady_clock::now();
		auto elapsed = duration_cast<milliseconds>(end - start).count();
		std::cout << "2번째 실행 :::: " << elapsed << std::endl;
	}
	return 0;
}
