#pragma once

#include <vector>

namespace MBRF
{

class Utils
{
public:
	static bool ReadFile(const char* fileName, std::vector<char> &fileOut);

	template <class T>
	static inline void HashCombine(std::size_t& seed, const T& v);
};

template <class T>
inline void Utils::HashCombine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}
