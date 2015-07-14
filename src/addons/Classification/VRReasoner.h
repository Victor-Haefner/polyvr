#ifndef VRREASONER_H_INCLUDED
#define VRREASONER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>

using namespace std;


#include "VROntology.h"
//class VROntology;
//class VREntity;

class VRReasoner {
    public:
        struct Result {
            vector<VREntity*> instances;
        };

        static vector<string> split(string s, string d);
        static vector<string> split(string s, char d);
        static bool startswith(string s, string subs);

    private:
        VRReasoner();

    public:
        static VRReasoner* get();
        vector<VRReasoner::Result> process(string query, VROntology* onto);
};


#endif // VRREASONER_H_INCLUDED
