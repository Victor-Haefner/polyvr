#pragma once

#include <string>
#include <vector>
#include <istream>

using namespace std;

void getFileCrc(istream& input_stream, vector<char>& buff, unsigned long& result_crc);
bool isLargeFile(istream& input_stream);
