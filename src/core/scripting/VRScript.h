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
#include "VRScriptFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScript : public VRName {
    public:
        struct arg : public VRName {
            string type = "NoneType";
            string val = "None";
            void* ptr = 0;
            PyObject* pyo = NULL;
            bool trig = false;

            arg(string nspace, string name = "arg");
            virtual ~arg();
        };

        struct trig : public VRName {
            string trigger = "none";
            string dev;
            int key = 0;
            string state = "Pressed";
            string param;
            VRSignalPtr sig = 0;
            VRSocket* soc = 0;
            arg* a = 0;

            trig();
            virtual ~trig();
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

    private:
        string core;
        string head;
        string type = "Python";
        string server = "server1";
        string group = "no group";
        PyObject* fkt = 0;
        PyObject* pargs = 0;
        arg* devArg = 0;
        arg* socArg = 0;
        list<arg*> args;
        list<trig*> trigs;
        bool active = true;
        float execution_time = -1;
        Search search;

        PyObject* getPyObj(arg* a);

        shared_ptr<VRFunction<int> > cbfkt_sys;
        VRFunction<VRDeviceWeakPtr>* cbfkt_dev;
        VRFunction<string>* cbfkt_soc;

        arg* getArg(string name);
        trig* getTrig(string name);
        void on_err_link_clicked(errLink link, string s);
        void pyTraceToConsole();
        void update();

    public:
        VRScript(string name);
        virtual ~VRScript();

        static VRScriptPtr create(string name = "Script");

        void clean();

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

        void enable(bool b);
        bool enabled();

        Search find(string s);
        Search getSearch();

        float getExecutionTime();

        arg* addArgument();
        void remArgument(string name);
        list<arg*> getArguments(bool withInternals = false);

        void changeArgName(string name, string _new);
        void changeArgValue(string name, string _new);
        void changeArgType(string name, string _new);

        list<trig*> getTriggers();
        void addTrigger();
        void remTrigger(string name);
        void changeTrigger(string name, string trigger);
        void changeTrigDev(string name, string dev);
        void changeTrigParams(string name, string params);
        void changeTrigKey(string name, int key);
        void changeTrigState(string name, string state);

        void compile( PyObject* pGlobal, PyObject* pModVR );
        void execute();
        void execute_dev(VRDeviceWeakPtr dev);
        void execute_soc(string);

        string getTriggerParams();
        string getTrigger();

        void setGroup(string g);
        string getGroup();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VRSCRIPT_H_INCLUDED
