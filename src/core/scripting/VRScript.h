#ifndef VRSCRIPT_H_INCLUDED
#define VRSCRIPT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <string>
#include <map>
#include <list>
#include "core/utils/VRFunctionFwd.h"
#include "core/setup/devices/VRSignal.h"
#include "../networking/VRSocket.h"
#include "core/utils/VRName.h"
#include "core/utils/VRGlobals.h"
#include "VRScriptFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScript : public std::enable_shared_from_this<VRScript>, public VRName {
    public:
        struct arg : public VRName {
            string type = "NoneType";
            string val = "None";
            void* ptr = 0;
            PyObject* pyo = NULL;
            bool trig = false;

            arg(string nspace, string name = "arg");
            virtual ~arg();

            static shared_ptr<arg> create(string nspace, string name = "arg");
        };

        struct trig : public VRName {
            string trigger = "none";
            string dev;
            int key = 0;
            string state = "Pressed";
            string param;
            VRSignalPtr sig = 0;
            VRSocketPtr soc = 0;
            shared_ptr<arg> a = 0;

            trig();
            virtual ~trig();

            static shared_ptr<trig> create();
        };

        struct Search {
            int N = 0;
            string search;
            map<int, vector<int> > result;
        };

        struct errLink {
            string filename;
            int line;
            int column;
            errLink(string f, int l, int c);
        };

        typedef shared_ptr<arg> argPtr;
        typedef shared_ptr<trig> trigPtr;

    private:
        string core = "\tpass";
        string head;
        string type = "Python";
        string server = "server1";
        string group = "no group";
        PyObject* fkt = 0;
        PyObject* pargs = 0;
        argPtr devArg = 0;
        argPtr socArg = 0;
        list<argPtr> args;
        list<trigPtr> trigs;
        bool active = true;
        float execution_time = -1;
        Search search;
        static VRGlobals::Int loadingFrame;
        bool isInitScript = false;

        PyObject* getPyObj(argPtr a);

        VRUpdateCbPtr cbfkt_sys;
        VRDeviceCbPtr cbfkt_dev;
        VRMessageCbPtr cbfkt_soc;

        argPtr getArg(string name);
        trigPtr getTrig(string name);
        void on_err_link_clicked(errLink link, string s);
        void pyErrPrint(string channel);
        void printSyntaxError(PyObject* exception, PyObject* value, PyObject* tb);
        void update();

    public:
        VRScript(string name);
        virtual ~VRScript();

        static VRScriptPtr create(string name = "Script");
        VRScriptPtr ptr();

        void clean();
        void updateDeviceTrigger();

        void setName(string n);
        void setFunction(PyObject* _fkt);
        void setCore(string _script);
        void setType(string type);
        void setHTMLHost(string server);

        string getHead();
        string getCore();
        string getScript();
        string getType();
        string getUri();
        string getServer();
        int getHeadSize();
        PyObject* getFunction();

        void enable(bool b);
        bool enabled();

        Search find(string s);
        Search getSearch();

        float getExecutionTime();

        argPtr addArgument();
        void remArgument(string name);
        list<argPtr> getArguments();
        void setArguments(vector<string> vals);

        void changeArgName(string name, string _new);
        void changeArgValue(string name, string _new);
        void changeArgType(string name, string _new);

        list<trigPtr> getTriggers();
        trigPtr addTrigger();
        void remTrigger(string name);
        void changeTrigger(string name, string trigger);
        void changeTrigDev(string name, string dev);
        void changeTrigParams(string name, string params);
        void changeTrigKey(string name, int key);
        void changeTrigState(string name, string state);
        bool hasTrigger(string type);

        void compile( PyObject* pGlobal, PyObject* pModVR );
        void execute();
        void execute_dev(VRDeviceWeakPtr dev);
        void execute_soc(string);
        void queueExecution();

        string getTriggerParams();
        string getTrigger();

        void setGroup(string g);
        string getGroup();

        void save(XMLElementPtr e);
        void load(XMLElementPtr e);
};

OSG_END_NAMESPACE;

#endif // VRSCRIPT_H_INCLUDED
