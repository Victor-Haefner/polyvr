#pragma once

#include "core/setup/devices/VRDevice.h"
#include "core/networking/VRWebSocket.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;

class VRLeapFrame : public std::enable_shared_from_this<VRLeapFrame> {
    public:
        ptrFwd(Hand);
        ptrFwd(Pen);

        struct Pen {
            Pose pose; // pos, dir, normal
            vector<vector<Vec3d>> joints; // joint positions of each finger, 0 is thumb -> 4 is pinky
            vector<vector<Pose>> bases; // 3 basis vectors for each bone, in index order, wrist to tip
            vector<bool> extended; // True, if the finger is a pointing, or extended, posture
            vector<Vec3d> directions; // direction is expressed as a unit vector pointing in the same direction as the tip
            float pinchStrength;
            float grabStrength;
            float confidence;

            Pen() : joints(5), bases(5), extended(5), directions(5) { }
            PenPtr clone();

            void transform(Matrix4d transformation);
        };

        struct Hand {
            Pose pose; // pos, dir, normal
            vector<vector<Vec3d>> joints; // joint positions of each finger, 0 is thumb -> 4 is pinky
            vector<vector<Pose>> bases; // 3 basis vectors for each bone, in index order, wrist to tip
            vector<bool> extended; // True, if the finger is a pointing, or extended, posture
            vector<Vec3d> directions; // direction is expressed as a unit vector pointing in the same direction as the tip
            float pinchStrength;
            float grabStrength;
            float confidence;

            Hand() : joints(5), bases(5), extended(5), directions(5) { }
            HandPtr clone();

            void transform(Matrix4d transformation);
        };

    private:
        VRLeapFrame() {}

        HandPtr rightHand;
        HandPtr leftHand;

    public:
        static VRLeapFramePtr create();

        HandPtr getLeftHand();
        HandPtr getRightHand();

        void setLeftHand(HandPtr hand);
        void setRightHand(HandPtr hand);
};

typedef VRLeapFrame::HandPtr HandPtr;
typedef VRLeapFrame::PenPtr PenPtr;

class VRLeap : public VRDevice {
    private:
        vector<std::function<void(VRLeapFramePtr)>> frameCallbacks;

        string host;
        int port = 6437;
        VRWebSocket webSocket;
        bool transformed{false};
        Matrix4d transformation;

        bool resetConnection();
        void newFrame(Json::Value json);

    public:
        VRLeap();

        ~VRLeap() {
            cout << "~VRLeap" << endl;
            frameCallbacks.clear();
            webSocket.close();
        }

        static VRLeapPtr create();

        bool open(string host = "localhost", int port = 6437);

        string getHost();
        void setHost(string newHost);
        int getPort();
        void setPort(int newPort);

        void registerFrameCallback(std::function<void(VRLeapFramePtr)> func);
        void setPose(Vec3d pos, Vec3d dir, Vec3d up);
};

OSG_END_NAMESPACE;
