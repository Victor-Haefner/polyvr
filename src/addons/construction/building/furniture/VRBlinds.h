#ifndef VRBLINDS_H_INCLUDED
#define VRBLINDS_H_INCLUDED

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
        VRDevCb* toggleCallback;
        VRSignal* sig;
        VRScene* scene;
        State state;
        string sound;
        string param;


        VRGeometry* window;
        VRAnimPtr fkt;

        VRGeometry* blend_geo;
        vector<Vec3f> bl_pos_open;
        vector<Vec3f> bl_pos_closed;

    protected:
        VRObject* copy(vector<VRObject*> children);

    public:
        VRBlinds(string name, VRGeometry* _window, VRScene* _scene);

        void setSound(string s);

        bool isClose();
        bool isOpen();

        void open();

        void close();

        void toggle(VRDevice* dev);

        void create();

        void interpolate(float t);

        VRDevCb* getCallback();
};

OSG_END_NAMESPACE;

#endif // VRBLINDS_H_INCLUDED
