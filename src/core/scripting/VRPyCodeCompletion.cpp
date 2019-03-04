#include "VRPyCodeCompletion.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "VRPyBaseT.h"

#include <string.h>
#include <iostream>

using namespace std;
using namespace OSG;

VRPyCodeCompletion::VRPyCodeCompletion() {}
VRPyCodeCompletion::~VRPyCodeCompletion() {}

PyObject* VRPyCodeCompletion::resolvePath(vector<string>& path) { // TODO
    string mod = "VR";
    string member = "";
    int N = path.size();
    if (N >= 2) mod = path[N-2];
    return getObject(mod);

    string objPath;
    for (uint i=0; i<path.size()-1; i++) objPath += path[i] + "."; // TODO
}

PyObject* VRPyCodeCompletion::getObject(string name) { // TODO
    if (objects.count(name)) return objects[name];
    auto scene = OSG::VRScene::getCurrent();
    if (!scene) return 0;
    auto mod = scene->getPyModule(name);
    if (mod) objects[name] = mod;
    return mod;
}

vector<string> VRPyCodeCompletion::getMembers(PyObject* obj) {
    vector<string> res;
    if (!obj) return res;
    if (members.count(obj)) return members[obj];

    PyObject* dict = PyModule_GetDict(obj);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        string name = PyString_AsString(key);
        if (startsWith(name, "__")) continue;
        res.push_back(name);
    }

    sort (res.begin(), res.end());
    members[obj] = res;
    return res;
}

bool VRPyCodeCompletion::startsWith(const string& a, const string& b) {
    return a.substr(0, b.size()) == b;
}

vector<string> VRPyCodeCompletion::getSuggestions(string input) {
    vector<string> res;
	if (input.size() <= 2) return res;

    auto path = splitString(input, '.');
    if (input[input.size()-1] == '.') path.push_back("");
    int N = path.size();
    if (N == 0) return res;

    PyObject* obj = resolvePath(path);
	if (!obj) return res;

    string guess = path[N-1];
    for (auto m : getMembers(obj)) {
        if (startsWith(m, guess)) res.push_back(m);
    }
    return res;
}




simpleVRPyType(Script, 0);

PyMethodDef VRPyScript::methods[] = {
    {"getScript", PyWrap(Script, getScript, "Get script content as string", string ) },
    {NULL}  /* Sentinel */
};


/**

TODO:
- get the py object at the path end (the one before the last '.')

*/


