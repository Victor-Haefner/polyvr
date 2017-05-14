#include "VRScriptManager.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VROptions.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#include "VRScript.h"
#include "VRSceneModules.h"
#include "VRSceneGlobals.h"
#include "VRPyListMath.h"

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <iostream>
#include <algorithm>
#include <memory>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRScriptManager::VRScriptManager() {
    initPyModules();

    setStorageType("Scripts");
    storeMap("Script", &scripts);
}

VRScriptManager::~VRScriptManager() {
    //cout << "VRScriptManager destroyed\n";
    blockScriptThreads();
    scripts.clear();
    if (PyErr_Occurred() != NULL) PyErr_Print();
    //Py_XDECREF(pModBase);
    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyErr_Clear();
    VRPyBase::err = 0;
    Py_Finalize();
}

void VRScriptManager::disableAllScripts() {
    for (auto s : scripts) s.second->enable(false);
}

VRScriptPtr VRScriptManager::newScript(string name, string core) {
    VRScriptPtr script = VRScript::create(name);
    script->setCore(core);
    //cout << "VRScriptManager::newScript A\n";
    addScript( script );
    //cout << "VRScriptManager::newScript B\n";
    return script;
}

void VRScriptManager::addScript(VRScriptPtr script) {
    string name = script->getName();
    scripts[name] = script;
    updateScript(name, script->getCore());
}

void VRScriptManager::remScript(string name) {
    if (scripts.count(name) == 0) return;
    scripts[name]->clean();
    scripts.erase(name);
}

vector<VRScriptPtr> VRScriptManager::searchScript(string s, VRScriptPtr sc) {
    vector<VRScriptPtr> res;
    VRScript::Search search;

    for (auto sc : scripts) sc.second->find(""); // clear old search results

    if (sc) {
        search = sc->find(s);
        if (search.N) res.push_back(sc);
        return res;
    }

    for (auto sc : scripts) {
        search = sc.second->find(s);
        if (search.N) res.push_back(sc.second);
    }

    return res;
}

void VRScriptManager::update() {
    //blockScriptThreads();
    for (auto s : scripts) updateScript(s.first, s.second->getCore());
    //allowScriptThreads();
}

VRScriptPtr VRScriptManager::changeScriptName(string name, string new_name) {
    map<string, VRScriptPtr>::iterator i = scripts.find(name);
    if (i == scripts.end()) return 0;

    VRScriptPtr script = i->second;
    script->setName(new_name);
    new_name = script->getName();
    scripts.erase(i);
    scripts[new_name] = script;
    return script;
}

map<string, VRScriptPtr> VRScriptManager::getScripts() { return scripts; }
VRScriptPtr VRScriptManager::getScript(string name) { return scripts.count(name) == 1 ? scripts[name] : 0; }
void VRScriptManager::triggerScript(string fkt) { if (scripts.count(fkt) == 1) scripts[fkt]->execute(); }

void VRScriptManager::updateScript(string name, string core, bool compile) {
    if (scripts.count(name) == 0) return;

    VRScriptPtr script = scripts[name];
    script->setCore(core);

    if (!compile) return;
    if (script->getType() == "Python") {
        //PyGILState_STATE gstate = PyGILState_Ensure();
        //PyObject* pValue = PyRun_String(script->getScript().c_str(), Py_file_input, pGlobal, pLocal);
        PyObject* pValue = PyRun_String(script->getScript().c_str(), Py_file_input, pGlobal, PyModule_GetDict(pModVR));
//    cout << "VRScriptManager::updateScript A\n";
        if (PyErr_Occurred() != NULL) PyErr_Print();
//    cout << "VRScriptManager::updateScript B\n";

        if (pValue == NULL) return;
        Py_DECREF(pValue);

        //script->setFunction( PyObject_GetAttrString(pModBase, name.c_str()) );
        script->setFunction( PyObject_GetAttrString(pModVR, name.c_str()) );
        //PyGILState_Release(gstate);
    }
}


// ---------------------------------- Python stuff -----------------------------------------

// intersept python stdout
static PyObject* writeOut(PyObject *self, PyObject *args) {
    const char *what;
    if (!PyArg_ParseTuple(args, "s", &what)) return NULL;
    VRGuiManager::get()->getConsole("Console")->write(what);
    return Py_BuildValue("");
}

static PyObject* writeErr(PyObject *self, PyObject *args) {
    const char *what;
    if (!PyArg_ParseTuple(args, "s", &what)) return NULL;
    VRGuiManager::get()->getConsole("Errors")->write(what);
    return Py_BuildValue("");
}

static PyMethodDef methOut[] = {
    {"write", writeOut, METH_VARARGS, "Write something."},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef methErr[] = {
    {"write", writeErr, METH_VARARGS, "Write something."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initVRPyStdOut(void) {
    PyObject *mOut = Py_InitModule("pyOut", methOut);
    PyObject *mErr = Py_InitModule("pyErr", methErr);
    if (mOut) PySys_SetObject((char *)"stdout", mOut);
    if (mErr) PySys_SetObject((char *)"stderr", mErr);
}

// ----------------------------

// TODO:
//  a script has parameters, they are some sort of default parameters with values defined in the gui
//  such a script can be called from another script, but only by passing all the variables again
//  it should be possible to call the script with any parameter, the gui parameter should be used as default ones!

void VRScriptManager::initPyModules() {
    Py_Initialize();
    PyEval_InitThreads();
    VRPyBase::err = PyErr_NewException((char *)"VR.Error", NULL, NULL);

    pGlobal = PyDict_New();

    //Create a new module object
    pModBase = PyImport_AddModule("PolyVR_base");
    PyModule_AddStringConstant(pModBase, "__file__", "");
    pLocal = PyModule_GetDict(pModBase); //Get the dictionary object from my module so I can pass this to PyRun_String

    PyDict_SetItemString(pLocal, "__builtins__", PyEval_GetBuiltins());
    PyDict_SetItemString(pGlobal, "__builtins__", PyEval_GetBuiltins());
#ifndef _WIN32
    VRPyListMath::init(pModBase);
#endif

    PyObject* sys_path = PySys_GetObject((char*)"path");
    PyList_Append(sys_path, PyString_FromString(".") );

    //string sys_path = PyString_AsString(PySys_GetObject("path"));
    //sys_path += ":.";
    //PySys_SetPath(sys_path.c_str());

    pModVR = Py_InitModule3("VR", VRSceneGlobals::methods, "VR Module");

    VRSceneModules sceneModules;
    sceneModules.setup(this, pModVR);

	if (!VROptions::get()->getOption<bool>("standalone")) initVRPyStdOut();

    // add cython local path to python search path
    PyRun_SimpleString(
        "import sys\n"
        "sys.path.append('cython/')\n"
    );

    //PyEval_ReleaseLock();
    //PyEval_SaveThread();
}

vector<string> VRScriptManager::getPyVRModules() {
    vector<string> res;
    res.push_back("VR");
    for (auto m : modules) if (m.first != "VR") res.push_back(m.first);
    sort (res.begin()+1, res.end());
    return res;
}

vector<string> VRScriptManager::getPyVRTypes(string mod) {
    vector<string> res;
    if (modules.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return res; }
    res.push_back("globals");
    for (auto m : modules[mod]) res.push_back(m.first);
    sort (res.begin()+1, res.end());
    return res;
}

string VRScriptManager::getPyVRDescription(string mod, string type) {
    if (type == "globals") return "";
    if (modules.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return ""; }
    if (modules[mod].count(type) == 0) { cout << "Method " << type << " not found\n"; return ""; }
    return modules[mod][type]->tp_doc;
}

vector<string> VRScriptManager::getPyVRMethods(string mod, string type) {
    vector<string> res;
    if (modules.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return res; }
    if (modules[mod].count(type) == 0 && type != "globals") { cout << "Method " << type << " not found\n"; return res; }
    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    if (type == "globals") {
        if (mod != "VR") return res;
        while (PyDict_Next(dict, &pos, &key, &value)) {
            string name = PyString_AsString(key);
            if (name[0] == '_' && name[1] == '_') continue;
            if (PyCFunction_Check(value)) res.push_back(name);
        }

        sort (res.begin(), res.end());
        return res;
    }

    dict = modules[mod][type]->tp_dict;
    pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        string name = PyString_AsString(key);
        if (name[0] == '_' && name[1] == '_') continue;
        res.push_back(name);
    }

    sort (res.begin(), res.end());
    return res;
}

string VRScriptManager::getPyVRMethodDoc(string mod, string type, string method) {
    if (modules.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return ""; }
    if (modules[mod].count(type) == 0 && type != "globals") { cout << "Method " << type << " not found\n"; return ""; }
    string res;

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *tp, *meth;
    Py_ssize_t pos = 0;

    if (type == "globals") {
        if (mod != "VR") return res;
        while (PyDict_Next(dict, &pos, &key, &meth)) {
            string name = PyString_AsString(key);
            if (method != name) continue;
            if (!PyCFunction_Check(meth)) continue;
            PyCFunctionObject* cfo =  (PyCFunctionObject*)meth;
            return cfo->m_ml->ml_doc;
        }
    }

    pos = 0;
    dict = modules[mod][type]->tp_dict;
    while (PyDict_Next(dict, &pos, &key, &meth)) {
        string name = PyString_AsString(key);
        if (method != name) continue;

        string ty = meth->ob_type->tp_name;
        if (ty != "method_descriptor") continue;

        PyMethodDescrObject* md = (PyMethodDescrObject*)meth;
        res = md->d_method->ml_doc;
        break;
    }

    return res;
}

void VRScriptManager::allowScriptThreads() {
    if (pyThreadState == 0) pyThreadState = PyEval_SaveThread();
}

void VRScriptManager::blockScriptThreads() {
    if (pyThreadState) { PyEval_RestoreThread(pyThreadState); pyThreadState = 0; }
}

OSG_END_NAMESPACE
