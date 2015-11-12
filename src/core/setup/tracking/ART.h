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
#include <boost/thread/mutex.hpp>

class DTrack;
template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSignal;
class VRFlystick;
class VRThread;

struct ART_device : public VRName {
    Matrix m;
    list<vector<int> > buttons;
    list<vector<float> > joysticks;

    VRTransformPtr ent = 0;
    VRFlystick* dev = 0;
    Vec3f offset;
    float scale = 1;
    int ID = 0;
    int type = 0;

    ART_device();
    ART_device(int ID, int type);

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
        Vec3f offset;
        string up;

        DTrack* dtrack = 0;
        map<int, ART_device*> devices;

        VRUpdatePtr updatePtr;
        VRSignalPtr on_new_device;

        template<typename dev>
        void getMatrix(dev t, ART_device* d);

        boost::mutex mutex;
        void scan(int type = -1, int N = 0);


        void updateT( weak_ptr<VRThread>  t); //update thread
        void updateL(); //update
        void applyEvents(); //main loop update
        void checkNewDevices(int type = -1, int N = 0); //update thread

    public:
        ART();
        ~ART();

        vector<int> getARTDevices();
        ART_device* getARTDevice(int dev);

        void setARTActive(bool b);
        bool getARTActive();

        void setARTPort(int port);
        int getARTPort();

        void setARTOffset(Vec3f o);
        Vec3f getARTOffset();

        void startTestStream();

        VRSignalPtr getSignal_on_new_art_device();
};

OSG_END_NAMESPACE

#endif // ART_H_INCLUDED


//KONZEPT
//ART_tracking -> WRAPPER [ vector { Matrix + ART_ID + ART_type } -> vector { Matrix + ID } ] -> SCENEGRAPH
