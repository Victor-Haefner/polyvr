#ifndef VRLEAP_H_INCLUDED
#define VRLEAP_H_INCLUDED

#include "core/setup/devices/VRDevice.h"
#include "core/networking/VRWebSocket.h"
#include "core/tools/VRToolsFwd.h"

#include <boost/thread/recursive_mutex.hpp>

#include "VRLeapFrame.h"


OSG_BEGIN_NAMESPACE;

struct VRLeapHistory {
    int N = 200;// history
    int K = 150; // mean
    int I = -1;
    vector< pair<float,Vec3d> > history;

    Vec3d add(Vec3d v, float f);
};

/* TODO

            VRLeapHistory posHistory;
            VRLeapHistory dirHistory;
            VRLeapHistory  upHistory;

    auto pos  = hand->posHistory.add( toVec3d(handData["palmPosition"]) * 0.001f, hand->confidence );
    auto dir  = hand->posHistory.add( toVec3d(handData["direction"]), hand->confidence );
    auto norm = hand->posHistory.add( toVec3d(handData["palmNormal"]), hand->confidence );

*/

class VRLeap : public VRDevice {
    private:
        //TODO: Debugging only
        int numPens;
        VRObjectPtr dndRoot;
        bool doDnD = false;
        VRDeviceCbPtr dndCb;

        VRAnalyticGeometryPtr ageo;

        vector<std::function<void(VRLeapFramePtr)>> frameCallbacks;

        string host{"localhost"};
        int port{6437};
        string connectionStatus{"not connected"};
        VRWebSocket webSocket;
        boost::recursive_mutex mutex;

        bool transformed{false};
        PosePtr transformation;
        bool calibrate{false};
        string serial;

        vector<HandPtr> hands;
        float dragThreshold = 0.05; // 0.5
        float dropThreshold = 0.03; // 0.3

        void newFrame(Json::Value json);
        void updateHandFromJson(Json::Value& handData, Json::Value& pointableData, HandPtr hand);
        void updateSceneData(vector<HandPtr> hands);
        VRTransformPtr getBeaconChild(int i);
        PosePtr computeCalibPose(vector<PenPtr>& pens);

        void leapDnD(VRDeviceWeakPtr dev);

    protected:
        void dragCB(VRTransformWeakPtr wcaster, VRObjectWeakPtr wtree, VRDeviceWeakPtr dev = VRDevicePtr(0)) override;

    public:
        VRLeap();

        ~VRLeap() {
            cout << "~VRLeap" << endl;
            frameCallbacks.clear();
            webSocket.close();
        }

        static VRLeapPtr create();
        VRLeapPtr ptr();

        void enableDnD(VRObjectPtr root);

        void setup();
        bool reconnect();

        string getHost();
        void setHost(string newHost);
        int getPort();
        void setPort(int newPort);
        string getConnectionStatus();
        string getSerial();

        string getAddress();
        void setAddress(string a);

        void registerFrameCallback(std::function<void(VRLeapFramePtr)> func);
        void clearFrameCallbacks();
        void setPose(PosePtr pose);
        PosePtr getPose();

        void startCalibration();
        void stopCalibration();
        void calibrateManual(Vec3d position, Vec3d direction, Vec3d normal);

        void clearSignals();

        float getDragThreshold();
        float getDropThreshold();
        void setDragThreshold(float t);
        void setDropThreshold(float t);

        float getPinchStrength(int hand);
        float getGrabStrength(int hand);
        float getConfidence(int hand);
        bool getIsPinching(int hand);
        vector<PosePtr> getHandPose();

        vector<VRObjectPtr> addHandsGeometry();
};

OSG_END_NAMESPACE;

#endif // VRLEAP_H_INCLUDED
