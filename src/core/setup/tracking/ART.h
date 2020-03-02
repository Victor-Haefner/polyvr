#ifndef ART_H_INCLUDED
#define ART_H_INCLUDED

#include "core/utils/VRName.h"
#include "core/utils/VRStorage.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <map>

class DTrack;

namespace boost { class recursive_mutex; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSignal;
class VRFlystick;
class VRThread;

struct ART_device : public VRName {
    Matrix4d m;
    vector<Matrix4d> fingers = vector<Matrix4d>(5);
    list<vector<int> > buttons;
    list<vector<float> > joysticks;

    VRTransformPtr ent = 0;
    vector<VRTransformPtr> fingerEnts;
    VRFlystickPtr dev = 0;
    Vec3d offset;
    float scale = 1;
    int ID = 0;
    int type = 0;

    ART_device();
    ART_device(int ID, int type);

    static ART_devicePtr create(int ID, int type);

    int key();
    static int key(int ID, int type);

    void init();
    void update();
};

class ART : public VRStorage {
    private:
        bool active = false;
        int port = 5000;
        int current_port = -1;
        Vec3i axis = Vec3i(0,1,2);
        Vec3d offset;
        string up;

        DTrack* dtrack = 0;
        map<int, ART_devicePtr> devices;

        VRUpdateCbPtr updatePtr;
        shared_ptr< VRFunction< weak_ptr<VRThread> > > threadFkt;
        VRSignalPtr on_new_device;

        template<typename dev> void getMatrix(dev t, Matrix4d& m, bool doOffset = true);
        template<typename dev> void getMatrix(dev t, ART_devicePtr d);

        boost::recursive_mutex* mutex = 0;
        void scan(int type = -1, int N = 0);

        void update_setup();

        void updateT( weak_ptr<VRThread>  t); //update thread
        void updateL(); //update
        void checkNewDevices(int type = -1, int N = 0); //update thread

    public:
        ART();
        ~ART();

        void applyEvents(); //main loop update

        vector<int> getARTDevices();
        ART_devicePtr getARTDevice(int dev);

        void setARTActive(bool b);
        void setARTPort(int port);
        void setARTOffset(Vec3d o);
        void setARTAxis(Vec3i a);
        bool getARTActive();
        int getARTPort();
        Vec3d getARTOffset();
        Vec3i getARTAxis();

        void startTestStream();

        VRSignalPtr getSignal_on_new_art_device();
};

OSG_END_NAMESPACE

#endif // ART_H_INCLUDED


//KONZEPT
//ART_tracking -> WRAPPER [ vector { Matrix4d + ART_ID + ART_type } -> vector { Matrix4d + ID } ] -> SCENEGRAPH
