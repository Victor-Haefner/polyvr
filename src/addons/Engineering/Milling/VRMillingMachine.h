#ifndef VRMILLINGMACHINE_H_INCLUDED
#define VRMILLINGMACHINE_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSocket;

class VRMillingMachine {
    private:
        int axis = 3;
        vector<VRTransformPtr> geos;

        VRSocket* http = 0;

        string address;
        bool online = false;
        int state = 1; // CNC Status (z.B. Achsen aktiv)
        int mode = 1; // CNC Modus (z.B. JOG, Auto, ..)

        Vec3f pos;
        float speed = 4;

        string post(string cmd, string data);

    public:
        VRMillingMachine();
        static shared_ptr<VRMillingMachine> create();

        void connect(string s);
        void disconnect();
        bool connected();

        void setSpeed(Vec3f v);
        void setSpeed(float s);
        void setGeometry(vector<VRTransformPtr> geos);

        void setPosition(Vec3f p);
        Vec3f getPosition();

        int getState();
        int getMode();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRMILLINGMACHINE_H_INCLUDED
