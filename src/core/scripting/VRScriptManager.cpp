#include "VRScriptManager.h"

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>

#include <sigc++/adaptors/bind.h>
#include <gtkmm/filechooser.h>

#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/import/VRImport.h"
#include "core/scene/import/VRExport.h"
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
#include "VRPyIntersection.h"
#include "VRPyPose.h"
#include "VRPyPath.h"
#include "VRPyGraph.h"
#include "VRPyPolygon.h"
#include "VRPyTriangulator.h"
#include "VRPyStroke.h"
#include "VRPyColorChooser.h"
#include "VRPyTextureRenderer.h"
#include "VRPyConstraint.h"
#include "VRPyHaptic.h"
#include "VRPyMouse.h"
#include "VRPyMobile.h"
#include "VRPyBaseT.h"
#include "VRPyMaterial.h"
#include "VRPyTextureGenerator.h"
#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyCamera.h"
#include "VRPyLod.h"
#include "VRPyRecorder.h"
#include "VRPyPathtool.h"
#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPyAnnotationEngine.h"
#include "VRPyAnalyticGeometry.h"
#include "VRPySelector.h"
#include "VRPySelection.h"
#include "VRPyPatchSelection.h"
#include "VRPyPolygonSelection.h"
#include "VRPyMenu.h"
#include "VRPyClipPlane.h"
#include "VRPyListMath.h"
#include "VRPySetup.h"
#include "VRPyNavigator.h"
#include "VRPyNavPreset.h"
#include "VRPyWaypoint.h"
#include "VRPyMeasure.h"
#include "VRPyJointTool.h"
#include "VRPyImage.h"
#include "VRPyProjectManager.h"
#include "VRPyGeoPrimitive.h"
#include "VRPyProgress.h"
#include "VRPyUndoManager.h"
#include <iostream>
#include <algorithm>
#include <memory>

//TODO: refactoring
#include "core/gui/VRGuiFile.h"
#include "core/gui/VRGuiManager.h"
#include "core/setup/VRSetup.h"
#include "addons/Algorithms/VRPyGraphLayout.h"
#include "addons/CaveKeeper/VRPyCaveKeeper.h"
#include "addons/Bullet/Particles/VRPyParticles.h"
#include "addons/Bullet/Fluids/VRPyFluids.h"
#include "addons/Bullet/CarDynamics/VRPyCarDynamics.h"
#include "addons/Engineering/Factory/VRPyLogistics.h"
#include "addons/Engineering/Factory/VRPyProduction.h"
#include "addons/Engineering/Factory/VRPyAMLLoader.h"
#include "addons/Engineering/Mechanics/VRPyMechanism.h"
#include "addons/Engineering/VRPyNumberingEngine.h"
#include "addons/CEF/VRPyCEF.h"
#include "addons/CEF/VRPyWebCam.h"
#include "addons/Semantics/Segmentation/VRPySegmentation.h"
#include "addons/Semantics/Segmentation/VRPyAdjacencyGraph.h"
#include "addons/Semantics/Processes/VRPyProcess.h"
#include "addons/Engineering/Chemistry/VRPyMolecule.h"
#include "addons/Engineering/Factory/VRPyFactory.h"
#include "addons/Engineering/Milling/VRPyMillingMachine.h"
#include "addons/Engineering/Milling/VRPyMillingWorkPiece.h"
#include "addons/Engineering/Milling/VRPyMillingCuttingToolProfile.h"
#include "addons/Engineering/VRPyRobotArm.h"
#include "addons/RealWorld/nature/VRPyTree.h"
#include "VRPyTypeCaster.h"
#include "PolyVR.h"

// not yet ported dependencies
#ifndef _WIN32
#include "addons/Engineering/CSG/VRPyCSG.h"
#include "addons/RealWorld/VRPyRealWorld.h"
#include "addons/RealWorld/traffic/VRPyTrafficSimulation.h"
#include "addons/SimViDekont/VRPySimViDekont.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#endif

#include "VRSceneGlobals.h"

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
    VRGuiManager::get()->printToConsole("Console", what);
    return Py_BuildValue("");
}

static PyObject* writeErr(PyObject *self, PyObject *args) {
    const char *what;
    if (!PyArg_ParseTuple(args, "s", &what)) return NULL;
    VRGuiManager::get()->printToConsole("Errors", what);
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
    registerModule<VRPyObject>("Object", pModVR, VRPyStorage::typeRef);
    registerModule<VRPyTransform>("Transform", pModVR, VRPyObject::typeRef);
    registerModule<VRPyGeometry>("Geometry", pModVR, VRPyTransform::typeRef);
    registerModule<VRPyMaterial>("Material", pModVR, VRPyObject::typeRef);
    registerModule<VRPyTextureGenerator>("TextureGenerator", pModVR);
    registerModule<VRPyImage>("Image", pModVR);
    registerModule<VRPyLight>("Light", pModVR, VRPyObject::typeRef);
    registerModule<VRPyLightBeacon>("LightBeacon", pModVR, VRPyTransform::typeRef);
    registerModule<VRPyCamera>("Camera", pModVR, VRPyTransform::typeRef);
    registerModule<VRPyLod>("Lod", pModVR, VRPyObject::typeRef);
    registerModule<VRPySprite>("Sprite", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPySound>("Sound", pModVR);
    registerModule<VRPySocket>("Socket", pModVR);
    registerModule<VRPyStroke>("Stroke", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyConstraint>("Constraint", pModVR);
    registerModule<VRPyDevice>("Device", pModVR);
    registerModule<VRPyIntersection>("Intersection", pModVR);
    registerModule<VRPyHaptic>("Haptic", pModVR, VRPyDevice::typeRef);
    registerModule<VRPyMobile>("Mobile", pModVR, VRPyDevice::typeRef);
    registerModule<VRPyMouse>("Mouse", pModVR, VRPyDevice::typeRef);
    registerModule<VRPyAnimation>("Animation", pModVR);
    registerModule<VRPyPose>("Pose", pModVR);
    registerModule<VRPyPath>("Path", pModVR);
    registerModule<VRPyGraph>("Graph", pModVR);
    registerModule<VRPyGraphLayout>("GraphLayout", pModVR);
    registerModule<VRPyPolygon>("Polygon", pModVR);
    registerModule<VRPyTriangulator>("Triangulator", pModVR);
    registerModule<VRPyRecorder>("Recorder", pModVR);
    registerModule<VRPyProjectManager>("ProjectManager", pModVR, VRPyObject::typeRef);
    registerModule<VRPyGeoPrimitive>("GeoPrimitive", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyStorage>("Storage", pModVR);
    registerModule<VRPySnappingEngine>("SnappingEngine", pModVR);
    registerModule<VRPyAnnotationEngine>("AnnotationEngine", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyAnalyticGeometry>("AnalyticGeometry", pModVR, VRPyObject::typeRef);
    registerModule<VRPyConstructionKit>("ConstructionKit", pModVR);
    registerModule<VRPyPathtool>("Pathtool", pModVR, VRPyObject::typeRef);
    registerModule<VRPySelector>("Selector", pModVR);
    registerModule<VRPySelection>("Selection", pModVR);
    registerModule<VRPyPatchSelection>("PatchSelection", pModVR, VRPySelection::typeRef);
    registerModule<VRPyPolygonSelection>("PolygonSelection", pModVR, VRPySelection::typeRef);
    registerModule<VRPyNavigator>("Navigator", pModVR);
    registerModule<VRPyNavPreset>("NavPreset", pModVR);

    registerModule<VRPyProgress>("Progress", pModVR);
    registerModule<VRPyUndoManager>("UndoManager", pModVR);
    registerModule<VRPyMenu>("Menu", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyClipPlane>("ClipPlane", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyWaypoint>("Waypoint", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyMeasure>("Measure", pModVR, VRPyAnalyticGeometry::typeRef);
    registerModule<VRPyJointTool>("JointTool", pModVR, VRPyGeometry::typeRef);
	registerModule<VRPyColorChooser>("ColorChooser", pModVR);
	registerModule<VRPyTextureRenderer>("TextureRenderer", pModVR, VRPyObject::typeRef);
    registerModule<VRPyCaveKeeper>("CaveKeeper", pModVR);
    registerModule<VRPyParticles>("Particles", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyFluids>("Fluids", pModVR, VRPyParticles::typeRef);
    registerModule<VRPyMetaBalls>("MetaBalls", pModVR, VRPyObject::typeRef);
    registerModule<VRPyCarDynamics>("CarDynamics", pModVR);
    registerModule<VRPyCEF>("CEF", pModVR);
    registerModule<VRPyWebCam>("Webcam", pModVR, VRPySprite::typeRef);
    registerModule<VRPySegmentation>("Segmentation", pModVR);
    registerModule<VRPyAdjacencyGraph>("AdjacencyGraph", pModVR);
    registerModule<VRPyMechanism>("Mechanism", pModVR);
    registerModule<VRPyNumberingEngine>("NumberingEngine", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyTree>("Tree", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyMillingMachine>("MillingMachine", pModVR);
    registerModule<VRPyMillingWorkPiece>("MillingWorkPiece", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyMillingCuttingToolProfile>("MillingCuttingToolProfile", pModVR);
    registerModule<VRPyMolecule>("Molecule", pModVR, VRPyGeometry::typeRef);
    registerModule<VRPyRobotArm>("RobotArm", pModVR);
    registerModule<VRPyOntology>("Ontology", pModVR);
    registerModule<VRPyProcess>("Process", pModVR);
    registerModule<VRPyProcessNode>("ProcessNode", pModVR);
    registerModule<VRPyProcessLayout>("ProcessLayout", pModVR, VRPyTransform::typeRef);
    registerModule<VRPyOntologyRule>("OntologyRule", pModVR);
    registerModule<VRPyProperty>("Property", pModVR);
    registerModule<VRPyConcept>("Concept", pModVR);
    registerModule<VRPyEntity>("Entity", pModVR);
    registerModule<VRPyReasoner>("Reasoner", pModVR);

#ifndef _WIN32
	registerModule<VRPyCSG>("CSGGeometry", pModVR, VRPyGeometry::typeRef);
	registerModule<VRPyRealWorld>("RealWorld", pModVR);
	registerModule<VRPyTrafficSimulation>("TrafficSimulation", pModVR);
	registerModule<VRPySimViDekont>("SimViDekont", pModVR);
#endif

    PyObject* pModSetup = Py_InitModule3("Setup", VRSceneGlobals::methods, "VR Module");
    registerModule<VRPySetup>("Setup", pModSetup, 0, "Setup");
    registerModule<VRPyView>("View", pModSetup, 0, "Setup");
    registerModule<VRPyWindow>("Window", pModSetup, 0, "Setup");

    PyObject* pModFactory = Py_InitModule3("Factory", VRSceneGlobals::methods, "VR Module");
    registerModule<FPyNode>("Node", pModFactory, 0, "Factory");
    registerModule<FPyNetwork>("Network", pModFactory, 0, "Factory");
    registerModule<FPyPath>("FPath", pModFactory, 0, "Factory");
    registerModule<FPyTransporter>("Transporter", pModFactory, 0, "Factory");
    registerModule<FPyContainer>("Container", pModFactory, 0, "Factory");
    registerModule<FPyProduct>("Product", pModFactory, 0, "Factory");
    registerModule<FPyLogistics>("Logistics", pModFactory, 0, "Factory");
    registerModule<VRPyFactory>("Factory", pModFactory, 0, "Factory");
    registerModule<VRPyProduction>("Production", pModFactory, 0, "Factory");
    registerModule<VRPyAMLLoader>("AMLLoader", pModFactory, 0, "Factory");
    PyModule_AddObject(pModVR, "Factory", pModFactory);

	if (!VROptions::get()->getOption<bool>("standalone")) initVRPyStdOut();

    // add cython local path to python search path
    PyRun_SimpleString(
        "import sys\n"
        "sys.path.append('cython/')\n"
    );

    //PyEval_ReleaseLock();
    //PyEval_SaveThread();
}

template<class T>
void VRScriptManager::registerModule(string mod, PyObject* parent, PyTypeObject* base, string mod_parent) {
    T::registerModule(mod, parent, base);
    modules[mod_parent][mod] = T::typeRef;
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

void VRScriptManager::allowScriptThreads() {
    if (pyThreadState == 0) pyThreadState = PyEval_SaveThread();
}

void VRScriptManager::blockScriptThreads() {
    if (pyThreadState) { PyEval_RestoreThread(pyThreadState); pyThreadState = 0; }
}

OSG_END_NAMESPACE
