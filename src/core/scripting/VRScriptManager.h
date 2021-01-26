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
class VRScriptTemplate;

class VRScriptManager : public VRStorage, public VRPyBase {
    protected:
        PyObject* pGlobal;
        PyObject* pLocal;
        PyObject* pModBase;
        PyObject* pModVR;
        map<int, VRThreadCbPtr> pyThreads;
        map<string, PyObject*> modules;
        map<string, map<string, PyTypeObject*> > moduleTypes;
        map<string, VRScriptPtr> scripts;
        map<string, VRSignalPtr> triggers;
        map<string, VRScriptTemplate> templates;
        map<string, vector<string>> importedScripts;
        PyThreadState* pyThreadState = 0;

        void test();
        void initPyModules();
        void initTemplates();
        void update();

    public:
        VRScriptManager();
        ~VRScriptManager();

        void allowScriptThreads();
        void blockScriptThreads();

        VRScriptPtr newScript(string name, string core);
        void addScript(VRScriptPtr script);
        void remScript(string name);

        void pauseScripts(bool b);

        void updateScript(string name, string core, bool compile = true);
        VRScriptPtr changeScriptName(string name, string new_name);

        void triggerScript(string name, vector<string> params = vector<string>());

        VRScriptPtr getScript(string name);
        map<string, VRScriptPtr> getScripts();

        vector<VRScriptPtr> searchScript(string s, VRScriptPtr sc = 0);

        vector<string> getPyVRModules();
        vector<string> getPyVRTypes(string mod);
        vector<string> getPyVRMethods(string mod, string type);
        string getPyVRDescription(string mod, string type);
        string getPyVRMethodDoc(string mod, string type, string method);

        map<string, vector<string>> getTemplates();
        string getTemplateCore(string t);
        void importTemplate(string t);

        PyObject* getPyModule(string name);
        PyObject* getGlobalModule();
        PyObject* getGlobalDict();

        void redirectPyOutput(string pyOutput, string console);

        PyObject* newModule(string name, PyMethodDef* methods, string doc);

        template<class T>
        void registerModule(string mod, PyObject* parent, PyTypeObject* base = 0, string mod_parent = "VR");

        void triggerOnLoad();
        void triggerOnImport();
};

OSG_END_NAMESPACE

#endif // VRSCRIPTMANAGER_H_INCLUDED
