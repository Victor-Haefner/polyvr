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
    private:
        int count = 0;
        int current = 60;
        int time_0 = 0;
        int time_1 = 0;
        bool print = false;

    public:
        VRRate();
        ~VRRate();
        void setPrint(bool b);
        int getRate();
};


#endif // VRRATE_H_INCLUDED
