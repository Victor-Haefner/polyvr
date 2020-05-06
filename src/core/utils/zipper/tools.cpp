#include "tools.h"
#include "defs.h"
#include <algorithm>
#include <iterator>

#include "filesystem.h"

#include <cstdio>
#include <sys/types.h>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif

void getFileCrc(istream& input_stream, vector<char>& buff, unsigned long& result_crc) {
    unsigned long calculate_crc = 0;
    unsigned long size_read = 0;
    unsigned long total_read = 0;

    do {
      input_stream.read(buff.data(), buff.size());
      size_read = (unsigned long)input_stream.gcount();
      if (size_read > 0) calculate_crc = crc32(calculate_crc, (const unsigned char*)buff.data(), size_read);
      total_read += size_read;
    } while (size_read > 0);

    input_stream.seekg(0);
    result_crc = calculate_crc;
}

bool isLargeFile(istream& input_stream) {
    ZPOS64_T pos = 0;
    input_stream.seekg(0, ios::end);
    pos = input_stream.tellg();
    input_stream.seekg(0);
    return pos >= 0xffffffff;
}




