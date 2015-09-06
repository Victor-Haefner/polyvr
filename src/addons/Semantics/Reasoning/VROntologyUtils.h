#ifndef VRONTOLOGYUTILS_H_INCLUDED
#define VRONTOLOGYUTILS_H_INCLUDED

#include <string>

using namespace std;

int guid();

struct VRNamedID {
    string name;
    int ID;
    VRNamedID();
};

#endif
