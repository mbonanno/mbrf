#include "utils.h"

#include <assert.h>
#include <iostream>
#include <fstream>

namespace MBRF
{

bool Utils::ReadFile(const char* fileName, std::vector<char> &fileOut)
{
	std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		std::cout << "failed to open file " << fileName << std::endl;
		assert(0);
		return false;
	}

	// std::ios::ate: open file at the end, convenient to get the file size
	size_t size = (size_t)file.tellg();
	fileOut.resize(size);

	file.seekg(0, std::ios::beg);
	file.read(fileOut.data(), size);

	file.close();

	return true;
}

}