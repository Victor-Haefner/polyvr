#ifndef VRRATE_H_INCLUDED
#define VRRATE_H_INCLUDED

#include <OpenSG/OSGStatElemDesc.h>
#include <OpenSG/OSGStatIntElem.h>

using namespace std;

/**
    Call getRate() every frame in an update function, it also returns the framerate.
    Use setPrint(true) to enable output every second to console.
*/

class VRRate {
    typedef OSG::StatElemDesc<OSG::StatIntElem> StatRate;

    public:
        static StatRate statFPStime;
        static StatRate statPhysFPStime;

    private:
        int count, current;
        int time_0, time_1;
        bool print;

        VRRate();

        void operator= (VRRate v);

    public:
        static VRRate* get();

        void setPrint(bool b);

        int getRate();
};


#endif // VRRATE_H_INCLUDED
