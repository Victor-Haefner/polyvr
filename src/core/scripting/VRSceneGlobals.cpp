#include "VRSceneGlobals.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"

#include "core/scene/import/VRDWG.h"
#include "core/scripting/VRPyGeometry.h"

#include "VRPyBaseT.h"
#include "VRPySetup.h"
#include "VRPyNavigator.h"
#include "VRPyRendering.h"
#include "VRPyTypeCaster.h"
#ifndef WITHOUT_GTK
#include <gtk/gtk.h>
#endif
#include "VRPyProgress.h"
#include "VRPySky.h"
#ifndef WITHOUT_AV
#include "VRPySound.h"
#endif
#include "VRPyMaterial.h"
#include "VRPyCodeCompletion.h"

#include "core/scene/VRAnimationManagerT.h"
#include "core/scene/import/VRImport.h"
#include "core/scene/import/VRExport.h"
#include "core/scene/import/E57/E57.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRTests.h"
#include "PolyVR.h"

#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#include "core/gui/VRGuiFile.h"
#endif

OSG_BEGIN_NAMESPACE;

string loadGeometryDoc =
"Loads a file and returns an object"
"\n\n\tobj loadGeometry(path, cached = True, preset = 'OSG', threaded = 0, parent = None, options = None, useBinaryCache = False)"
"\n\n\tpreset can be: 'OSG', 'PVR', 'COLLADA', 'SOLIDWORKS-VRML2'"
"\n\n\toptions example for pointclouds (.e57, .xyz):"
"\n\t\topts = {}"
"\n\t\topts['lit'] = 0"
"\n\t\topts['downsampling'] = 0.1"
"\n\t\topts['pointSize'] = 5"
"\n\t\topts['lod1'] = [5, 20]"
"\n\t\topts['lod2'] = [10, 200]"
"\n\t\topts['xyzNoColor'] = 0"
;

PyMethodDef VRSceneGlobals::methods[] = {
	{"exit", (PyCFunction)VRSceneGlobals::exit, METH_NOARGS, "Terminate application" },
	{"loadGeometry", (PyCFunction)VRSceneGlobals::loadGeometry, METH_VARARGS|METH_KEYWORDS, loadGeometryDoc.c_str() },
	{"exportToFile", (PyCFunction)VRSceneGlobals::exportToFile, METH_VARARGS, "Export a node ( object, path ), supported extensions: [wrl, wrz, obj, osb, osg, ply, gltf]" },
	{"getLoadGeometryProgress", (PyCFunction)VRSceneGlobals::getLoadGeometryProgress, METH_VARARGS, "Return the progress object for geometry loading - getLoadGeometryProgress()" },
	{"stackCall", (PyCFunction)VRSceneGlobals::stackCall, METH_VARARGS, "Schedules a call to a python function - stackCall( function, delay, [args] )" },
	{"openFileDialog", (PyCFunction)VRSceneGlobals::openFileDialog, METH_VARARGS, "Open a file dialog - openFileDialog( onLoad, mode, title, default_path, filter )\n mode : {Save, Load, New, Create}" },
	{"updateGui", (PyCFunction)VRSceneGlobals::updateGui, METH_NOARGS, "Update the gui" },
	{"render", (PyCFunction)VRSceneGlobals::render, METH_NOARGS, "Renders the viewports" },
	{"triggerScript", (PyCFunction)VRSceneGlobals::pyTriggerScript, METH_VARARGS, "Trigger a script - triggerScript( str script )" },
	{"getRoot", (PyCFunction)VRSceneGlobals::getRoot, METH_NOARGS, "Return the root node of the scenegraph - object getRoot()" },
	{"find", (PyCFunction)VRSceneGlobals::find, METH_VARARGS, "Return a ressource by name - something find(str name)\n\tthe ressources searched are: Objects, Devices" },
	{"findByID", (PyCFunction)VRSceneGlobals::findByID, METH_VARARGS, "Return a ressource by ID - something find(int ID)\n\tthe ressources searched are: Objects" },
	{"printOSG", (PyCFunction)VRSceneGlobals::printOSG, METH_NOARGS, "Print the OSG tree to console" },
	{"getNavigator", (PyCFunction)VRSceneGlobals::getNavigator, METH_NOARGS, "Return a handle to the navigator object" },
	{"getRendering", (PyCFunction)VRSceneGlobals::getRendering, METH_NOARGS, "Return a handle to the rendering manager" },
	{"getSetup", (PyCFunction)VRSceneGlobals::getSetup, METH_NOARGS, "Return a handle to the active hardware setup" },
	{"loadScene", (PyCFunction)VRSceneGlobals::loadScene, METH_VARARGS, "Close the current scene and open another - loadScene( str path/to/my/scene.xml )" },
	{"startThread", (PyCFunction)VRSceneGlobals::startThread, METH_VARARGS, "Start a thread - int startThread( callback, [params] )" },
	{"joinThread", (PyCFunction)VRSceneGlobals::joinThread, METH_VARARGS, "Join a thread - joinThread( int ID )" },
	{"getSystemDirectory", (PyCFunction)VRSceneGlobals::getSystemDirectory, METH_VARARGS, "Return the path to one of the specific PolyVR directories - getSystemDirectory( str dir )\n\tdir can be: ROOT, EXAMPLES, RESSOURCES, TRAFFIC" },
	{"setPhysicsActive", (PyCFunction)VRSceneGlobals::setPhysicsActive, METH_VARARGS, "Pause and unpause physics - setPhysicsActive( bool b )" },
	{"runTest", (PyCFunction)VRSceneGlobals::runTest, METH_VARARGS, "Run a built-in system test - runTest( string test )" },
	{"getSceneMaterials", (PyCFunction)VRSceneGlobals::getSceneMaterials, METH_NOARGS, "Get all materials of the scene - getSceneMaterials()" },
	{"getSky", (PyCFunction)VRSceneGlobals::getSky, METH_NOARGS, "Get sky module" },
	{"getSoundManager", (PyCFunction)VRSceneGlobals::getSoundManager, METH_NOARGS, "Get sound manager module" },
	{"getFrame", (PyCFunction)VRSceneGlobals::getFrame, METH_NOARGS, "Get current frame number" },
	{"getScript", (PyCFunction)VRSceneGlobals::getScript, METH_VARARGS, "Get python script by name" },
	{"importScene", (PyCFunction)VRSceneGlobals::importScene, METH_VARARGS, "Import scene" },
	{"getActiveCamera", (PyCFunction)VRSceneGlobals::getActiveCamera, METH_NOARGS, "Get active camera" },
	{"testDWGArcs", (PyCFunction)VRSceneGlobals::testDWGArcs, METH_NOARGS, "A test for DWG arcs tesselation" },
	{"setWindowTitle", (PyCFunction)VRSceneGlobals::setWindowTitle, METH_VARARGS, "Set window title" },
    {NULL}  /* Sentinel */
};


// ==============
// Python methods
// ==============

PyObject* VRSceneGlobals::setWindowTitle(VRSceneGlobals* self, PyObject* args) {
    string name = parseString(args);
#ifndef WITHOUT_GTK
    VRGuiManager::get()->setWindowTitle(name);
#endif
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::getScript(VRSceneGlobals* self, PyObject* args) {
    string name = parseString(args);
    VRScriptPtr s = VRScene::getCurrent()->getScript(name);
    return VRPyScript::fromSharedPtr( s );
}

PyObject* VRSceneGlobals::getFrame(VRSceneGlobals* self) {
    return PyInt_FromLong(VRGlobals::CURRENT_FRAME);
}

PyObject* VRSceneGlobals::getActiveCamera(VRSceneGlobals* self) {
    return VRPyTypeCaster::cast( VRScene::getCurrent()->getActiveCamera() );
}

PyObject* VRSceneGlobals::testDWGArcs(VRSceneGlobals* self) {
#ifndef WITHOUT_DWG
    return VRPyGeometry::fromSharedPtr( dwgArcTest() );
#else
	return 0;
#endif
}

PyObject* VRSceneGlobals::getSoundManager(VRSceneGlobals* self) {
#ifndef WITHOUT_AV
    return VRPySoundManager::fromSharedPtr( VRSoundManager::get() );
#else
	return 0;
#endif
}

PyObject* VRSceneGlobals::getSky(VRSceneGlobals* self) {
    auto scene = VRScene::getCurrent();
    return VRPySky::fromSharedPtr( scene->getSky() );
}

PyObject* VRSceneGlobals::getSceneMaterials(VRSceneGlobals* self) {
    auto scene = VRScene::getCurrent();
    auto res = PyList_New(0);
    if (scene) {
        auto mats = VRMaterial::getAll();
        for (auto m : mats) PyList_Append(res, VRPyMaterial::fromSharedPtr(m));
    }
    return res;
}

PyObject* VRSceneGlobals::setPhysicsActive(VRSceneGlobals* self, PyObject *args) {
#ifndef WITHOUT_BULLET
    auto scene = VRScene::getCurrent();
    if (scene) (dynamic_pointer_cast<VRPhysicsManager>(scene))->setPhysicsActive( parseBool(args) );
#endif
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::getSystemDirectory(VRSceneGlobals* self, PyObject *args) {
    string dir = parseString(args);
    string path = VRSceneManager::get()->getOriginalWorkdir();
    if (dir == "ROOT") ;
    if (dir == "EXAMPLES") path += "/examples";
    if (dir == "RESSOURCES") path += "/ressources";
    if (dir == "TRAFFIC") path += "/src/addons/RealWorld/traffic/simulation/bin";
    return PyString_FromString(path.c_str());
}

PyObject* VRSceneGlobals::loadScene(VRSceneGlobals* self, PyObject *args) {
    auto fkt = VRUpdateCb::create( "scheduled scene load", bind(&VRSceneManager::loadScene, VRSceneManager::get(), parseString(args), false, "" ) );
    VRSceneManager::get()->queueJob(fkt);
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::getSetup(VRSceneGlobals* self) {
    return VRPySetup::fromSharedPtr(VRSetup::getCurrent());
}

PyObject* VRSceneGlobals::getNavigator(VRSceneGlobals* self) {
    auto scene = VRScene::getCurrent();
    return VRPyNavigator::fromSharedPtr(dynamic_pointer_cast<VRNavigator>(scene));
}

PyObject* VRSceneGlobals::getRendering(VRSceneGlobals* self) {
    auto scene = VRScene::getCurrent();
    return VRPyRendering::fromSharedPtr(dynamic_pointer_cast<VRRendering>(scene));
}

PyObject* VRSceneGlobals::printOSG(VRSceneGlobals* self) {
    VRObject::printOSGTree( VRScene::getCurrent()->getRoot()->getNode() );
    VRSetup::getCurrent()->printOSG();
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::exit(VRSceneGlobals* self) {
    PolyVR::shutdown();
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::find(VRSceneGlobals* self, PyObject *args) {
    string name = parseString(args);
    auto setup = VRSetup::getCurrent();
    if (setup)
        if (auto res = setup->getDevice(name)) return VRPyTypeCaster::cast(res);
    if (auto res = VRScene::getCurrent()->get(name, true)) return VRPyTypeCaster::cast(res);
    if (auto res = VRScene::getCurrent()->get(name, false)) return VRPyTypeCaster::cast(res);
    Py_RETURN_NONE;
}

PyObject* VRSceneGlobals::findByID(VRSceneGlobals* self, PyObject *args) {
    int ID = parseInt(args);
    if (auto res = VRScene::getCurrent()->get(ID)) return VRPyTypeCaster::cast(res);
    Py_RETURN_NONE;
}

PyObject* VRSceneGlobals::getRoot(VRSceneGlobals* self) {
    return VRPyTypeCaster::cast( VRScene::getCurrent()->getRoot() );
}

PyObject* VRSceneGlobals::importScene(VRSceneGlobals* self, PyObject *args) {
    const char* path = "";
    const char* key = "";
    int offLights = 0;
    if (! PyArg_ParseTuple(args, "s|si", &path, &key, &offLights)) return NULL;
    string Path = path?path:"";
    auto res = VRSceneLoader::get()->importScene( Path, key?key:"", offLights );
    if (res) return VRPyTypeCaster::cast(res);
    else { VRPyBase::setErr("Import scene, path '"+Path+"' not found!"); return NULL; }
    //else Py_RETURN_NONE;
}

PyObject* VRSceneGlobals::loadGeometry(VRSceneGlobals* self, PyObject *args, PyObject *kwargs) {
    const char* path = "";
    int cached = 0;
    int threaded = 0;
    int useBinaryCache = 0;
    const char* preset = "OSG";
    const char* parent = "";
    PyObject* opt = 0;

    const char* kwlist[] = {"path", "cached", "preset", "threaded", "parent", "options", "useBinaryCache", NULL};
    string format = "s|isisOi:loadGeometry";
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, format.c_str(), (char**)kwlist, &path, &cached, &preset, &threaded, &parent, &opt, &useBinaryCache)) return NULL;

    VRObjectPtr prnt = VRScene::getCurrent()->getRoot()->find( parent );
    map<string, string> options;
    if (opt) toValue(opt, options);

    /*cout << "loadGeometry options? " << opt << endl;
    for (auto o : options) {
        cout << " loadGeometry option: " << o.first << " -> " << o.second << endl;
    }*/

    VRTransformPtr obj = VRImport::get()->load( path, prnt, cached, preset, threaded, options, useBinaryCache);
    if (obj == 0) { VRPyBase::setErr("Error: " + string(path) + " not loaded!"); return NULL; }
    obj->setPersistency(0);
    return VRPyTypeCaster::cast(obj);
}

PyObject* VRSceneGlobals::exportToFile(VRSceneGlobals* self, PyObject *args) {
    const char* path = "";
    VRPyObject* o;
    if (! PyArg_ParseTuple(args, "Os", &o, &path)) return NULL;
    VRExport::get()->write( o->objPtr, path );
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::getLoadGeometryProgress(VRSceneGlobals* self) {
    return VRPyProgress::fromSharedPtr( VRImport::get()->getProgressObject() );
}

PyObject* VRSceneGlobals::pyTriggerScript(VRSceneGlobals* self, PyObject *args) {
    VRScene::getCurrent()->triggerScript( parseString(args) );
    Py_RETURN_TRUE;
}

void execCall(PyObject* pyFkt, PyObject* pArgs, float f) {
    if (pyFkt == 0) return;
    PyGILState_STATE gstate = PyGILState_Ensure();
    if (PyErr_Occurred() != NULL) PyErr_Print();
    if (pArgs == 0) pArgs = PyTuple_New(0);

    PyObject_CallObject(pyFkt, pArgs);

    Py_XDECREF(pArgs);
    Py_DecRef(pyFkt);

    if (PyErr_Occurred() != NULL) PyErr_Print();
    PyGILState_Release(gstate);
}

void execThread(PyObject* pyFkt, PyObject* pArgs,  std::weak_ptr<VRThread>  thread) {
    execCall(pyFkt, pArgs, 0);
}

map<int, VRThreadCbPtr> pyThreadsTmp;
PyObject* VRSceneGlobals::startThread(VRSceneGlobals* self, PyObject *args) {
    PyObject *pyFkt, *pArgs = 0;
    if (! PyArg_ParseTuple(args, "O|O", &pyFkt, &pArgs)) return NULL;
    Py_IncRef(pyFkt);

    if (pArgs != 0) {
        std::string type = pArgs->ob_type->tp_name;
        if (type == "list") pArgs = PyList_AsTuple(pArgs);
    }

    auto pyThread = VRFunction< VRThreadWeakPtr >::create( "pyExecCall", bind(execThread, pyFkt, pArgs, _1) );
    int t = VRScene::getCurrent()->initThread(pyThread, "python thread");
    pyThreadsTmp[t] = pyThread; // need to keep a reference!
    //self->pyThreads[t] = pyThread; // TODO: self is 0 ???
    return PyInt_FromLong(t);
}

PyObject* VRSceneGlobals::joinThread(VRSceneGlobals* self, PyObject *args) {
    int ID = parseInt(args);
    VRScene::getCurrent()->stopThread(ID);
    pyThreadsTmp.erase(ID);
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::stackCall(VRSceneGlobals* self, PyObject *args) {
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

    auto fkt = VRAnimCb::create( "pyExecCall", bind(execCall, pyFkt, pArgs, _1) );
    auto a = VRScene::getCurrent()->addAnimation(0, delay, fkt, 0.f, 0.f, false, true);
    Py_RETURN_TRUE;
}

void on_py_file_diag_cb(PyObject* pyFkt) {
#ifndef WITHOUT_GTK
    string res = VRGuiFile::getRelativePath_toWorkdir();
    PyObject *pArgs = PyTuple_New(3);
    PyTuple_SetItem( pArgs, 0, PyString_FromString(res.c_str()) );
    PyTuple_SetItem( pArgs, 1, PyFloat_FromDouble( VRGuiFile::getScale() ) );
    PyTuple_SetItem( pArgs, 2, PyString_FromString( VRGuiFile::getPreset().c_str() ) );
    execCall( pyFkt, pArgs, 0 );
#endif
}

PyObject* VRSceneGlobals::openFileDialog(VRSceneGlobals* self, PyObject *args) {
#ifndef WITHOUT_GTK
    PyObject *cb, *mode, *title, *default_path, *filter;
    if (! PyArg_ParseTuple(args, "OOOOO", &cb, &mode, &title, &default_path, &filter)) return NULL;
    Py_IncRef(cb);

    VRGuiFile::clearFilter();
    VRGuiFile::gotoPath( PyString_AsString(default_path) );
    VRGuiFile::setFile( PyString_AsString(default_path) );
    VRGuiFile::setCallbacks( bind(on_py_file_diag_cb, cb) );

    string m = PyString_AsString(mode);
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    if (m == "Save" || m == "New" || m == "Create") action = GTK_FILE_CHOOSER_ACTION_SAVE;
    else VRGuiFile::setGeoLoadWidget();
    VRGuiFile::open( m, action, PyString_AsString(title) );
#endif
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::updateGui(VRSceneGlobals* self) {
#ifndef WITHOUT_GTK
    VRGuiManager::get()->updateGtk();
#endif
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::render(VRSceneGlobals* self) {
    VRSceneManager::get()->updateScene();
    VRSetup::getCurrent()->updateWindows();
#ifndef WITHOUT_GTK
    VRGuiManager::get()->updateGtk();
#endif
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::runTest(VRSceneGlobals* self, PyObject *args) {
    const char* test = "";
    if (!PyArg_ParseTuple(args, "s", &test)) return NULL;
    VRRunTest(test);
    Py_RETURN_TRUE;
}

OSG_END_NAMESPACE;
