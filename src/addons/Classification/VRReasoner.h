#ifndef VRREASONER_H_INCLUDED
#define VRREASONER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>

using namespace std;

class VROntology;

class VRReasoner {
    public:
        struct Result {
            VROntology* o = 0;
            string toString();
        };

    private:
        VRReasoner();

    public:
        static VRReasoner* get();
        Result process(string query);
};


#endif // VRREASONER_H_INCLUDED
