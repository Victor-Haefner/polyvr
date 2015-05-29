#ifndef VRSCRIPT_H_INCLUDED
#define VRSCRIPT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <string>
#include <map>
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRSignal.h"
#include "../networking/VRSocket.h"
#include "core/utils/VRName.h"

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
        };

        struct trig : public VRName {
            string trigger = "none";
            string dev;
            int key = 0;
            string state = "Pressed";
            string param;
            VRSignal* sig = 0;
            VRSocket* soc = 0;
            arg* a = 0;

            trig();
        };

        struct Search {
            int N = 0;
            string search;
            map<int, vector<int> > result;
        };

    private:
        string core;
        string head;
        string type = "Python";
        string mobile;
        PyObject* fkt = 0;
        PyObject* pargs = 0;
        map<string, arg*> args;
        map<string, arg*>::iterator a_itr;
        map<string, trig*> trigs;
        map<string, trig*>::iterator t_itr;
        bool active = true;
        float execution_time = -1;
        Search search;

        PyObject* getPyObj(arg* a);

        VRFunction<int>* cbfkt_sys;
        VRFunction<VRDevice*>* cbfkt_dev;
        VRFunction<string>* cbfkt_soc;

        void update();

    public:
        VRScript(string name);
        ~VRScript();

        void clean();

        void setName(string n);
        void setFunction(PyObject* _fkt);
        void setCore(string _script);
        void setType(string type);
        void setHTMLHost(string mobile);

        string getHead();
        string getCore();
        string getScript();
        string getType();
        string getUri();
        string getMobile();
        int getHeadSize();

        void enable(bool b);
        bool enabled();

        Search find(string s);
        Search getSearch();

        float getExecutionTime();

        arg* addArgument();
        void remArgument(string name);
        map<string, arg*> getArguments();

        void changeArgName(string name, string _new);
        void changeArgValue(string name, string _new);
        void changeArgType(string name, string _new);

        map<string, trig*> getTriggers();
        void addTrigger();
        void remTrigger(string name);
        void changeTrigger(string name, string trigger);
        void changeTrigDev(string name, string dev);
        void changeTrigParams(string name, string params);
        void changeTrigKey(string name, int key);
        void changeTrigState(string name, string state);

        void execute();
        void execute_dev(VRDevice* dev);
        void execute_soc(string);

        string getTriggerParams();
        string getTrigger();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);

};

OSG_END_NAMESPACE;

#endif // VRSCRIPT_H_INCLUDED
