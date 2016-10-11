#ifndef VRSCRIPTMANAGER_H_INCLUDED
#define VRSCRIPTMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>
#include "core/setup/devices/VRSignal.h"
#include "core/utils/VRStorage.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/scripting/VRScriptFwd.h"
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
        map<string, VRScriptPtr> scripts;
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

        VRScriptPtr newScript(string name, string core);
        void addScript(VRScriptPtr script);
        void remScript(string name);

        void disableAllScripts();

        void updateScript(string name, string core, bool compile = true);
        VRScriptPtr changeScriptName(string name, string new_name);

        void triggerScript(string name);

        VRScriptPtr getScript(string name);
        map<string, VRScriptPtr> getScripts();

        vector<VRScriptPtr> searchScript(string s, VRScriptPtr sc = 0);

        vector<string> getPyVRModules();
        vector<string> getPyVRTypes(string mod);
        vector<string> getPyVRMethods(string mod, string type);
        string getPyVRDescription(string mod, string type);
        string getPyVRMethodDoc(string mod, string type, string method);
};

OSG_END_NAMESPACE

#endif // VRSCRIPTMANAGER_H_INCLUDED
