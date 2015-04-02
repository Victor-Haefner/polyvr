#ifndef VRMILLINGMACHINE_H_INCLUDED
#define VRMILLINGMACHINE_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTransform;
class VRSocket;

class VRMillingMachine {
    private:
        int axis = 3;
        vector<VRTransform*> geos;

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

        void connect(string s);
        void disconnect();

        void setSpeed(Vec3f v);
        void setSpeed(float s);
        void setGeometry(vector<VRTransform*> geos);

        void setPosition(Vec3f p);
        Vec3f getPosition();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRMILLINGMACHINE_H_INCLUDED
