#include "VRPyCodeCompletion.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScript.h"
#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "VRPyBaseT.h"

#include <string.h>
#include <iostream>

using namespace OSG;

simpleVRPyType(Script, 0);

PyMethodDef VRPyScript::methods[] = {
    {"getScript", PyWrap(Script, getScript, "Get script content as string", string ) },
    {NULL}  /* Sentinel */
};



VRPyCodeCompletion::VRPyCodeCompletion() {}
VRPyCodeCompletion::~VRPyCodeCompletion() {}

PyObject* VRPyCodeCompletion::getObject(string name, PyObject* parent) {
    if (name == "") return 0;
    auto scene = VRScene::getCurrent();
    if (!scene) return 0;
    if (name == "VR" && parent == 0) return scene->getGlobalModule();
    for (auto m : getMembers(parent)) {
        if (m.first == name) return m.second;
    }
    return 0;
}

PyObject* VRPyCodeCompletion::resolvePath(vector<string>& path) {
    vector<PyObject*> oPath;
    PyObject* obj = 0;
    for (auto p : path) { obj = getObject(p, obj); if (obj) oPath.push_back(obj); }
    return oPath.size() ? oPath.back() : 0;
}

map<string, PyObject*> VRPyCodeCompletion::getMembers(PyObject* obj) {
    map<string, PyObject*> res;
    if (!obj) return res;

    PyObject* dict = 0;
    if (PyModule_Check(obj)) dict = PyModule_GetDict(obj);
    if (PyType_Check(obj)) dict = ((PyTypeObject*)obj)->tp_dict;

    if (!dict) {
        cout << " VRPyCodeCompletion::getMembers no dict!! type: " << obj->ob_type->tp_name << endl;
        return res;
    }
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        string name = PyString_AsString(key);
        if (startsWith(name, "__")) continue;
        res[name] = value;
    }

    return res;
}

bool VRPyCodeCompletion::startsWith(const string& a, const string& b) {
    return a.substr(0, b.size()) == b;
}

vector<string> VRPyCodeCompletion::getSuggestions(string input) {
    vector<string> res;
	if (input.size() <= 2) return res; // limit to min 2 chars!

    auto path = splitString(input, '.');
    if (input.back() == '.') path.push_back("");
    int N = path.size();
    if (N == 0) return res;

    PyObject* obj = resolvePath(path);
	if (!obj) return res;

    string guess = path.back();
    for (auto m : getMembers(obj)) if (startsWith(m.first, guess)) res.push_back(m.first);
    return res;
}

// jedi sucks, not used!
vector<string> VRPyCodeCompletion::getJediSuggestions(VRScriptPtr script, int line, int column) {
    vector<string> res;

    line += 1;

    cout << "VRPyCodeCompletion::getJediSuggestions " << script->getName() << "  l " << line << "  c " << column << endl;

    if (!jediWrap) {
        auto scene = VRScene::getCurrent();
        string jediScript = "def jediScript(script,l,c):\n\timport jedi\n\treturn [ c.name for c in jedi.Script(script, l, c, '').completions() ]";
        PyObject* pCode = Py_CompileString(jediScript.c_str(), "jediScript", Py_file_input);
        if (!pCode) { PyErr_Print(); return res; }
        auto pModVR = scene->getGlobalModule();
        PyObject* pValue = PyEval_EvalCode((PyCodeObject*)pCode, scene->getGlobalDict(), PyModule_GetDict(pModVR));
        if (!pValue) { PyErr_Print(); return res; }
        Py_DECREF(pCode);
        Py_DECREF(pValue);
        jediWrap = PyObject_GetAttrString(pModVR, "jediScript");
    }

    if (jediWrap) {
        string data = script->getScript();
        PyGILState_STATE gstate = PyGILState_Ensure();
        auto args = PyTuple_New(3);
        PyTuple_SetItem(args, 0, PyString_FromString(data.c_str()));
        PyTuple_SetItem(args, 1, PyInt_FromLong(line));
        PyTuple_SetItem(args, 2, PyInt_FromLong(column));
        auto r = PyObject_CallObject(jediWrap, args);
        if (!r) { PyErr_Print(); return res; }
        for (int i=0; i<PyList_Size(r); i++) {
            auto c = PyList_GetItem(r,i);
            res.push_back(PyString_AsString(c));
        }
        PyGILState_Release(gstate);
    }

    return res;
}




/**

TODO:
- get the py object at the path end (the one before the last '.')

*/


