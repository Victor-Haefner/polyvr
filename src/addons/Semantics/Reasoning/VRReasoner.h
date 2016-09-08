#ifndef VRREASONER_H_INCLUDED
#define VRREASONER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <memory>

using namespace std;

#include "VROntology.h"
#include "../VRSemanticsFwd.h"
#include "VRStatement.h"
#include "VRSemanticUtils.h"

OSG_BEGIN_NAMESPACE;

class VRReasoner {
    public:
        string pre = "  ?!?  ";
        string redBeg  = "\033[0;38;2;255;150;150m";
        string greenBeg = "\033[0;38;2;150;255;150m";
        string blueBeg = "\033[0;38;2;150;150;255m";
        string yellowBeg = "\033[0;38;2;255;255;150m";
        string colEnd = "\033[0m";

        bool verbGui = true;
        bool verbConsole = true;

        enum COLOR {
            BLUE,
            RED,
            GREEN,
            YELLOW
        };

        static vector<string> split(string s, string d);
        static vector<string> split(string s, char d);
        static bool startswith(string s, string subs);

    private:
        VRReasoner();

        bool evaluate(VRStatementPtr s, Context& c);
        bool apply(VRStatementPtr s, Context& c);
        bool is(VRStatementPtr s, Context& c);
        bool has(VRStatementPtr s, Context& c);
        bool findRule(VRStatementPtr s, Context& c);
        void print(const string& s);
        void print(const string& s, COLOR c);

    public:
        static VRReasonerPtr create();
        vector<VREntityPtr> process(string query, VROntologyPtr onto);
        void setVerbose(bool gui, bool console);
};

OSG_END_NAMESPACE;

#endif // VRREASONER_H_INCLUDED
