#ifndef VRREASONER_H_INCLUDED
#define VRREASONER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>

using namespace std;

class VROntology;

class VRReasoner {
    public:
        struct Result {
            VROntology* o = 0;
            string toString();
        };

        struct Job {
            string content;
            Job(string s);
        };

    private:
        vector<string> split(string s, string d);
        vector<string> split(string s, char d);
        bool startswith(string s, string subs);

        VRReasoner();

    public:
        static VRReasoner* get();
        Result process(string query, VROntology* onto);
};


#endif // VRREASONER_H_INCLUDED
