#ifndef VRCODECOMPLETION_H_INCLUDED
#define VRCODECOMPLETION_H_INCLUDED

#include <map>
#include <vector>
#include "VRPyBase.h"

using namespace std;

class VRPyCodeCompletion {
    private:
        map<PyObject*, vector<string>> members;
        map<string, PyObject*> objects;

        bool startsWith(const string& a, const string& b);

        PyObject* getObject(string);
        PyObject* resolvePath(vector<string>& path);
        vector<string> getMembers(PyObject* obj);

	public:
        VRPyCodeCompletion();
        ~VRPyCodeCompletion();

        vector<string> getSuggestions(string s);
};

#endif // VRCODECOMPLETION_H_INCLUDED
