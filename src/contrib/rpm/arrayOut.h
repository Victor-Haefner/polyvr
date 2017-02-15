#ifndef ARRAYOUT_H_INCLUDED
#define ARRAYOUT_H_INCLUDED

#include <iostream>
#include <fstream>

class arrayToFile {
public:
  arrayToFile(const char * filename, double* data, uint length){
  ofstream file;
  file.open(filename);
  for (int i = 0; i < length; ++i) file << data[i]<<" ";
  file.close(); };
  arrayToFile(const char * filename, short* data, uint length){
  ofstream file;
  file.open(filename);
  for (int i = 0; i < length; ++i) file << data[i]<<" ";
  file.close(); };
};

#endif
