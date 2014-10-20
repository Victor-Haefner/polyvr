#ifndef ART_H_INCLUDED
#define ART_H_INCLUDED

#include "core/utils/VRName.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <map>

class DTrack;
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFlystick;
class VRTransform;

struct ART_device : public VRName {
    VRTransform* ent;
    VRFlystick* dev;
    Vec3f offset;
    float scale;
    int ID;
    int type;

    ART_device();
};

class ART {
    private:
        bool active;
        int port;
        Vec3f offset;

        DTrack* dtrack;
        map<string, ART_device*> devices;
        map<string, ART_device*>::iterator itr;

        //hohlt die Orientierung des getrackten Objektes
        template<typename type>
        void getRotation(type t, ART_device* dev, Matrix& m);

        //hohlt die Position des getrackten Objektes
        template<typename type>
        void getPosition(type t, ART_device* dev, Matrix& m);

        void checkIncomming();

        //update thread
        void update();

    public:
        ART();
        ~ART();

        ART_device* addARTDevice(VRTransform* trans);
        ART_device* addARTDevice(VRFlystick* dev = 0);
        vector<string> getARTDevices();
        ART_device* getARTDevice(string s);

        void setARTActive(bool b);
        bool getARTActive();

        void setARTPort(int port);
        int getARTPort();

        void setARTOffset(Vec3f o);
        Vec3f getARTOffset();

        VRFunction<int>* getARTUpdateFkt();

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE

#endif // ART_H_INCLUDED


//KONZEPT
//ART_tracking -> WRAPPER [ vector { Matrix + ART_ID + ART_type } -> vector { Matrix + ID } ] -> SCENEGRAPH
