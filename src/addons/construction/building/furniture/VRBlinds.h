#ifndef VRBLINDS_H_INCLUDED
#define VRBLINDS_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRBlinds: public VRTransform {
    public:
        enum State { OPEN, CLOSE };

    private:
        VRDeviceCbPtr toggleCallback;
        VRSignalPtr sig;
        VRScene* scene;
        State state;
        string sound;
        string param;


        VRGeometryPtr window;
        VRAnimCbPtr fkt;

        VRGeometryPtr blend_geo;
        vector<Vec3f> bl_pos_open;
        vector<Vec3f> bl_pos_closed;

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        VRBlinds(string name, VRGeometryPtr _window, VRScene* _scene);
        static VRBlindsPtr create(string name, VRGeometryPtr _window, VRScene* _scene);
        VRBlindsPtr ptr();

        void setSound(string s);

        bool isClose();
        bool isOpen();

        void open();

        void close();

        void toggle(VRDeviceWeakPtr dev);

        void create();

        void interpolate(float t);

        VRDeviceCbPtr getCallback();
};

OSG_END_NAMESPACE;

#endif // VRBLINDS_H_INCLUDED
