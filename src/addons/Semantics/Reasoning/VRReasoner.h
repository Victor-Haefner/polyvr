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
        static string pre;
        static string redBeg;
        static string greenBeg;
        static string blueBeg;
        static string yellowBeg;
        static string colEnd;

        enum COLOR {
            BLUE,
            RED,
            GREEN,
            YELLOW
        };

        static bool verbGui;
        static bool verbConsole;
        static void print(const string& s);
        static void print(const string& s, COLOR c);

        static vector<string> split(string s, string d);
        static vector<string> split(string s, char d);
        static bool startswith(string s, string subs);

    private:
        map<string, bool> options;

        VRReasoner();

        bool evaluate(VRStatementPtr s, VRSemanticContextPtr c);
        bool apply(VRStatementPtr s, Query query, VRSemanticContextPtr c);
        bool builtin(VRStatementPtr s, VRSemanticContextPtr c);
        bool is(VRStatementPtr s, VRSemanticContextPtr c);
        bool set(VRStatementPtr s, VRSemanticContextPtr c);
        bool has(VRStatementPtr s, VRSemanticContextPtr c);
        bool findRule(VRStatementPtr s, VRSemanticContextPtr c);
        bool processQuery(Query& query, VRSemanticContextPtr c);

    public:
        static VRReasonerPtr create();

        void setOption(string option, bool b);
        vector<VREntityPtr> process(string query, VROntologyPtr onto);
        void setVerbose(bool gui, bool console);
};

OSG_END_NAMESPACE;

#endif // VRREASONER_H_INCLUDED
