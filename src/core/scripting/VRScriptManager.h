#ifndef VRSCRIPTMANAGER_H_INCLUDED
#define VRSCRIPTMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>
#include "core/setup/devices/VRSignal.h"
#include "core/utils/VRStorage.h"
#include "core/utils/VRDeviceFwd.h"
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
        map<int, VRThreadCb> pyThreads;
        map<string, map<string, PyTypeObject*> > modules;
        map<string, VRScript*> scripts;
        map<string, VRSignalPtr> triggers;
        PyThreadState* pyThreadState = 0;

        template<class T>
        void registerModule(string mod, PyObject* parent, PyTypeObject* base = 0, string mod_parent = "VR");

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

        vector<string> getPyVRModules();
        vector<string> getPyVRTypes(string mod);
        vector<string> getPyVRMethods(string mod, string type);
        string getPyVRDescription(string mod, string type);
        string getPyVRMethodDoc(string mod, string type, string method);

        // Python Methods
		static PyObject* exit(VRScriptManager* self);
		static PyObject* loadGeometry(VRScriptManager* self, PyObject *args, PyObject *kwargs);
		static PyObject* getLoadGeometryProgress(VRScriptManager* self);
		static PyObject* exportGeometry(VRScriptManager* self, PyObject *args);
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
		static PyObject* getSystemDirectory(VRScriptManager* self, PyObject *args);
		static PyObject* setPhysicsActive(VRScriptManager* self, PyObject *args);
};

OSG_END_NAMESPACE

#endif // VRSCRIPTMANAGER_H_INCLUDED
