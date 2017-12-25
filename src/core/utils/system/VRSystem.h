#ifndef VRSYSTEM_H_INCLUDED
#define VRSYSTEM_H_INCLUDED

#include <string>

using namespace std;

bool exists(string path);
bool makedir(string path);
bool removeFile(string path);
string canonical(string path);

int systemCall(string cmd);
bool compileCodeblocksProject(string path);

#endif // VRSYSTEM_H_INCLUDED
