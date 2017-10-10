#pragma once

#include "core/setup/devices/VRDevice.h"
#include "core/networking/VRWebSocket.h"

OSG_BEGIN_NAMESPACE ;

class VRLeapFrame : public std::enable_shared_from_this<VRLeapFrame> {

public:

    struct Hand {
        Hand() : pose(3), joints(5), bases(5), extended(5), directions(5) { }

        vector<Vec3d> pose; // pos, dir, normal
        vector<vector<Vec3d>> joints; // 0 is thumb; 4 is pinky
        vector<vector<vector<Vec3d>>> bases; // 3 basis vectors for each bone, in index order, wrist to tip
        vector<bool> extended; // True, if the finger is a pointing, or extended, posture
        vector<Vec3d> directions; // direction is expressed as a unit vector pointing in the same direction as the tip
        float pinchStrength;
        float grabStrength;
        float confidence;

        std::shared_ptr<Hand> clone();

        void transform(Matrix4d transformation);
    };

public:

    static VRLeapFramePtr create();

    std::shared_ptr<Hand> getLeftHand();

    std::shared_ptr<Hand> getRightHand();

    void setLeftHand(std::shared_ptr<Hand> hand);

    void setRightHand(std::shared_ptr<Hand> hand);

private:

    VRLeapFrame() { }

    std::shared_ptr<Hand> rightHand{nullptr};
    std::shared_ptr<Hand> leftHand{nullptr};

};

typedef std::shared_ptr<VRLeapFrame::Hand> HandPtr;

class VRLeap : public VRDevice {

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

private:

    bool resetConnection();

    void newFrame(Json::Value json);

    vector<std::function<void(VRLeapFramePtr)>> frameCallbacks;

    string host;
    int port;
    VRWebSocket webSocket;
    bool transformed{false};
    Matrix4d transformation;

};

OSG_END_NAMESPACE;
