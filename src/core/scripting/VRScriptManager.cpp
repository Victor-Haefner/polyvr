#include "VRScriptManager.h"
#include "core/scene/VRSceneManager.h"
#include "VRScript.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPySocket.h"
#include "VRPySprite.h"
#include "VRPySound.h"
#include "VRPyDevice.h"
#include "VRPyPath.h"
#include "VRPyStroke.h"
#include "VRPyColorChooser.h"
#include "VRPyConstraint.h"
#include "VRPyHaptic.h"
#include "VRPyBaseT.h"
#include "VRPyMaterial.h"
#include "VRPyLod.h"
#include <iostream>

//TODO: refactoring
#include "../gui/VRGuiBits.h"
#include "addons/CSG/VRPyCSG.h"
#include "addons/RealWorld/VRPyRealWorld.h"
#include "addons/RealWorld/traffic/VRPyTrafficSimulation.h"
#include "addons/CaveKeeper/VRPyCaveKeeper.h"
#include "addons/SimViDekont/VRPySimViDekont.h"
#include "addons/Bullet/CarDynamics/VRPyCarDynamics.h"
#include "addons/Factory/VRPyLogistics.h"
#include "addons/Factory/VRPyMechanism.h"
#include "addons/Factory/VRPyNumberingEngine.h"
#include "addons/CEF/VRPyCEF.h"
#include "addons/Classification/VRPySegmentation.h"
//#include "addons/WebKit/VRPyWebKit.h"
#include "core/utils/VRStorage_template.h"
#include "VRPyTypeCaster.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRScriptManager::VRScriptManager() {
    initPyModules();
    storeMap("Script", &scripts);
}

VRScriptManager::~VRScriptManager() {
    for (auto s : scripts) delete s.second;
    scripts.clear();
    Py_XDECREF(pModBase);
    Py_Finalize();
}

void VRScriptManager::disableAllScripts() {
    for (auto s : scripts) s.second->enable(false);
}

VRScript* VRScriptManager::newScript(string name, string core) {
    //if (scripts.count(name) == 0) scripts[name] = new VRScript(name);
    VRScript* script = new VRScript(name);
    name = script->getName();
    scripts[name] = script;
    updateScript(name, core);
    return script;
}

void VRScriptManager::remScript(string name) {
    if (scripts.count(name) == 0) return;
    scripts[name]->clean();
    delete scripts[name];
    scripts.erase(name);
}

void VRScriptManager::update() {
    for (auto s : scripts) updateScript(s.first, s.second->getCore());
}

VRScript* VRScriptManager::changeScriptName(string name, string new_name) {
    map<string, VRScript*>::iterator i = scripts.find(name);
    if (i == scripts.end()) return 0;

    VRScript* script = i->second;
    script->setName(new_name);
    new_name = script->getName();
    scripts.erase(i);
    scripts[new_name] = script;
    return script;
}

map<string, VRScript*> VRScriptManager::getScripts() { return scripts; }
VRScript* VRScriptManager::getScript(string name) { return scripts.count(name) == 1 ? scripts[name] : 0; }
void VRScriptManager::triggerScript(string fkt) { if (scripts.count(fkt) == 1) scripts[fkt]->execute(); }

void VRScriptManager::updateScript(string name, string core, bool compile) {
    if (scripts.count(name) == 0) return;

    VRScript* script = scripts[name];
    script->setCore(core);

    if (!compile) return;
    if (script->getType() == "Python") {
        //PyObject* pValue = PyRun_String(script->getScript().c_str(), Py_file_input, pGlobal, pLocal);
        PyObject* pValue = PyRun_String(script->getScript().c_str(), Py_file_input, pGlobal, PyModule_GetDict(pModVR));
        if (PyErr_Occurred() != NULL) PyErr_Print();

        if (pValue == NULL) return;
        Py_DECREF(pValue);

        //script->setFunction( PyObject_GetAttrString(pModBase, name.c_str()) );
        script->setFunction( PyObject_GetAttrString(pModVR, name.c_str()) );
    }

    if (script->getType() == "HTML") {
        ;
    }
}


// ---------------------------------- Python stuff -----------------------------------------

// intersept python stdout
static PyObject* vte_write(PyObject *self, PyObject *args) {
    const char *what;
    if (!PyArg_ParseTuple(args, "s", &what)) return NULL;
    VRGuiBits::write_to_terminal(what);
    //printf("==%s==", what);
    return Py_BuildValue("");
}

static PyMethodDef so_methods[] = {
    {"write", vte_write, METH_VARARGS, "Write something."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initVRPyStdOut(void) {
    PyObject *m = Py_InitModule("aview", so_methods);
    if (m == NULL) return;
    PySys_SetObject((char *)"stdout", m);
    PySys_SetObject((char *)"stderr", m);
}

// ----------------------------

// TODO:
//  a script has parameters, they are some sort of default parameters with values defined in the gui
//  such a script can be called from another script, but only by passing all the variables again
//  it should be possible to call the script with any parameter, the gui parameter should be used as default ones!

static PyMethodDef VRScriptManager_module_methods[] = {
	{"loadCollada", (PyCFunction)VRScriptManager::loadCollada, METH_VARARGS, "loads a collada file and returns a VR.Geometry node" },
    {NULL}  /* Sentinel */
};

void VRScriptManager::initPyModules() {
    Py_Initialize();
    VRPyBase::err = PyErr_NewException((char *)"VR.Error", NULL, NULL);

    pGlobal = PyDict_New();

    //Create a new module object
    pModBase = PyModule_New("PolyVR_base");
    PyModule_AddStringConstant(pModBase, "__file__", "");
    pLocal = PyModule_GetDict(pModBase); //Get the dictionary object from my module so I can pass this to PyRun_String

    PyDict_SetItemString(pLocal, "__builtins__", PyEval_GetBuiltins());
    PyDict_SetItemString(pGlobal, "__builtins__", PyEval_GetBuiltins());

    pModVR = Py_InitModule3("VR", VRScriptManager_module_methods, "VR Module");
    VRPyObject::registerModule("Object", pModVR);
    VRPyTransform::registerModule("Transform", pModVR, VRPyObject::typeRef);
    VRPyGeometry::registerModule("Geometry", pModVR, VRPyTransform::typeRef);
    //VRPySocket::registerModule("Socket", pModVR);
    VRPySprite::registerModule("Sprite", pModVR, VRPyGeometry::typeRef);
    VRPyRealWorld::registerModule("RealWorld", pModVR);
    VRPyTrafficSimulation::registerModule("TrafficSimulation", pModVR);
    VRPySimViDekont::registerModule("SimViDekont", pModVR);
    VRPyCaveKeeper::registerModule("CaveKeeper", pModVR);
    //VRPySound::registerModule("Sound", pModVR);
    VRPyCarDynamics::registerModule("CarDynamics", pModVR);
    //VRPyPath::registerModule("Path", pModVR);
    VRPyStroke::registerModule("Stroke", pModVR, VRPyObject::typeRef);
    VRPyColorChooser::registerModule("ColorChooser", pModVR);
    VRPyCSG::registerModule("CSGGeometry", pModVR, VRPyGeometry::typeRef);
    VRPyConstraint::registerModule("Constraint", pModVR);
    VRPyDevice::registerModule("Device", pModVR);
    VRPyHaptic::registerModule("Haptic", pModVR, VRPyDevice::typeRef);
    VRPyMaterial::registerModule("Material", pModVR, VRPyObject::typeRef);
    VRPyLod::registerModule("Lod", pModVR, VRPyObject::typeRef);

    VRPySound::registerModule("Sound", pModVR);

    VRPyCEF::registerModule("CEF", pModVR);
    VRPySegmentation::registerModule("Segmentation", pModVR);
    VRPyMechanism::registerModule("Mechanism", pModVR);
    VRPyNumberingEngine::registerModule("NumberingEngine", pModVR, VRPyGeometry::typeRef);

    PyObject* pModFactory = Py_InitModule3("Factory", VRScriptManager_module_methods, "VR Module");
    FPyNode::registerModule("Node", pModFactory);
    FPyNetwork::registerModule("Network", pModFactory);
    FPyPath::registerModule("Path", pModFactory);
    FPyTransporter::registerModule("Transporter", pModFactory);
    FPyContainer::registerModule("Container", pModFactory);
    FPyProduct::registerModule("Product", pModFactory);
    FPyLogistics::registerModule("Logistics", pModFactory);
    PyModule_AddObject(pModVR, "Factory", pModFactory);

    initVRPyPath(pModVR); // TODO
    initVRPySocket(pModVR);

    initVRPyStdOut();

    // add cython local path to python search path
    PyRun_SimpleString(
        "import sys\n"
        "sys.path.append('cython/')\n"
    );
}

vector<string> VRScriptManager::getPyVRTypes() {
    vector<string> res;

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (!PyType_Check(value)) continue;

        string name = PyString_AsString(key);
        res.push_back(name);
    }

    return res;
}

vector<string> VRScriptManager::getPyVRMethods(string type) {
    vector<string> res;

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (!PyType_Check(value)) continue;
        string name = PyString_AsString(key);
        if (name != type) continue;
        break;
    }

    // value is the type object
    PyTypeObject* t = (PyTypeObject*)value;
    dict = t->tp_dict; pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        //if (!PyMethod_Check(value)) continue;
        string name = PyString_AsString(key);
        if (name[0] == '_' and name[1] == '_') continue;
        res.push_back(name);
    }

    return res;
}

string VRScriptManager::getPyVRMethodDoc(string type, string method) {
    string res;

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *tp, *meth;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &tp)) {
        if (!PyType_Check(tp)) continue;
        string name = PyString_AsString(key);
        if (name != type) continue;
        break;
    }

    // tp is the type object
    PyTypeObject* t = (PyTypeObject*)tp;
    dict = t->tp_dict; pos = 0;
//PyMethodDescrObject::d_method::
    while (PyDict_Next(dict, &pos, &key, &meth)) {
        string name = PyString_AsString(key);
        if (method != name) continue;

        string ty = meth->ob_type->tp_name;
        //cout << "\nMethod type " << ty << endl;
        if (ty != "method_descriptor") continue;

        PyMethodDescrObject* md = (PyMethodDescrObject*)meth;
        res = md->d_method->ml_doc;
        break;
    }

    // meth is the method
    ;

    return res;
}

// ==============
// Python methods
// ==============

PyObject* VRScriptManager::loadCollada(VRScriptManager* self, PyObject *args) {
    //if (self->obj == 0) { PyErr_SetString(err, "VRScriptManager::loadCollada - Object is invalid"); return NULL; }

	PyObject* pyPath;
    if (! PyArg_ParseTuple(args, "O", &pyPath)) return NULL;
    string path = PyString_AsString(pyPath);
    VRTransform *obj = VRSceneLoader::get()->load3DContent(path, 0);
    return VRPyTypeCaster::cast(obj);
}

OSG_END_NAMESPACE
