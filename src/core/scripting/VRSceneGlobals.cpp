#include "VRSceneGlobals.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"

#include "VRPyBaseT.h"
#include "VRPySetup.h"
#include "VRPyNavigator.h"
#include "VRPyTypeCaster.h"
#include "VRPyProgress.h"

#include "core/scene/VRAnimationManagerT.h"
#include "core/scene/import/VRImport.h"
#include "core/scene/import/VRExport.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiFile.h"
#include "core/objects/VRTransform.h"
#include "PolyVR.h"

#include <boost/bind.hpp>
#include <sigc++/adaptors/bind.h>
#include <gtkmm/filechooser.h>

OSG_BEGIN_NAMESPACE;

PyMethodDef VRSceneGlobals::methods[] = {
	{"exit", (PyCFunction)VRSceneGlobals::exit, METH_NOARGS, "Terminate application" },
	{"loadGeometry", (PyCFunction)VRSceneGlobals::loadGeometry, METH_VARARGS|METH_KEYWORDS, "Loads a file and returns an object - obj loadGeometry(str path, bool cached = True, str preset = 'OSG', bool threaded = 0, str parent = None, str options = None)"
                                                                                             "\n\tpreset can be: 'OSG', 'COLLADA', 'SOLIDWORKS-VRML2'"
                                                                                             "\n\toptions can be: 'explorer' (currently only for STEP files)" },
	{"exportGeometry", (PyCFunction)VRSceneGlobals::exportGeometry, METH_VARARGS, "Export a part of the scene - exportGeometry( object, path )" },
	{"getLoadGeometryProgress", (PyCFunction)VRSceneGlobals::getLoadGeometryProgress, METH_VARARGS, "Return the progress object for geometry loading - getLoadGeometryProgress()" },
	{"stackCall", (PyCFunction)VRSceneGlobals::stackCall, METH_VARARGS, "Schedules a call to a python function - stackCall( function, delay, [args] )" },
	{"openFileDialog", (PyCFunction)VRSceneGlobals::openFileDialog, METH_VARARGS, "Open a file dialog - openFileDialog( onLoad, mode, title, default_path, filter )\n mode : {Save, Load, New, Create}" },
	{"updateGui", (PyCFunction)VRSceneGlobals::updateGui, METH_NOARGS, "Update the gui" },
	{"render", (PyCFunction)VRSceneGlobals::render, METH_NOARGS, "Renders the viewports" },
	{"triggerScript", (PyCFunction)VRSceneGlobals::pyTriggerScript, METH_VARARGS, "Trigger a script - triggerScript( str script )" },
	{"getRoot", (PyCFunction)VRSceneGlobals::getRoot, METH_NOARGS, "Return the root node of the scenegraph - object getRoot()" },
	{"find", (PyCFunction)VRSceneGlobals::find, METH_VARARGS, "Return a ressource by name - something find(str name)\n\tthe ressources searched are: Objects, Devices" },
	{"printOSG", (PyCFunction)VRSceneGlobals::printOSG, METH_NOARGS, "Print the OSG tree to console" },
	{"getNavigator", (PyCFunction)VRSceneGlobals::getNavigator, METH_NOARGS, "Return a handle to the navigator object" },
	{"getSetup", (PyCFunction)VRSceneGlobals::getSetup, METH_NOARGS, "Return a handle to the active hardware setup" },
	{"loadScene", (PyCFunction)VRSceneGlobals::loadScene, METH_VARARGS, "Close the current scene and open another - loadScene( str path/to/my/scene.xml )" },
	{"startThread", (PyCFunction)VRSceneGlobals::startThread, METH_VARARGS, "Start a thread - int startThread( callback, [params] )" },
	{"joinThread", (PyCFunction)VRSceneGlobals::joinThread, METH_VARARGS, "Join a thread - joinThread( int ID )" },
	{"getSystemDirectory", (PyCFunction)VRSceneGlobals::getSystemDirectory, METH_VARARGS, "Return the path to one of the specific PolyVR directories - getSystemDirectory( str dir )\n\tdir can be: ROOT, EXAMPLES, RESSOURCES, TRAFFIC" },
	{"setPhysicsActive", (PyCFunction)VRSceneGlobals::setPhysicsActive, METH_VARARGS, "Pause and unpause physics - setPhysicsActive( bool b )" },
    {NULL}  /* Sentinel */
};


// ==============
// Python methods
// ==============

PyObject* VRSceneGlobals::setPhysicsActive(VRSceneGlobals* self, PyObject *args) {
    auto scene = VRScene::getCurrent();
    if (scene) (dynamic_pointer_cast<VRPhysicsManager>(scene))->setPhysicsActive( parseBool(args) );
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
    auto fkt = VRFunction<int>::create( "scheduled scene load", boost::bind(&VRSceneManager::loadScene, VRSceneManager::get(), parseString(args), false ) );
    VRSceneManager::get()->queueJob(fkt);
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::getSetup(VRSceneGlobals* self) {
    return VRPySetup::fromSharedPtr(VRSetup::getCurrent());
}

PyObject* VRSceneGlobals::getNavigator(VRSceneGlobals* self) {
    auto scene = VRScene::getCurrent();
    return VRPyNavigator::fromPtr((VRNavigator*)scene.get());
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
    if (auto res = VRScene::getCurrent()->getRoot()->find(name)) return VRPyTypeCaster::cast(res);
    if (auto res = VRSetup::getCurrent()->getDevice(name)) return VRPyTypeCaster::cast(res);
    Py_RETURN_NONE;
}

PyObject* VRSceneGlobals::getRoot(VRSceneGlobals* self) {
    return VRPyTypeCaster::cast( VRScene::getCurrent()->getRoot() );
}

PyObject* VRSceneGlobals::loadGeometry(VRSceneGlobals* self, PyObject *args, PyObject *kwargs) {
    const char* path = "";
    int ignoreCache = 0;
    int threaded = 0;
    const char* preset = "OSG";
    const char* parent = "";
    const char* options = "";

    const char* kwlist[] = {"path", "cached", "preset", "threaded", "parent", "options", NULL};
    string format = "s|isiss:loadGeometry";
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, format.c_str(), (char**)kwlist, &path, &ignoreCache, &preset, &threaded, &parent, &options)) return NULL;

    VRObjectPtr prnt = VRScene::getCurrent()->getRoot()->find( parent );

    VRTransformPtr obj = VRImport::get()->load( path, prnt, !ignoreCache, preset, threaded, options);
    if (obj == 0) {
        VRGuiManager::get()->printToConsole("Errors", "Warning: " + string(path) + " not loaded!\n");
        Py_RETURN_NONE;
    }
    obj->setPersistency(0);
    return VRPyTypeCaster::cast(obj);
}

PyObject* VRSceneGlobals::exportGeometry(VRSceneGlobals* self, PyObject *args) {
    const char* path = "";
    VRPyObject* o;
    if (! PyArg_ParseTuple(args, "Os", &o, &path)) return NULL;
    VRExport::get()->write( o->objPtr, path );
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::getLoadGeometryProgress(VRSceneGlobals* self) {
    const char* path = "";
    return VRPyProgress::fromSharedPtr( VRImport::get()->getProgressObject() );
}

PyObject* VRSceneGlobals::pyTriggerScript(VRSceneGlobals* self, PyObject *args) {
    VRScene::getCurrent()->triggerScript( parseString(args) );
    Py_RETURN_TRUE;
}

void execCall(PyObject* pyFkt, PyObject* pArgs, int i) {
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

    auto pyThread = VRFunction< VRThreadWeakPtr >::create( "pyExecCall", boost::bind(execThread, pyFkt, pArgs, _1) );
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

    VRUpdateCbPtr fkt = VRFunction<int>::create( "pyExecCall", boost::bind(execCall, pyFkt, pArgs, _1) );
    VRUpdateCbWeakPtr wkp = fkt;

    auto scene = VRScene::getCurrent();
    auto a = scene->addAnimation(0, delay, wkp, 0, 0, false);
    a->setCallbackOwner(true);
    Py_RETURN_TRUE;
}

void on_py_file_diag_cb(PyObject* pyFkt) {
    string res = VRGuiFile::getRelativePath_toWorkdir();
    float scale = VRGuiFile::getScale();
    PyObject *pArgs = PyTuple_New(2);
    PyTuple_SetItem( pArgs, 0, PyString_FromString(res.c_str()) );
    PyTuple_SetItem( pArgs, 1, PyFloat_FromDouble(scale) );
    execCall( pyFkt, pArgs, 0 );
}

PyObject* VRSceneGlobals::openFileDialog(VRSceneGlobals* self, PyObject *args) {
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

PyObject* VRSceneGlobals::updateGui(VRSceneGlobals* self) {
    VRGuiManager::get()->updateGtk();
    Py_RETURN_TRUE;
}

PyObject* VRSceneGlobals::render(VRSceneGlobals* self) {
    VRSceneManager::get()->updateScene();
    VRSetup::getCurrent()->updateWindows();
    VRGuiManager::get()->updateGtk();
    Py_RETURN_TRUE;
}

OSG_END_NAMESPACE;
