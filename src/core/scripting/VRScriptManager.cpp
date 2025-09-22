#include "VRScriptManager.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRLogger.h"
#include "core/gui/VRGuiManager.h"
#ifndef WITHOUT_IMGUI
#include "core/gui/VRGuiConsole.h"
#endif
#include "VRScript.h"
#include "VRSceneModules.h"
#include "VRSceneGlobals.h"
#include "VRPyListMath.h"
#include "VRPyBaseT.h"

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <iostream>
#include <algorithm>
#include <memory>

#define TEMPLATEV(...) #__VA_ARGS__
#define TEMPLATE(...) TEMPLATEV(__VA_ARGS__)

OSG_BEGIN_NAMESPACE;
using namespace std;

void clear_sys_modules() {
    PyObject *sys_module = PyImport_ImportModule("sys");
    if (sys_module) {
        PyObject *modules_dict = PyObject_GetAttrString(sys_module, "modules");
        if (modules_dict && PyDict_Check(modules_dict)) {
            PyDict_Clear(modules_dict);  // Clear all modules
        }
        Py_XDECREF(modules_dict);
        Py_DECREF(sys_module);
    }
}

void clear_global_objects() {
    PyObject *globals = PyEval_GetGlobals();
    if (globals && PyDict_Check(globals)) {
        PyDict_Clear(globals);  // Clear all global variables
    }
}

void clear_all_objects() {
    PyObject *gc_module = PyImport_ImportModule("gc");
    if (gc_module) {
        PyObject *all_objects = PyObject_CallMethod(gc_module, "get_objects", NULL);
        if (all_objects && PyList_Check(all_objects)) {
            for (Py_ssize_t i = 0; i < PyList_Size(all_objects); ++i) {
                PyObject *obj = PyList_GetItem(all_objects, i);
                if (obj) {
                    Py_DECREF(obj);  // Decrease ref count
                }
            }
        }
        Py_XDECREF(all_objects);
        Py_DECREF(gc_module);
    }
}

void checkGarbageCollection() { // for diagnostic purposes
    auto gc = PyImport_ImportModule("gc");
    string name = PyModule_GetName(gc);
    cout << "Python checkGarbageColection, module " << name << endl;
    auto gc_dict = PyModule_GetDict(gc);

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    map<string, PyObject*> gc_members;
    while (PyDict_Next(gc_dict, &pos, &key, &value)) {
        string key_name = PyUnicode_AsUTF8(key);
        gc_members[key_name] = value;
        //cout << " " << key_name << "  " << value << endl;
    }

    auto exec = [&](string cb) {
        auto pyFkt = gc_members[cb];
        PyGILState_STATE gstate = PyGILState_Ensure();
        if (PyErr_Occurred() != NULL) PyErr_Print();
        PyObject* res = PyObject_CallObject(pyFkt, 0);
        if (PyErr_Occurred() != NULL) PyErr_Print();
        PyGILState_Release(gstate);
        return res;
    };

    cout << " GC execute 'collect'" << endl;
    exec("collect");
    cout << " ..done" << endl;

    auto garbage = gc_members["garbage"];
    for (int i=0; i<PyList_Size(garbage); i++) {
        PyObject* item = PyList_GetItem(garbage, i);
        cout << "Py garbage: " << item << endl;
    }
}

void clearModule(PyObject* mod) {
    PyObject* module_dict = PyModule_GetDict(mod);
    if (module_dict && PyDict_Check(module_dict)) {
        cout << "clear py module!" << endl;
        PyDict_Clear(module_dict);  // Clear mod
    }
}

VRScriptManager::VRScriptManager() {
    cout << "Init ScriptManager" << endl;
    initPyModules();

    cout << " Python version: " << Py_GetVersion() << endl;

    setStorageType("Scripts");
    storeMap("Script", &scripts);

    VRLog::setTag("PyAPI", true);
}

VRScriptManager::~VRScriptManager() {
    cout << "VRScriptManager destroyed\n";
    blockScriptThreads();
    scripts.clear();
    if (PyErr_Occurred() != NULL) PyErr_Print();
    //Py_XDECREF(pModBase);
    if (PyErr_Occurred() != NULL) PyErr_Print();

    clearModule(pModVR); // helps with fixing cycles
    cleanupPyCallbacks();

    int N = Py_REFCNT(pModVR);
    for (int i=1; i<N; i++) Py_DECREF(pModVR); // reduce the count to 1, clear pModVR in Py_Finalize

    //Py_DECREF(pModVR);
    //clear_sys_modules();
    //clear_global_objects();
    //clear_all_objects();
    //checkGarbageCollection();

    PyErr_Clear();
    cout << " VRScriptManager Py_Finalize\n";
    Py_Finalize();
    VRPyBase::err = 0;
}

int VRScriptManager::getNScripts() { return scripts.size(); }

string VRScriptManager::getIthScriptName(int i) {
    int k = 0;
    for (auto s : scripts) {
        if (k == i) return s.first;
        k++;
    }
    return "";
}

void VRScriptManager::pauseScripts(bool b) {
    uiSignal("on_scripts_paused", {{"paused",toString(b)}});
    for (auto s : scripts) s.second->enable(!b);
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

void VRScriptManager::triggerScript(string fkt, vector<string> params) {
    //cout << "VRScriptManager::triggerScript " << fkt << endl;
    //for (auto p : params) cout << " param: " << p << endl;
    if (!scripts.count(fkt)) return;
    scripts[fkt]->setArguments(params);
    scripts[fkt]->execute();
}

void VRScriptManager::updateScript(string name, string core, bool compile) {
    if (scripts.count(name) == 0) return;

    VRScriptPtr script = scripts[name];
    script->setCore(core);
    if (compile && script->getType() == "Python") script->compile( pGlobal, pModVR );
}


// ---------------------------------- Python stuff -----------------------------------------

static string pyOutConsole = "Console";
static string pyErrConsole = "Errors";

// intercept python stdout
static PyObject* writeOut(PyObject *self, PyObject *args) {
    const char* what = 0;
    if (!PyArg_ParseTuple(args, "s", &what)) return NULL;
#ifndef WITHOUT_IMGUI
    if (what) if (auto c = VRConsoleWidget::get(pyOutConsole)) c->write(what);
#else
    if (what) cout << what;
#endif
    return Py_BuildValue("");
}

// intercept python stderr
static PyObject* writeErr(PyObject *self, PyObject *args) {
    const char* what = 0;
    if (!PyArg_ParseTuple(args, "s", &what)) return NULL;
#ifndef WITHOUT_IMGUI
    if (what) if (auto c = VRConsoleWidget::get(pyErrConsole)) c->write(what);
#else
    if (what) cout << what;
#endif
    return Py_BuildValue("");
}

static PyObject* flushDummy(PyObject *self, PyObject *args) {
    Py_RETURN_NONE;
}

static PyMethodDef methOut[] = {
    {"write", writeOut, METH_VARARGS, "Write something."},
    {"flush", flushDummy, METH_NOARGS, "Dummy flush."},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef methErr[] = {
    {"write", writeErr, METH_VARARGS, "Write something."},
    {"flush", flushDummy, METH_NOARGS, "Dummy flush."},
    {NULL, NULL, 0, NULL}
};

typedef struct {
    PyObject_HEAD
} PyConsoleRedirect;

static PyTypeObject PyOutConsoleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "OutConsoleRedirect",             /* tp_name */
    sizeof(PyConsoleRedirect),    /* tp_basicsize */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    Py_TPFLAGS_DEFAULT,            /* tp_flags */
    "Console redirection object", /* tp_doc */
    0,0,0,0,0,0,
    methOut,              /* tp_methods */
};

static PyTypeObject PyErrConsoleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ErrConsoleRedirect",             /* tp_name */
    sizeof(PyConsoleRedirect),    /* tp_basicsize */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    Py_TPFLAGS_DEFAULT,            /* tp_flags */
    "Console redirection object", /* tp_doc */
    0,0,0,0,0,0,
    methErr,              /* tp_methods */
};

void VRScriptManager::redirectPyOutput(string pyOutput, string console) {
    if (pyOutput == "stdout") {
        if (PyType_Ready(&PyOutConsoleType) < 0) return;
        PyObject* redirector = (PyObject*)PyObject_New(PyConsoleRedirect, &PyOutConsoleType);
        PySys_SetObject(pyOutput.c_str(), redirector);
        pyOutConsole = console;
        Py_DECREF(redirector);
    }

    if (pyOutput == "stderr") {
        if (PyType_Ready(&PyErrConsoleType) < 0) return;
        PyObject* redirector = (PyObject*)PyObject_New(PyConsoleRedirect, &PyErrConsoleType);
        PySys_SetObject(pyOutput.c_str(), redirector);
        pyErrConsole = console;
        Py_DECREF(redirector);
    }
}

// ----------------------------

PyObject* VRScriptManager::getGlobalModule() { return pModVR; }
PyObject* VRScriptManager::getGlobalDict() { return pGlobal; }

static struct PyModuleDef VRModDef = {
    PyModuleDef_HEAD_INIT,
    "VR",                                // module name
    "VR Module",                         // module docstring
    -1,                                  // module state size (-1 if global vars)
    VRSceneGlobals::methods              // method table
};

PyMODINIT_FUNC PyInit_VR(void) {
    auto s = VRScene::getCurrent();
    return s ? s->getGlobalModule() : 0;
}

void VRScriptManager::initPyModules() {
    cout << " initPyModules" << endl;
#if defined(WASM) || defined(WIN32)
    Py_NoSiteFlag = 1;
#endif
    PyImport_AppendInittab("VR", PyInit_VR);

    Py_Initialize();

    cout << "  Py_Initialize done" << endl;
    wchar_t* wargv[1];
    wargv[0] = Py_DecodeLocale("PolyVR", NULL);
    PySys_SetArgv(1, wargv);
    PyMem_RawFree(wargv[0]);

#ifndef WASM
    PyEval_InitThreads();
    cout << "  PyEval_InitThreads done" << endl;
#endif
    VRPyBase::err = PyErr_NewException((char *)"VR.Error", NULL, NULL);

    pGlobal = PyDict_New();

    //Create a new module object
    pModBase = PyImport_AddModule("PolyVR_base");
    PyModule_AddStringConstant(pModBase, "__file__", "");
    pLocal = PyModule_GetDict(pModBase); //Get the dictionary object from my module so I can pass this to PyRun_String

    PyDict_SetItemString(pLocal, "__builtins__", PyEval_GetBuiltins());
    PyDict_SetItemString(pGlobal, "__builtins__", PyEval_GetBuiltins());
    //VRPyListMath::init(pModBase);
    cout << "  Added module PolyVR_base" << endl;

    PyObject* sys_path = PySys_GetObject((char*)"path");
    PyList_Append(sys_path, PyUnicode_FromString(".") );

    pModVR = PyModule_Create(&VRModDef);
    if (!pModVR) {
        cout << "PyModule_Create of pModVR failed!" << endl;
        return;
    }
    PyObject_SetAttrString(pModVR, "__path__", PyList_New(0)); // module becomes a package

    VRSceneModules sceneModules;
    sceneModules.setup(this, pModVR);
    cout << "  Added scene modules" << endl;

	if (!VROptions::get()->getOption<bool>("standalone") && !VROptions::get()->getOption<bool>("headless")) {
        redirectPyOutput("stdout", "Console");
        redirectPyOutput("stderr", "Errors");
	}

    PyRun_SimpleString( // add cython local path to python search path
        "import sys\n"
        "sys.path.append('cython/')\n"
    );
}

PyObject* VRScriptManager::newModule(string name, PyMethodDef* methods, string doc) {
    string name2 = "VR."+name;

    PyModuleDef modDef = {
        PyModuleDef_HEAD_INIT,
        name2.c_str(),                                // module name
        doc.c_str(),                         // module docstring
        -1,                                  // module state size (-1 if global vars)
        methods              // method table
    };

    PyObject* m = PyModule_Create(&modDef);
    PyObject* sysModules = PyImport_GetModuleDict();
    PyDict_SetItemString(sysModules, name2.c_str(), m);
    modules[name] = m;
    PyModule_AddObject(pModVR, name.c_str(), m);
    return m;
}

PyObject* VRScriptManager::getPyModule(string name) {
    if (name == "VR") return pModVR;
    if (modules.count(name)) return modules[name];
    return 0;
}

vector<string> VRScriptManager::getPyVRModules() {
    vector<string> res;
    res.push_back("VR");
    for (auto m : moduleTypes) if (m.first != "VR") res.push_back(m.first);
    sort (res.begin()+1, res.end());
    return res;
}

vector<string> VRScriptManager::getPyVRTypes(string mod) {
    vector<string> res;
    if (moduleTypes.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return res; }
    res.push_back("globals");
    for (auto m : moduleTypes[mod]) res.push_back(m.first);
    sort (res.begin()+1, res.end());
    return res;
}

string VRScriptManager::getPyVRDescription(string mod, string type) {
    if (type == "globals") return "";
    if (moduleTypes.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return ""; }
    if (moduleTypes[mod].count(type) == 0) { cout << "Method " << type << " not found\n"; return ""; }
    return moduleTypes[mod][type]->tp_doc;
}

vector<string> VRScriptManager::getPyVRMethods(string mod, string type) {
    vector<string> res;
    if (moduleTypes.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return res; }
    if (moduleTypes[mod].count(type) == 0 && type != "globals") { cout << "Method " << type << " not found\n"; return res; }
    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    if (type == "globals") {
        if (mod != "VR") return res;
        while (PyDict_Next(dict, &pos, &key, &value)) {
            string name = PyUnicode_AsUTF8(key);
            if (name[0] == '_' && name[1] == '_') continue;
            if (PyCFunction_Check(value)) res.push_back(name);
        }

        sort (res.begin(), res.end());
        return res;
    }

    dict = moduleTypes[mod][type]->tp_dict;
    pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        string name = PyUnicode_AsUTF8(key);
        if (name[0] == '_' && name[1] == '_') continue;
        res.push_back(name);
    }

    sort (res.begin(), res.end());
    return res;
}

string VRScriptManager::getPyVRMethodDoc(string mod, string type, string method) {
    if (moduleTypes.count(mod) == 0) { cout << "Module " << mod << " not found\n"; return ""; }
    if (moduleTypes[mod].count(type) == 0 && type != "globals") { cout << "Method " << type << " not found\n"; return ""; }
    string res;

    PyObject* dict = PyModule_GetDict(pModVR);
    PyObject *key, *meth;
    Py_ssize_t pos = 0;

    if (type == "globals") {
        if (mod != "VR") return res;
        while (PyDict_Next(dict, &pos, &key, &meth)) {
            string name = PyUnicode_AsUTF8(key);
            if (method != name) continue;
            if (!PyCFunction_Check(meth)) continue;
            PyCFunctionObject* cfo =  (PyCFunctionObject*)meth;
            return cfo->m_ml->ml_doc;
        }
    }

    pos = 0;
    dict = moduleTypes[mod][type]->tp_dict;
    while (PyDict_Next(dict, &pos, &key, &meth)) {
        string name = PyUnicode_AsUTF8(key);
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

void VRScriptManager::triggerOnLoad() {
    cout << "VRScriptManager::triggerOnLoad " << scripts.size() << endl;
    for (auto script : scripts) if (script.second->hasTrigger("on_scene_load")) script.second->queueExecution();
}

void VRScriptManager::triggerOnImport() { // deprecated
    cout << "VRScriptManager::triggerOnImport" << endl;
    for (auto script : scripts) {
        if (script.second->hasTrigger("on_scene_import")) {
            script.second->execute();
            //script.second->queueExecution(); // not working??
        }
    }
}

string hudSite = TEMPLATE(
<!DOCTYPE html>\n
<html>\n\n

<head>\n
\t<style type="text/css">\n
\t\tbody {\n
\t\t\tmargin:0;\n
\t\t}\n\n
\t\tbutton {\n
\t\t\tfont-size:10vh;\n
\t\t\twidth:100vw;\n
\t\t\theight:20vh;\n
\t\t}\n
\t</style>\n
\t<script>\n
\t\tvar websocket = new WebSocket('ws://localhost:$PORT_server1$');\n
\t\twebsocket.onopen = function() { send('register|hud'); };\n
\t\twebsocket.onerror = function(e) {};\n
\t\twebsocket.onmessage = function(m) { if(m.data) handle(m.data); };\n
\t\twebsocket.onclose = function(e) {};\n\n

\t\tfunction send(m) { websocket.send(m); };\n
\t\tfunction handle(m) { console.log(m); };\n
\t</script>\n
</head>\n\n

<body>\n
\t<button onclick="send('message1 from hud')">send message1</button>\n
\t<button onclick="send('message2 from hud')">send message2</button>\n
\t<button onclick="send('message3 from hud')">send message3</button>\n
\t<button onclick="send('message4 from hud')">send message4</button>\n
\t<button onclick="send('message5 from hud')">send message5</button>\n
</body>\n
</html>
);

string hudInit = TEMPLATE(
\timport VR\n\n
\tdef addHud(site,w,h,x,y,parent):\n
\t\ts = VR.Sprite('site')\n
\t\ts.setSize(w,h)\n
\t\tport = VR.find('server1').getPort()\n
\t\ts.webOpen('http://localhost:'+str(port)+'/'+site, 400, w/h)\n
\t\ts.setFrom([x,y,-2])\n
\t\tparent.addChild(s)\n\n
\tif hasattr(VR, 'hud'): VR.hud.destroy()\n
\tVR.hud = VR.Object('hud')\n
\tVR.find('Default').addChild(VR.hud)\n\n
\taddHud( 'hudSite', 0.5,0.5, 0,1, VR.hud )\n
);

string hudHandler = TEMPLATE(
\timport VR\n\n
\tm = dev.getMessage()\n
\tprint m\n
);

string restClient = TEMPLATE(
\timport VR\n\n
\tif not hasattr(VR, 'client'): VR.client = VR.RestClient()\n\n
\tdef cb(r):\n
\t\tprint 'async: ' + r.getData()\n\n
\tVR.client.getAsync("http://reqbin.com/echo/get/json", cb)\n\n
\tres = VR.client.get("http://reqbin.com/echo/get/json")\n
\tprint 'sync: ' + res.getData()\n
);

string OrderedDict = TEMPLATE(
\tclass OrderedDict:\n
\t\tdef __init__(self):\n
\t\t\tself.dict = {}\n
\t\t\tself.keys = []\n\n
\t\tdef __setitem__(self, k, v):\n
\t\t\tself.dict[k] = v\n
\t\t\tself.keys.append(k)\n\n
\t\tdef __getitem__(self, k):\n
\t\t\treturn self.dict[k]\n\n
\t\tdef items(self):\n
\t\t\treturn [ (k,self.dict[k]) for k in self.keys ]\n
);

string pointCloudImport = TEMPLATE(
\timport VR\n\n
\tif hasattr(VR, 'scene'): VR.scene.destroy()\n
\tVR.scene = VR.Object('scene', 'light')\n\n
\topts = {}\n
\topts['downsampling'] = 1\n
\topts['lit'] = 0\n
\topts['resolution'] = 2\n
\topts['pointSize'] = 5\n
\topts['lod1'] = [5, 20]\n
\topts['lod2'] = [10, 200]\n
\topts['swapYZ'] = 1\n
\topts['keepOctree'] = 0\n\n
\tpath = 'data/myPC.e57'\n
\tpc = VR.loadGeometry(path, options = opts)\n
\tVR.scene.addChild(pc)\n
);

string simpleVP =
"#version 120\n"
TEMPLATE(
attribute vec4 osg_Vertex;\n
attribute vec3 osg_Normal;\n
attribute vec4 osg_Color;\n
attribute vec2 osg_MultiTexCoord0;\n\n
varying vec4 vertPos;\n
varying vec3 vertNorm;\n
varying vec4 color;\n
void main(void) {\n
\tvertPos = gl_ModelViewMatrix * osg_Vertex;\n
\tvertNorm = gl_NormalMatrix * osg_Normal;\n
\tgl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);\n
\tcolor = gl_Color;\n
\tgl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;\n
}\n
);

string simpleFP =
"#version 120\n"
TEMPLATE(
varying vec4 vertPos;\n
varying vec3 vertNorm;\n
varying vec4 color;\n
uniform sampler2D tex0;\n\n
void applyLightning() {\n
\tvec3 n = normalize(vertNorm);\n
\tvec3 light;\n
\tif (gl_LightSource[0].position.w < 0.5) light = normalize( gl_LightSource[0].position.xyz ); // dir light\n
\telse light = normalize( gl_LightSource[0].position.xyz - vertPos.xyz ); // pnt light\n
\tfloat NdotL = max(dot( n, light ), 0.0);\n
\tvec4 ambient = gl_LightSource[0].ambient * color;\n
\tvec4 diffuse = gl_LightSource[0].diffuse * NdotL * color;\n
\tfloat NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);\n
\tvec4 specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );\n
\tgl_FragColor = ambient + diffuse + specular;\n
}\n\n
void main(void) {\n
\tvec3 pos = vertPos.xyz / vertPos.w;\n
\tvec4 diffCol = texture2D(tex0, gl_TexCoord[0].xy);\n
\tapplyLightning();\n
}\n
);

struct VRScriptTemplate {
    string name;
    string type;
    string core;
    vector<VRScript::trig> trigs;
    vector<VRScript::arg>  args;
};

void VRScriptManager::importTemplate(string n) {
    if (!templates.count(n)) return;
    auto& t = templates[n];
    VRScriptPtr script = VRScript::create(t.name);
    if (t.type == "shaders") script->setType("GLSL");
    if (t.type == "websites") script->setType("HTML");

    for (auto& tr : t.trigs) {
        auto tri = script->addTrigger();
        tri->trigger = tr.trigger;
        tri->param = tr.param;
        tri->dev = tr.dev;
        tri->key = tr.key;
        tri->state = tr.state;
    }
    script->updateDeviceTrigger();

    script->setCore(t.core);
    addScript( script );
}

void VRScriptManager::initTemplates() {
    auto addTemplate = [&](string type, string name, string core) {
        VRScriptTemplate s;
        s.name = name;
        s.type = type;
        s.core = core;
        templates[name] = s;
    };

    auto addTrigger = [&](string name, string t, string p = "", string d = "none", int k = 0, string s = "Pressed") {
        VRScript::trig tr;
        tr.trigger = t;
        tr.param = p;
        tr.dev = d;
        tr.key = k;
        tr.state = s;
        templates[name].trigs.push_back(tr);
    };

    if (templates.size() == 0) {
        addTemplate("scripts", "onClick", "\timport VR\n\n\tif dev.intersect():\n\t\ti = dev.getIntersected()\n\t\tp = dev.getIntersection()\n\t\tprint i.getName(), p");
        addTrigger("onClick", "on_device", "0", "mouse");
        addTemplate("scripts", "hudInit", hudInit);
        addTemplate("scripts", "hudHandler", hudHandler);
        addTrigger("hudHandler", "on_device", "0", "server1", -1, "Released");
        addTemplate("websites", "hudSite", hudSite);
        addTemplate("shaders", "simpleVP", simpleVP);
        addTemplate("shaders", "simpleFP", simpleFP);
        addTemplate("scripts", "restClient", restClient);
        addTemplate("scripts", "OrderedDict", OrderedDict);
        addTemplate("scripts", "pointCloudImport", pointCloudImport);
    }
}

map<string, vector<string>> VRScriptManager::getTemplates() {
    initTemplates();
    map<string, vector<string>> res;
    for (auto& t : templates) res[t.second.type].push_back(t.first);
    return res;
}

string VRScriptManager::getTemplateCore(string t) {
    initTemplates();
    if (!templates.count(t)) return "";
    return templates[t].core;
}

OSG_END_NAMESPACE
