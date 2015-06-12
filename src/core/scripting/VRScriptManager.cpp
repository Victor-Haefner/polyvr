#include "VRScriptManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/import/VRImport.h"
#include "core/scene/VRAnimationManagerT.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VROptions.h"
#include "VRScript.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyAnimation.h"
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
#include "VRPyTextureGenerator.h"
#include "VRPyLight.h"
#include "VRPyLod.h"
#include "VRPyRecorder.h"
#include "VRPyPathtool.h"
#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPySelector.h"
#include "VRPyMenu.h"
#include "VRPyClipPlane.h"
#include "VRPyListMath.h"
#include "VRPySetup.h"
#include "VRPyNavigator.h"
#include "VRPyNavPreset.h"
#include "VRPyImage.h"
#include <iostream>
#include <algorithm>

//TODO: refactoring
#include "core/gui/VRGuiFile.h"
#include "core/gui/VRGuiManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "addons/CaveKeeper/VRPyCaveKeeper.h"
#include "addons/Bullet/Particles/VRPyParticles.h"
#include "addons/Bullet/CarDynamics/VRPyCarDynamics.h"
#include "addons/Engineering/Factory/VRPyLogistics.h"
#include "addons/Engineering/Factory/VRPyProduction.h"
#include "addons/Engineering/Factory/VRPyAMLLoader.h"
#include "addons/Engineering/Mechanics/VRPyMechanism.h"
#include "addons/Engineering/VRPyNumberingEngine.h"
#include "addons/CEF/VRPyCEF.h"
#include "addons/CEF/VRPyWebCam.h"
#include "addons/Classification/VRPySegmentation.h"
#include "addons/Engineering/Chemistry/VRPyMolecule.h"
#include "addons/Engineering/Factory/VRPyFactory.h"
#include "addons/Engineering/Milling/VRPyMillingMachine.h"
#include "addons/RealWorld/nature/VRPyTree.h"
#include "VRPyTypeCaster.h"
#include "PolyVR.h"

// not yet ported dependencies
#ifndef _WIN32
#include "addons/Engineering/CSG/VRPyCSG.h"
#include "addons/RealWorld/VRPyRealWorld.h"
#include "addons/RealWorld/traffic/VRPyTrafficSimulation.h"
#include "addons/SimViDekont/VRPySimViDekont.h"
#endif

OSG_BEGIN_NAMESPACE;
using namespace std;

VRScriptManager::VRScriptManager() {
    initPyModules();
    storeMap("Script", &scripts);
}

VRScriptManager::~VRScriptManager() {
    for (auto s : scripts) delete s.second;
    scripts.clear();
    if (PyErr_Occurred() != NULL) PyErr_Print();
    //Py_XDECREF(pModBase);
    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyErr_Clear();
    Py_Finalize();
}

void VRScriptManager::disableAllScripts() {
    for (auto s : scripts) s.second->enable(false);
}

VRScript* VRScriptManager::newScript(string name, string core) {
    VRScript* script = new VRScript(name);
    script->setCore(core);
    addScript( script );
    return script;
}

void VRScriptManager::addScript(VRScript* script) {
    string name = script->getName();
    scripts[name] = script;
    updateScript(name, script->getCore());
}

void VRScriptManager::remScript(string name) {
    if (scripts.count(name) == 0) return;
    scripts[name]->clean();
    delete scripts[name];
    scripts.erase(name);
}

vector<VRScript*> VRScriptManager::searchScript(string s, VRScript* sc) {
    vector<VRScript*> res;
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
    VRGuiManager::get()->printInfo(what);
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
	{"exit", (PyCFunction)VRScriptManager::exit, METH_NOARGS, "Terminate application" },
	{"loadGeometry", (PyCFunction)VRScriptManager::loadGeometry, METH_VARARGS, "Loads a collada file and returns a VR.Object node - obj loadGeometry('myPath', bool ignore_cache)" },
	{"stackCall", (PyCFunction)VRScriptManager::stackCall, METH_VARARGS, "Schedules a call to a python function - stackCall( function, delay, [args] )" },
	{"openFileDialog", (PyCFunction)VRScriptManager::openFileDialog, METH_VARARGS, "Open a file dialog - openFileDialog( onLoad, mode, title, default_path, filter )\n mode : {Save, Load, New, Create}" },
	{"updateGui", (PyCFunction)VRScriptManager::updateGui, METH_NOARGS, "Update the gui" },
	{"render", (PyCFunction)VRScriptManager::render, METH_NOARGS, "Renders the viewports" },
	{"triggerScript", (PyCFunction)VRScriptManager::pyTriggerScript, METH_VARARGS, "Trigger a script - triggerScript( str script )" },
	{"getRoot", (PyCFunction)VRScriptManager::getRoot, METH_NOARGS, "Return the root node of the scenegraph - object getRoot()" },
	{"printOSG", (PyCFunction)VRScriptManager::printOSG, METH_NOARGS, "Print the OSG tree to console" },
	{"getNavigator", (PyCFunction)VRScriptManager::getNavigator, METH_NOARGS, "Return a handle to the navigator object" },
	{"getSetup", (PyCFunction)VRScriptManager::getSetup, METH_NOARGS, "Return a handle to the active hardware setup" },
	{"loadScene", (PyCFunction)VRScriptManager::loadScene, METH_VARARGS, "Close the current scene and open another - loadScene( str path/to/my/scene.xml )" },
    {NULL}  /* Sentinel */
};

void VRScriptManager::initPyModules() {
    Py_Initialize();
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

    pModVR = Py_InitModule3("VR", VRScriptManager_module_methods, "VR Module");
    VRPyObject::registerModule("Object", pModVR);
    VRPyTransform::registerModule("Transform", pModVR, VRPyObject::typeRef);
    VRPyGeometry::registerModule("Geometry", pModVR, VRPyTransform::typeRef);
    VRPyMaterial::registerModule("Material", pModVR, VRPyObject::typeRef);
    VRPyTextureGenerator::registerModule("TextureGenerator", pModVR);
    VRPyImage::registerModule("Image", pModVR);
    VRPyLight::registerModule("Light", pModVR, VRPyObject::typeRef);
    VRPyLod::registerModule("Lod", pModVR, VRPyObject::typeRef);
    VRPySprite::registerModule("Sprite", pModVR, VRPyGeometry::typeRef);
    VRPySound::registerModule("Sound", pModVR);
    VRPySocket::registerModule("Socket", pModVR);
    VRPyStroke::registerModule("Stroke", pModVR, VRPyGeometry::typeRef);
    VRPyConstraint::registerModule("Constraint", pModVR);
    VRPyDevice::registerModule("Device", pModVR);
    VRPyHaptic::registerModule("Haptic", pModVR, VRPyDevice::typeRef);
    VRPyAnimation::registerModule("Animation", pModVR);
    VRPyPath::registerModule("Path", pModVR);
    VRPyRecorder::registerModule("Recorder", pModVR);
    VRPySnappingEngine::registerModule("SnappingEngine", pModVR);
    VRPyConstructionKit::registerModule("ConstructionKit", pModVR);
    VRPyPathtool::registerModule("Pathtool", pModVR);
    VRPySelector::registerModule("Selector", pModVR);
    VRPySetup::registerModule("Setup", pModVR);
    VRPyNavigator::registerModule("Navigator", pModVR);
    VRPyNavPreset::registerModule("NavPreset", pModVR);

    VRPyMenu::registerModule("Menu", pModVR, VRPyGeometry::typeRef);
    VRPyClipPlane::registerModule("ClipPlane", pModVR, VRPyGeometry::typeRef);
	VRPyColorChooser::registerModule("ColorChooser", pModVR);
    VRPyCaveKeeper::registerModule("CaveKeeper", pModVR);
    VRPyParticles::registerModule("Particles", pModVR, VRPyGeometry::typeRef);
    VRPyCarDynamics::registerModule("CarDynamics", pModVR);
    VRPyCEF::registerModule("CEF", pModVR);
    VRPyWebCam::registerModule("Webcam", pModVR, VRPySprite::typeRef);
    VRPySegmentation::registerModule("Segmentation", pModVR);
    VRPyMechanism::registerModule("Mechanism", pModVR);
    VRPyNumberingEngine::registerModule("NumberingEngine", pModVR, VRPyGeometry::typeRef);
    VRPyTree::registerModule("Tree", pModVR, VRPyGeometry::typeRef);
    VRPyMillingMachine::registerModule("MillingMachine", pModVR);
    VRPyMolecule::registerModule("Molecule", pModVR, VRPyGeometry::typeRef);

#ifndef _WIN32
	VRPyCSG::registerModule("CSGGeometry", pModVR, VRPyGeometry::typeRef);
	VRPyRealWorld::registerModule("RealWorld", pModVR);
	VRPyTrafficSimulation::registerModule("TrafficSimulation", pModVR);
	VRPySimViDekont::registerModule("SimViDekont", pModVR);
#endif

    PyObject* pModFactory = Py_InitModule3("Factory", VRScriptManager_module_methods, "VR Module");
    FPyNode::registerModule("Node", pModFactory);
    FPyNetwork::registerModule("Network", pModFactory);
    FPyPath::registerModule("Path", pModFactory);
    FPyTransporter::registerModule("Transporter", pModFactory);
    FPyContainer::registerModule("Container", pModFactory);
    FPyProduct::registerModule("Product", pModFactory);
    FPyLogistics::registerModule("Logistics", pModFactory);
    VRPyFactory::registerModule("Factory", pModFactory);
    VRPyProduction::registerModule("Production", pModFactory);
    VRPyAMLLoader::registerModule("AMLLoader", pModFactory);
    PyModule_AddObject(pModVR, "Factory", pModFactory);

	if (!VROptions::get()->getOption<bool>("standalone")) initVRPyStdOut();

    // add cython local path to python search path
    PyRun_SimpleString(
        "import sys\n"
        "sys.path.append('cython/')\n"
    );
}

vector<string> VRScriptManager::getPyVRTypes() {
    vector<string> res;
    res.push_back("VR globals");

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (!PyType_Check(value)) continue;

        string name = PyString_AsString(key);
        res.push_back(name);
    }

    sort (res.begin()+1, res.end());
    return res;
}

vector<string> VRScriptManager::getPyVRMethods(string type) {
    vector<string> res;
    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    if (type == "VR globals") {
        while (PyDict_Next(dict, &pos, &key, &value)) {
            string name = PyString_AsString(key);
            if (name[0] == '_' && name[1] == '_') continue;
            if (PyCFunction_Check(value)) res.push_back(name);
        }

        sort (res.begin(), res.end());
        return res;
    }

    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (!PyType_Check(value)) continue;
        string name = PyString_AsString(key);
        if (name != type) continue;
        PyTypeObject*t = (PyTypeObject*)value;
        dict = t->tp_dict;
        break;
    }

    pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        string name = PyString_AsString(key);
        if (name[0] == '_' && name[1] == '_') continue;
        res.push_back(name);
    }

    sort (res.begin(), res.end());
    return res;
}

string VRScriptManager::getPyVRMethodDoc(string type, string method) {
    string res;

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *tp, *meth;
    Py_ssize_t pos = 0;

    if (type == "VR globals") {
        while (PyDict_Next(dict, &pos, &key, &meth)) {
            string name = PyString_AsString(key);
            if (method != name) continue;
            if (!PyCFunction_Check(meth)) continue;
            PyCFunctionObject* cfo =  (PyCFunctionObject*)meth;
            return cfo->m_ml->ml_doc;
        }
    }

    while (PyDict_Next(dict, &pos, &key, &tp)) {
        if (!PyType_Check(tp)) continue;
        string name = PyString_AsString(key);
        if (name != type) continue;
        break;
    }

    PyTypeObject* t = (PyTypeObject*)tp;
    dict = t->tp_dict; pos = 0;
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

// ==============
// Python methods
// ==============

PyObject* VRScriptManager::loadScene(VRScriptManager* self, PyObject *args) {
    auto fkt = new VRFunction<int>( "scheduled scene load", boost::bind(&VRSceneManager::loadScene, VRSceneManager::get(), parseString(args), false ) );
    VRSceneManager::get()->queueJob(fkt);
    Py_RETURN_TRUE;
}

PyObject* VRScriptManager::getSetup(VRScriptManager* self) {
    return VRPySetup::fromPtr(VRSetupManager::getCurrent());
}

PyObject* VRScriptManager::getNavigator(VRScriptManager* self) {
    return VRPyNavigator::fromPtr((VRNavigator*)VRSceneManager::getCurrent());
}

PyObject* VRScriptManager::printOSG(VRScriptManager* self) {
    VRObject::printOSGTree( VRSceneManager::getCurrent()->getRoot()->getNode() );
    VRSetupManager::getCurrent()->printOSG();
    Py_RETURN_TRUE;
}

PyObject* VRScriptManager::exit(VRScriptManager* self) {
    exitPolyVR();
    Py_RETURN_TRUE;
}

PyObject* VRScriptManager::getRoot(VRScriptManager* self) {
    return VRPyTypeCaster::cast( VRSceneManager::getCurrent()->getRoot() );
}

PyObject* VRScriptManager::loadGeometry(VRScriptManager* self, PyObject *args) {
    PyObject* path = 0;
    PyObject *preset = 0;
    int ignoreCache = 0;

    if (pySize(args) == 1) if (! PyArg_ParseTuple(args, "O", &path)) return NULL;
    if (pySize(args) == 2) if (! PyArg_ParseTuple(args, "Oi", &path, &ignoreCache)) return NULL;
    if (pySize(args) == 3) if (! PyArg_ParseTuple(args, "OiO", &path, &ignoreCache, &preset)) return NULL;
    if (pySize(args) < 1 || pySize(args) > 3) { PyErr_SetString(err, "VRScriptManager::loadGeometry: wrong number of arguments"); return NULL; }

    string p = PyString_AsString(path);
    string pre = "OSG";
    if (preset) pre = PyString_AsString(preset);

    VRTransform* obj = VRImport::get()->load( p, 0, ignoreCache, pre);
    if (obj == 0) {
        VRGuiManager::get()->printInfo("Warning: " + p + " not found.\n");
        Py_RETURN_NONE;
    }
    obj->addAttachment("dynamicaly_generated", 0);
    return VRPyTypeCaster::cast(obj);
}

PyObject* VRScriptManager::pyTriggerScript(VRScriptManager* self, PyObject *args) {
    VRSceneManager::getCurrent()->triggerScript( parseString(args) );
    Py_RETURN_TRUE;
}

void execCall(PyObject* pyFkt, PyObject* pArgs, int i) {
    if (pyFkt == 0) return;
    if (PyErr_Occurred() != NULL) PyErr_Print();

    if (pArgs == 0) pArgs = PyTuple_New(0);
    PyObject_CallObject(pyFkt, pArgs);
    Py_XDECREF(pArgs);
    Py_DecRef(pyFkt);

    if (PyErr_Occurred() != NULL) PyErr_Print();
}

PyObject* VRScriptManager::stackCall(VRScriptManager* self, PyObject *args) {
    PyObject *pyFkt, *pArgs = 0;
    float delay;
    if (PyTuple_Size(args) == 2) {
        if (! PyArg_ParseTuple(args, "Of", &pyFkt, &delay)) return NULL;
    } else if (! PyArg_ParseTuple(args, "OfO", &pyFkt, &delay, &pArgs)) return NULL;
    Py_IncRef(pyFkt);

    if (pArgs != 0) {
        std::string type = pArgs->ob_type->tp_name;
        if (type == "list") pArgs = PyList_AsTuple(pArgs);
    }

    VRFunction<int>* fkt = new VRFunction<int>( "pyExecCall", boost::bind(execCall, pyFkt, pArgs, _1) );

    VRScene* scene = VRSceneManager::getCurrent();
    scene->addAnimation(0, delay, fkt, 0, 0, false);
    Py_RETURN_TRUE;
}

void on_py_file_diag_cb(PyObject* pyFkt) {
    string res = VRGuiFile::getRelativePath_toWorkdir();
    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem( pArgs, 0, PyString_FromString(res.c_str()) );
    execCall( pyFkt, pArgs, 0 );
}

PyObject* VRScriptManager::openFileDialog(VRScriptManager* self, PyObject *args) {
    PyObject *cb, *mode, *title, *default_path, *filter;
    if (! PyArg_ParseTuple(args, "OOOOO", &cb, &mode, &title, &default_path, &filter)) return NULL;
    Py_IncRef(cb);

    VRGuiFile::clearFilter();
    VRGuiFile::gotoPath( PyString_AsString(default_path) );
    VRGuiFile::setFile( PyString_AsString(default_path) );
    VRGuiFile::setCallbacks( sigc::bind<PyObject*>( sigc::ptr_fun( &on_py_file_diag_cb ), cb) );

    string m = PyString_AsString(mode);
    Gtk::FileChooserAction action = Gtk::FILE_CHOOSER_ACTION_OPEN;
    if (m == "Save" || m == "New" || m == "Create") action = Gtk::FILE_CHOOSER_ACTION_SAVE;
    VRGuiFile::open( m, action, PyString_AsString(title) );

    Py_RETURN_TRUE;
}

PyObject* VRScriptManager::updateGui(VRScriptManager* self) {
    VRGuiManager::get()->updateGtk();
    Py_RETURN_TRUE;
}

PyObject* VRScriptManager::render(VRScriptManager* self) {
    VRSceneManager::get()->updateScene();
    VRSetupManager::getCurrent()->updateWindows();
    VRGuiManager::get()->updateGtk();
    Py_RETURN_TRUE;
}

OSG_END_NAMESPACE
