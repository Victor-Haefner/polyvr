#ifndef VIVE_H_INCLUDED
#define VIVE_H_INCLUDED

#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>

#include <openvr.h>
#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;

class Vive {
    private:
        bool ready = false;
        vr::IVRSystem* HMD = 0;
        vr::TrackedDevicePose_t poses[ vr::k_unMaxTrackedDeviceCount ];

        Matrix convMat( const vr::HmdMatrix34_t &m );
        void processEvent();
        void processController();
        void updatePoses();

    public:
        Vive();
        ~Vive();

        void update();

        void setActive(bool b);
        bool getActive();
};

OSG_END_NAMESPACE;

#endif // VIVE_H_INCLUDED
