#ifndef VRSIGNAL_H_INCLUDED
#define VRSIGNAL_H_INCLUDED

#include "core/utils/VRName.h"
#include <OpenSG/OSGConfig.h>
#include <vector>

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
typedef VRFunction<VRDevice*> VRDevCb;

class VRSignal : public VRName {
    private:
        vector<VRDevCb*> callbacks;
        VRDevice* dev;
        VRFunction<int>* trig_fkt;
        bool _doUpdate;

    public:
        void add(VRDevCb* fkt);
        void sub(VRDevCb* fkt);

        VRSignal(VRDevice* dev = 0);
        ~VRSignal();

        void trigger();

        void setUpdate(bool b);
        bool doUpdate();

        VRFunction<int>* getTriggerFkt();

        void clear();
};

OSG_END_NAMESPACE

#endif // VRSIGNAL_H_INCLUDED
