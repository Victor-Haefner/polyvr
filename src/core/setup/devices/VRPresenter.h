#ifndef VRPresenter_H_INCLUDED
#define VRPresenter_H_INCLUDED

#include "VRFlystick.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPresenter : public VRFlystick {
    private:
        string input;
        int fd;
        VRUpdateCbPtr updatePtr;

    public:
        VRPresenter();
        ~VRPresenter();

        static VRPresenterPtr create();
        VRPresenterPtr ptr();

        void setup(VRStorageContextPtr context);

        string getDevice();
        void setDevice(string dev);

        void updateDevice();
        void connectDevice();
        void disconnectDevice();
};

OSG_END_NAMESPACE;

#endif // VRPresenter_H_INCLUDED
