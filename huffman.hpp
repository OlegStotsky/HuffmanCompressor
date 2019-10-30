#pragma once

#include <cstddef>
#include <fstream>
#include <vector>

void compress(std::ifstream &in, std::ofstream &out, bool verbose);

void decompress(std::ifstream &in, std::ofstream &out, bool verbose);
