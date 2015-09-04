#ifndef VRSCRIPTMANAGER_H_INCLUDED
#define VRSCRIPTMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <string>
#include "core/setup/devices/VRSignal.h"
#include "core/utils/VRStorage.h"
#include "VRPyBase.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScript;

class VRScriptManager : public VRStorage, public VRPyBase {
    private:
        PyObject* pGlobal;
        PyObject* pLocal;
        PyObject* pModBase;
        PyObject* pModVR;
        map<string, PyTypeObject*> modules;
        map<string, VRScript*> scripts;
        map<string, VRSignal*> triggers;
        PyThreadState* pyThreadState = 0;

        template<class T>
        void registerModule(string mod, PyObject* parent, PyTypeObject* base = 0);

        void test();

        void initPyModules();

    protected:
        void update();

    public:
        VRScriptManager();
        ~VRScriptManager();

        void allowScriptThreads();
        void blockScriptThreads();

        VRScript* newScript(string name, string function);
        void addScript(VRScript* script);
        void remScript(string name);

        void disableAllScripts();

        void updateScript(string name, string core, bool compile = true);
        VRScript* changeScriptName(string name, string new_name);

        void triggerScript(string name);

        VRScript* getScript(string name);
        map<string, VRScript*> getScripts();

        vector<VRScript*> searchScript(string s, VRScript* sc = 0);

        vector<string> getPyVRTypes();
        vector<string> getPyVRMethods(string type);
        string getPyVRDescription(string type);
        string getPyVRMethodDoc(string type, string method);

        // Python Methods
		static PyObject* exit(VRScriptManager* self);
		static PyObject* loadGeometry(VRScriptManager* self, PyObject *args);
		static PyObject* pyTriggerScript(VRScriptManager* self, PyObject *args);
		static PyObject* stackCall(VRScriptManager* self, PyObject *args);
		static PyObject* openFileDialog(VRScriptManager* self, PyObject *args);
		static PyObject* updateGui(VRScriptManager* self);
		static PyObject* render(VRScriptManager* self);
		static PyObject* getRoot(VRScriptManager* self);
		static PyObject* printOSG(VRScriptManager* self);
		static PyObject* getNavigator(VRScriptManager* self);
		static PyObject* getSetup(VRScriptManager* self);
		static PyObject* loadScene(VRScriptManager* self, PyObject *args);
		static PyObject* startThread(VRScriptManager* self, PyObject *args);
		static PyObject* joinThread(VRScriptManager* self, PyObject *args);
};

OSG_END_NAMESPACE

#endif // VRSCRIPTMANAGER_H_INCLUDED
