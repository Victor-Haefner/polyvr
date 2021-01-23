#include "VRScript.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#endif
#include <iostream>
#include <functional>
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyLight.h"
#include "VRPyLod.h"
#include "core/setup/devices/VRServer.h"
#include "VRPySocket.h"
#include "VRPyMouse.h"
#ifndef WITHOUT_MTOUCH
#include "VRPyMultiTouch.h"
#endif
#ifndef WITHOUT_VIRTUOSE
#include "VRPyHaptic.h"
#endif
#include "VRPyMobile.h"
#include "VRPyBaseT.h"
#include "addons/LeapMotion/VRPyLeap.h"
#include "core/utils/VRTimer.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/setup/VRSetup.h"
#include "core/objects/material/VRMaterial.h"
#include <frameobject.h>
#include <pyerrors.h>

using namespace OSG;

template<> string typeName(const VRScript& t) { return "Script"; }


void updateArgPtr(VRScript::argPtr a) {
    string t = a->type;
    auto scene = VRScene::getCurrent();

    if (t == "VRPyObjectType" || t == "VRPyGeometryType" || t == "VRPyTransformType" || t == "VRPyLightType" || t == "VRPyLodType" || t == "VRPyLeapFrameType") {
        a->ptr = (void*)scene->get(a->val).get();
        return;
    }
    if (t == "VRPySocketType") {
        a->ptr = (void*)scene->getSocket(a->val).get();
        return;
    }
    if (t == "VRPyDeviceType" || t == "VRPyMouseType" || t == "VRPyHapticType" || t == "VRPyServerType") {
        auto setup = VRSetup::getCurrent();
        if (setup) a->ptr = (void*)setup->getDevice(a->val).get();
        return;
    }

    if (t == "int" || t == "float" || t == "str" || t == "NoneType") return;

    cout << "\nupdateArgPtr: " << t << " is an unknown argument type!" << endl;
}

VRScript::trig::trig() { setName("trigger"); }
VRScript::arg::arg(string nspace, string name) {
    auto ns = setNameSpace(nspace);
    ns->setSeparator('_');
    setName(name);
}

VRScript::trig::~trig() {}
VRScript::arg::~arg() {}

VRScript::trigPtr VRScript::trig::create() { return VRScript::trigPtr( new trig() ); }
VRScript::argPtr VRScript::arg::create(string nspace, string name) { return VRScript::argPtr( new arg(nspace, name) ); }

void VRScript::clean() {
    if ( auto setup = VRSetup::getCurrent() ) {
        VRServerPtr mob = dynamic_pointer_cast<VRServer>( setup->getDevice(server) );
        if (mob) mob->remWebSite(getName());
    }

    auto scene = VRScene::getCurrent();

    if (devArg) devArg = 0;
    if (socArg) socArg = 0;

    for (auto t : trigs) {
        if (t->soc) t->soc->unsetCallbacks();
        if (t->sig) t->sig->sub(cbfkt_dev);
        if (t->trigger == "on_timeout") scene->dropTimeoutFkt(cbfkt_sys);
        if (t->trigger == "on_scene_close") VRSceneManager::get()->getSignal_on_scene_close()->sub(cbfkt_sys);
        t->soc = 0;
        t->sig = 0;
        t->a = 0;
    }
}

void VRScript::update() {
    if (type == "HTML") {
        if (VRSetup::getCurrent()) {
            VRServerPtr mob = dynamic_pointer_cast<VRServer>( VRSetup::getCurrent()->getDevice(server) );
            if (mob) mob->addWebSite(getName(), core);
        }
    }

    auto scene = VRScene::getCurrent();
    if (!scene) return;

    for (auto t : trigs) {
        if (t->trigger == "on_scene_close") {
            VRSceneManager::get()->getSignal_on_scene_close()->add(cbfkt_sys);
            continue;
        }

        if (t->trigger == "on_timeout") {
            int i = toInt(t->param);
            scene->addTimeoutFkt(cbfkt_sys, 0, i);
            continue;
        }

        if (t->trigger == "on_device") {
            auto s = VRSetup::getCurrent();
            if (!s) continue;
            VRDevicePtr dev = s->getDevice(t->dev);
            int state = -1;
            if (t->state == "Released") state = 0;
            if (t->state == "Pressed") state = 1;
            if (t->state == "Drag") state = 2;
            if (t->state == "Drop") state = 3;
            if (t->state == "To edge") state = 4;
            if (t->state == "From edge") state = 5;
            if (state == -1) continue;

            if (dev != 0) {
                if (state <= 1) t->sig = dev->newSignal(t->key, state);
                if (state == 2) t->sig = dev->getDragSignal();
                if (state == 3) t->sig = dev->getDropSignal();
                if (state == 4) t->sig = dev->getToEdgeSignal();
                if (state == 5) t->sig = dev->getFromEdgeSignal();
                if (t->sig == 0) continue;
                t->sig->add(cbfkt_dev);
            }

            // add dev argument
            if (!devArg) devArg = arg::create(VRName::getName(), "dev");
            devArg->type = "VRPyDeviceType";
            devArg->val = "";
            devArg->trig = true;
            t->a = devArg;
            continue;
        }

        if (t->trigger == "on_socket") {
            t->soc = scene->getSocket(t->dev);
            if (t->soc == 0) continue;
            t->soc->setTCPCallback(cbfkt_soc);

            // add msg argument
            if (!socArg) socArg = arg::create(VRName::getName(), "msg");
            socArg->type = "str";
            socArg->val = "";
            t->a = socArg;
            socArg->trig = true;
            continue;
        }
    }

    // update args namespaces
    for (auto a : args) {
        a->setNameSpace(VRName::getName());
        changeArgValue(a->getName(), a->val);
    }

    // update head
    head = "";
    if (type == "Python") {
        head = "def " + name + "(";
        bool first = true;
        for (auto a : getArguments()) {
            if (!first) head += ", ";
            head += a->getName();
            first = false;
        }
        head += "):\n";
    }
}

VRScript::VRScript(string _name) {
    setStorageType("Script");
    auto ns = setNameSpace("__script__");
    ns->setSeparator('_');
    setName(_name);
    cbfkt_sys = VRUpdateCb::create(_name + "_ScriptCallback_sys", bind(&VRScript::execute, this));
    cbfkt_dev = VRDeviceCb::create(_name + "_ScriptCallback_dev", bind(&VRScript::execute_dev, this, _1));
    cbfkt_soc = VRMessageCb::create(_name + "_ScriptCallback_soc", bind(&VRScript::execute_soc, this, _1));

    setOverrideCallbacks(true);
    store("type", &type);
    store("server", &server);
    store("group", &group);
}

VRScript::~VRScript() {
    for (auto t : trigs) {
        if (t->trigger == "on_scene_close") VRSceneManager::get()->getSignal_on_scene_close()->sub(cbfkt_sys);
    }
}

VRScriptPtr VRScript::create(string name) { return VRScriptPtr( new VRScript(name) ); }
VRScriptPtr VRScript::ptr() { return shared_from_this(); }

VRScript::argPtr VRScript::addArgument() {
    clean();
    argPtr a = arg::create(VRName::getName());
    args.push_back(a);
    update();
    return a;
}

PyObject* VRScript::getPyObj(argPtr a) {
    updateArgPtr(a);
    if (a->type == "int") return Py_BuildValue("i", toInt(a->val.c_str()));
    else if (a->type == "float") return Py_BuildValue("f", toFloat(a->val.c_str()));
    else if (a->type == "NoneType") return Py_None;
    else if (a->type == "str") return PyString_FromString(a->val.c_str());
    else if (a->ptr == 0) { /*cout << "\ngetPyObj ERROR: " << a->type << " ptr is 0\n";*/ Py_RETURN_NONE; }
    else if (a->type == "VRPyObjectType") return VRPyObject::fromSharedPtr(((VRObject*)a->ptr)->ptr());
    else if (a->type == "VRPyTransformType") return VRPyTransform::fromSharedPtr(((VRTransform*)a->ptr)->ptr());
    else if (a->type == "VRPyGeometryType") return VRPyGeometry::fromSharedPtr( ((VRGeometry*)a->ptr)->ptr() );
    else if (a->type == "VRPyLightType") return VRPyLight::fromSharedPtr(((VRLight*)a->ptr)->ptr());
    else if (a->type == "VRPyLodType") return VRPyLod::fromSharedPtr(((VRLod*)a->ptr)->ptr());
    else if (a->type == "VRPyDeviceType") return VRPyDevice::fromSharedPtr(((VRDevice*)a->ptr)->ptr());
    else if (a->type == "VRPyMouseType") return VRPyMouse::fromSharedPtr(((VRMouse*)a->ptr)->ptr());
#ifndef WITHOUT_MTOUCH
    else if (a->type == "VRPyMultiTouchType") return VRPyMultiTouch::fromSharedPtr(((VRMultiTouch*)a->ptr)->ptr());
#endif
#ifndef WITHOUT_VIRTUOSE
    else if (a->type == "VRPyHapticType") return VRPyHaptic::fromSharedPtr(((VRHaptic*)a->ptr)->ptr());
#endif
    else if (a->type == "VRPyServerType") return VRPyServer::fromSharedPtr(((VRServer*)a->ptr)->ptr());
    else if (a->type == "VRPyLeapFrameType") return VRPyLeapFrame::fromSharedPtr(((VRLeapFrame*)a->ptr)->ptr());
    //else if (a->type == "VRPySocketType") return VRPySocket::fromSharedPtr(((VRSocket*)a->ptr)->ptr());
    else { cout << "\ngetPyObj ERROR: " << a->type << " unknown!\n"; Py_RETURN_NONE; }
}

VRScript::argPtr VRScript::getArg(string name) {
    for (auto a : args) if (a->getName() == name) return a;
    return 0;
}

VRScript::trigPtr VRScript::getTrig(string name) {
    for (auto t : trigs) if (t->getName() == name) return t;
    return 0;
}

void VRScript::changeArgName(string name, string _new) {
    if (auto a = getArg(name)) {
        clean();
        a->setName(_new);
    }
    update();
}

void VRScript::changeArgValue(string name, string _new) {
    if (auto a = getArg(name)) {
        a->val = _new;
        a->ptr = 0;
        updateArgPtr(a);
    }
}

void VRScript::changeArgType(string name, string _new) {
    if (auto a = getArg(name)) {
        a->type = _new;
        a->val = "0";
    }
}

VRScript::Search VRScript::getSearch() { return search; }
VRScript::Search VRScript::find(string s) {
    search = Search();
    if (s == "") return search;

    search.search = s;
    map<int, bool> res;

    unsigned int pos = core.find(s, 0);
    while(pos != string::npos && pos <= core.size()) {
        res[pos] = false;
        pos = core.find(s, pos+1);
    }
    pos = core.find("\n", 0);
    while(pos != string::npos && pos <= core.size()) {
        res[pos] = true;
        pos = core.find("\n", pos+1);
    }

    int l = getHeadSize()+1;
    int lpo = 0;
    for (auto r : res) {
        if (r.second) { l++; lpo = r.first; continue; } // new line
        if (search.result.count(l) == 0) search.result[l] = vector<int>();
        search.result[l].push_back(r.first - lpo);
    }

    search.N = search.result.size();

    return search;
}

list<VRScript::argPtr> VRScript::getArguments() {
    auto tmp = args;
    if (socArg) tmp.push_front(socArg);
    if (devArg) tmp.push_front(devArg);
    return tmp;
}

void VRScript::setArguments(vector<string> vals) {
    size_t i=0;
    for (auto a : args) {
        if (i >= vals.size()) return;
        a->val = vals[i];
        a->type = "str";
        i++;
    }
}

void VRScript::setName(string n) { clean(); VRName::setName(n); update(); }
void VRScript::setFunction(PyObject* fkt) { this->fkt = fkt; }
void VRScript::setCore(string core) { clean(); this->core = core; update(); }
void VRScript::setType(string type) { clean(); this->type = type; update(); }
void VRScript::setHTMLHost(string server) { clean(); this->server = server; update(); }

string VRScript::getCore() { return core; }
string VRScript::getHead() { return head; }
string VRScript::getScript() { return head + core; }
string VRScript::getType() { return type; }
string VRScript::getServer() { return server; }

int VRScript::getHeadSize() { // number of head lines
    if (type == "Python") return 1;
    return 0;
}

void VRScript::on_err_link_clicked(errLink link, string s) {
    //VRGuiManager::get()->focusScript(getName(), link.line, link.column);
#ifndef WITHOUT_GTK
    VRGuiManager::get()->focusScript(link.filename, link.line, link.column);
#endif
}

VRScript::errLink::errLink(string f, int l, int c) : filename(f), line(l), column(c) {}


int parse_syntax_error(PyObject *err, PyObject **message, char **filename, int *lineno, int *offset, char **text) {
    long hold;
    PyObject *v;

    /* old style errors */
    if (PyTuple_Check(err)) return PyArg_ParseTuple(err, "O(ziiz)", message, filename, lineno, offset, text);

    /* new style errors.  `err' is an instance */
    if (! (v = PyObject_GetAttrString(err, "msg"))) goto finally;
    *message = v;

    if (!(v = PyObject_GetAttrString(err, "filename"))) goto finally;
    if (v == Py_None) *filename = NULL;
    else if (! (*filename = PyString_AsString(v))) goto finally;

    Py_DECREF(v);
    if (!(v = PyObject_GetAttrString(err, "lineno"))) goto finally;
    hold = PyInt_AsLong(v);
    Py_DECREF(v);
    v = NULL;
    if (hold < 0 && PyErr_Occurred()) goto finally;
    *lineno = (int)hold;

    if (!(v = PyObject_GetAttrString(err, "offset"))) goto finally;
    if (v == Py_None) {
        *offset = -1;
        Py_DECREF(v);
        v = NULL;
    } else {
        hold = PyInt_AsLong(v);
        Py_DECREF(v);
        v = NULL;
        if (hold < 0 && PyErr_Occurred())
            goto finally;
        *offset = (int)hold;
    }

    if (!(v = PyObject_GetAttrString(err, "text"))) goto finally;
    if (v == Py_None) *text = NULL;
    else if (! (*text = PyString_AsString(v))) goto finally;
    Py_DECREF(v);
    return 1;

finally:
    Py_XDECREF(v);
    return 0;
}

void print_error_text(int offset, char *text) {
    auto print = [&]( string m, string style = "", shared_ptr< VRFunction<string> > link = 0 ) {
#ifndef WITHOUT_GTK
        VRGuiManager::get()->getConsole( "Syntax" )->write( m, style, link );
#endif
    };

    char *nl;
    if (offset >= 0) {
        if (offset > 0 && offset == (int)strlen(text) && text[offset - 1] == '\n') offset--;
        for (;;) {
            nl = strchr(text, '\n');
            if (nl == NULL || nl-text >= offset) break;
            offset -= (int)(nl+1-text);
            text = nl+1;
        }
        while (*text == ' ' || *text == '\t') {
            text++;
            offset--;
        }
    }
    print("    ");
    print(text);
    if (*text == '\0' || text[strlen(text)-1] != '\n') print("\n");
    if (offset == -1) return;
    print("    ");
    offset--;
    while (offset > 0) {
        print(" ");
        offset--;
    }
    print("^\n");
}

void VRScript::printSyntaxError(PyObject *exception, PyObject *value, PyObject *tb) {
    auto print = [&]( string m, string style = "", shared_ptr< VRFunction<string> > link = 0 ) {
#ifndef WITHOUT_GTK
        VRGuiManager::get()->getConsole( "Syntax" )->write( m, style, link );
#endif
    };

    int err = 0;
    Py_INCREF(value);
    if (Py_FlushLine()) PyErr_Clear();
    fflush(stdout);
    if (err == 0 && PyObject_HasAttrString(value, "print_file_and_line")) {
        PyObject *message;
        char *filename, *text;
        int lineno, offset;
        if (!parse_syntax_error(value, &message, &filename, &lineno, &offset, &text)) PyErr_Clear();
        else {
            string fn = filename ? filename : "<string>";
            errLink eLink(fn, lineno, 0);
            auto fkt = VRFunction<string>::create("search_link", bind(&VRScript::on_err_link_clicked, this, eLink, _1) );
            print("  ");
            print("Script \"" + fn + "\", line " + toString(lineno), "redLink", fkt);
            print("\n");
            if (text != NULL) print_error_text(offset, text);
            Py_DECREF(value);
            value = message;
            if (PyErr_Occurred()) err = -1;
        }
    }

    Py_DECREF(value);
    PyErr_Clear();
}

void VRScript::pyErrPrint(string channel) {
    if (!PyErr_Occurred()) return;

    auto print = [&]( string m, string style = "", shared_ptr< VRFunction<string> > link = 0 ) {
#ifndef WITHOUT_GTK
        VRGuiManager::get()->getConsole( channel )->write( m, style, link );
#else
        cout << m << endl;
#endif
    };

    auto getTracebackFrame = [](PyTracebackObject* tb, vector<PyFrameObject*>& frames) {
        while (tb->tb_next) tb = tb->tb_next;
        if (tb->tb_frame) frames.push_back(tb->tb_frame);
    };

    auto getThreadStateFrames = [&](PyThreadState* tstate) {
        vector<PyFrameObject*> frames;
        if (tstate->frame) frames.push_back(tstate->frame);
        if (auto tb = (PyTracebackObject*)tstate->exc_traceback) getTracebackFrame(tb, frames);
        if (auto tb = (PyTracebackObject*)tstate->curexc_traceback) getTracebackFrame(tb, frames);
        return frames;
    };

#ifndef WITHOUT_GTK
    VRGuiManager::get()->getConsole( channel )->addStyle( "redLink", "#ff3311", "#ffffff", false, false, true );
#endif

    struct Line {
        shared_ptr<VRFunction<string>> fkt;
        string line;
    };
    list<Line> lines;

    PyThreadState* tstate = PyThreadState_GET();
    for (auto frame : getThreadStateFrames(tstate)) {
        while (frame) {
            int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
            string filename = PyString_AsString(frame->f_code->co_filename);
            string funcname = PyString_AsString(frame->f_code->co_name);
            errLink eLink(filename, line, 0);
            Line l;
            l.fkt = VRFunction<string>::create("search_link", bind(&VRScript::on_err_link_clicked, this, eLink, _1) );
            //l.line = "Line "+toString(line)+" in "+funcname+" in script "+filename;
            l.line = "Script "+filename+", line "+toString(line);
            if (filename != funcname) l.line += ", in "+funcname;
            lines.push_front(l);
            frame = frame->f_back;
        }
    }

    if (lines.size() > 0) { // print trace back
        print( "Traceback (most recent call last):\n" );
        for (auto l : lines) {
            print( "  " );
            print( l.line, "redLink", l.fkt );
            print( "\n" );
        }
    }

    // print error
    PyObject *exception, *v, *tb;
    PyErr_Fetch(&exception, &v, &tb);
    if (exception == NULL) return;
    PyErr_NormalizeException(&exception, &v, &tb);
    if (exception == NULL) return;

    printSyntaxError(exception, v, tb);

    if (v != NULL && v != Py_None) print( string(PyString_AsString( PyObject_Str(v) )) + "\n");
    Py_XDECREF(exception);
    Py_XDECREF(v);
    Py_XDECREF(tb);
    PyErr_Clear();
}

PyObject* VRScript::getFunction() { return fkt; }

void VRScript::compile( PyObject* pGlobal, PyObject* pModVR ) {
    setFunction( 0 );
    PyObject* pCode = Py_CompileString(getScript().c_str(), getName().c_str(), Py_file_input);
    if (!pCode) { pyErrPrint("Syntax"); return; }
    PyObject* pValue = PyEval_EvalCode((PyCodeObject*)pCode, pGlobal, PyModule_GetDict(pModVR));
    pyErrPrint("Errors");
    if (!pValue) return;
    Py_DECREF(pCode);
    Py_DECREF(pValue);
    setFunction( PyObject_GetAttrString(pModVR, name.c_str()) );
}

void VRScript::execute() {
    if (type == "Python") {
        if (!isInitScript && VRGlobals::CURRENT_FRAME <= loadingFrame + 2) return; // delay timeout scripts
        if (fkt == 0 || !active) return;
        PyGILState_STATE gstate = PyGILState_Ensure();
        pyErrPrint( "Errors" );

        VRTimer timer; timer.start();
        auto args = getArguments();
        PyObject* pArgs = PyTuple_New(args.size());
        pyErrPrint("Errors");

        int i=0;
        for (auto a : args) {
            a->pyo = getPyObj(a);
            PyTuple_SetItem(pArgs, i, a->pyo);
            pyErrPrint("Errors");
            i++;
        }

        auto res = PyObject_CallObject(fkt, pArgs);
        pyErrPrint("Errors");
        if (!res) { cout << "Warning in VRScript::execute: PyObject_CallObject failed! in script " << name << endl; return; }

        execution_time = timer.stop();

        Py_XDECREF(pArgs);
        pyErrPrint("Errors");
        PyGILState_Release(gstate);
    }

    if (type == "HTML") {
        VRServerPtr mob = dynamic_pointer_cast<VRServer>( VRSetup::getCurrent()->getDevice(server) );
        if (mob) mob->updateClients(getName());
    }

    if (type == "GLSL") {
        string name = getName();
        for (auto m : VRMaterial::materials) {
            auto mat = m.second.lock();
            if (!mat) continue;
            if (mat->getVertexScript() == name) mat->setVertexScript(name);
            if (mat->getFragmentScript() == name) mat->setFragmentScript(name);
            if (mat->getFragmentScript(true) == name) mat->setFragmentScript(name, true);
            if (mat->getGeometryScript() == name) mat->setGeometryScript(name);
            if (mat->getTessControlScript() == name) mat->setTessControlScript(name);
            if (mat->getTessEvaluationScript() == name) mat->setTessEvaluationScript(name);

            /*cout << "VRScript::execute GLSL " << name << " " << mat->getName() << " " << mat->getFragmentScript() << " "
                << (mat->getVertexScript() == name) << (mat->getFragmentScript() == name)
                << (mat->getFragmentScript(true) == name) << (mat->getGeometryScript() == name)
                << (mat->getTessControlScript() == name) << (mat->getTessEvaluationScript() == name) << endl;*/
        }
    }
}

void VRScript::execute_dev(VRDeviceWeakPtr _dev) {
    auto dev = _dev.lock();
    if (!dev || !devArg) return;
    if (type != "Python") return;

    devArg->type = "VRPyDeviceType";
    if (dev->getType() == "haptic") devArg->type = "VRPyHapticType";
    if (dev->getType() == "server") devArg->type = "VRPyServerType";
    devArg->val = dev->getName();
    devArg->ptr = dev.get();
    execute();
    devArg->val = "";
}

void VRScript::execute_soc(string s) {
    if (type != "Python" || !active) return;
    socArg->val = s;
    execute();
}

void VRScript::enable(bool b) { active = b; }
bool VRScript::enabled() { return active; }

list<VRScript::trigPtr> VRScript::getTriggers() { return trigs; }
VRScript::trigPtr VRScript::addTrigger() { auto t = trig::create(); trigs.push_back(t); return t; }
void VRScript::changeTrigger(string name, string trigger) { clean(); if (auto t = getTrig(name)) t->trigger = trigger; update(); }
void VRScript::changeTrigDev(string name, string dev) { clean(); if (auto t = getTrig(name)) t->dev = dev; update(); }
void VRScript::changeTrigParams(string name, string params) { clean(); if (auto t = getTrig(name)) t->param = params; update(); }
void VRScript::changeTrigKey(string name, int key) { clean(); if (auto t = getTrig(name)) t->key = key; update(); }
void VRScript::changeTrigState(string name, string state) { clean(); if (auto t = getTrig(name)) t->state = state; update(); }
bool VRScript::hasTrigger(string type) { for (auto t : trigs) if (t->trigger == type) return true; return false; }

void VRScript::updateDeviceTrigger() { // TODO: optimize
    clean();
    update();
}

void VRScript::remTrigger(string name) {
    if (auto t = getTrig(name)) {
        clean();
        trigs.remove(t);
        update();
    }
}

float VRScript::getExecutionTime() { return execution_time; }

void VRScript::remArgument(string name) {
    if (auto a = getArg(name)) {
        clean();
        args.remove(a);
        update();
    }
}

void VRScript::setGroup(string g) { group = g; }
string VRScript::getGroup() { return group; }

void VRScript::save(XMLElementPtr e) {
    XMLElementPtr ec = e->addChild("core");
    ec->setText("\n"+core+"\n");

    for (auto a : args) {
        if (a->trig) continue;
        XMLElementPtr ea = e->addChild("arg");
        ea->setAttribute("type", a->type);
        ea->setAttribute("value", a->val);
        a->save(ea);
    }

    for (auto t : trigs) {
        XMLElementPtr ea = e->addChild("trig");
        ea->setAttribute("type", t->trigger);
        ea->setAttribute("dev", t->dev);
        ea->setAttribute("state", t->state);
        ea->setAttribute("param", t->param);
        ea->setAttribute("key", toString(t->key));
        t->save(ea);
    }
}

void VRScript::load(XMLElementPtr e) {
    clean();
    VRName::load(e);
    if (e->hasAttribute("core")) core = e->getAttribute("core");
    if (e->hasAttribute("type")) type = e->getAttribute("type");
    if (e->hasAttribute("server")) server = e->getAttribute("server");
    if (e->hasAttribute("group")) group = e->getAttribute("group");

    for (auto el : e->getChildren() ) {
        if (!el) continue;

        string name = el->getName();

        if (name == "core") {
            if (el->hasText()) {
                core = el->getText();
                core = core.substr(1,core.size()-2);
            }
        }

        if (name == "arg") {
            argPtr a = addArgument();
            a->type = el->getAttribute("type");
            a->val  = el->getAttribute("value");
            string oname = a->getName();
            a->load(el);
            changeArgName(oname, a->getName());
        }

        if (name == "trig") {
            trigPtr t = trig::create();
            t->trigger = el->getAttribute("type");
            t->dev = el->getAttribute("dev");
            if (t->dev == "mobile") t->dev = "server1"; // Temp fix for old scenes after changing default server name!
            t->state = el->getAttribute("state");
            t->param = el->getAttribute("param");
            t->key = toInt( el->getAttribute("key") );
            t->load(el);
            trigs.push_back(t);

            if ((t->trigger == "on_scene_load" && active) || (t->trigger == "on_scene_import" && active)) {
                //queueExecution();
                isInitScript = true;
                loadingFrame = VRGlobals::CURRENT_FRAME;
            }
        }
    }

    update();
}

void VRScript::queueExecution() {
    auto scene = VRScene::getCurrent();
    scene->queueJob(cbfkt_sys);
}

VRGlobals::Int VRScript::loadingFrame = 0;

