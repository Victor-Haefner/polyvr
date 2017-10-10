#include "VRLeap.h"
#include "core/objects/VRTransform.h"
#include <OpenSG/OSGQuaternion.h>

OSG_BEGIN_NAMESPACE ;


VRLeap::VRLeap() : VRDevice("leap") {

    auto cb = [&](Json::Value msg) {
        newFrame(msg);
    };

    webSocket.registerJsonCallback(cb);
}

VRLeapPtr VRLeap::create() {
    auto d = VRLeapPtr(new VRLeap());
    return d;
}

bool VRLeap::open(string host, int port) {
    this->host = host;
    this->port = port;
    return resetConnection();
}

string VRLeap::getHost() {
    return host;
}

void VRLeap::setHost(string newHost) {
    if (newHost == host) return;
    host = newHost;
    resetConnection();
}

int VRLeap::getPort() {
    return port;
}

void VRLeap::setPort(int newPort) {
    if (newPort == port) return;
    port = newPort;
    resetConnection();
}

void VRLeap::registerFrameCallback(function<void(VRLeapFramePtr)> func) {
    frameCallbacks.push_back(func);
}

void VRLeap::newFrame(Json::Value json) {

    VRLeapFramePtr frame = VRLeapFrame::create();

    for (uint i = 0; i < json["hands"].size(); ++i) { // Get the hands
        auto newHand = json["hands"][i];

        auto hand = make_shared<VRLeapFrame::Hand>();
        // Setup new hand
        auto pos = newHand["palmPosition"];
        hand->pose[0] = Vec3d(pos[0].asFloat() * 0.001f, pos[1].asFloat() * 0.001f, pos[2].asFloat() * 0.001f);
        auto dir = newHand["direction"];
        hand->pose[1] = Vec3d(dir[0].asFloat(), dir[1].asFloat(), dir[2].asFloat());
        auto normal = newHand["palmNormal"];
        hand->pose[2] = Vec3d(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat());

        hand->pinchStrength = newHand["pinchStrength"].asFloat();
        hand->grabStrength = newHand["grabStrength"].asFloat();
        hand->confidence = newHand["confidence"].asFloat();

        int id = newHand["id"].asInt();

        for (uint j = 0; j < json["pointables"].size(); ++j) { // Get the corresponding fingers
            auto pointable = json["pointables"][j];

            if (pointable["handId"].asInt() != id) continue;

            int type = pointable["type"].asInt();

            // Setup joint positions
            vector<Json::Value> joints = {pointable["carpPosition"],
                                          pointable["mcpPosition"],
                                          pointable["pipPosition"],
                                          pointable["dipPosition"],
                                          pointable["btipPosition"]};

            for (auto& jnt : joints) {
                hand->joints[type].push_back(
                        Vec3d(jnt[0].asFloat() * 0.001f, jnt[1].asFloat() * 0.001f, jnt[2].asFloat() * 0.001f));
            }

            // Setup joint poses
            Json::Value bases = pointable["bases"];

            for (auto& bases_ : bases) {
                vector<Vec3d> current(3);
                for (int b = 0; b < 3; ++b) {
                    current[b] = Vec3d(bases_[b][0].asFloat(), bases_[b][1].asFloat(), bases_[b][2].asFloat());
                }
                hand->bases[type].push_back(current);
            }

            // Setup extension and direction
            hand->extended[type] = pointable["extended"].asBool();
            auto direction = pointable["direction"];
            hand->directions[type] = Vec3d(direction[0].asFloat(), direction[1].asFloat(), direction[2].asFloat());
        }

        if (transformed || true) {
            hand->transform(transformation);
        }

        if (newHand["type"].asString() == "left") frame->setLeftHand(hand);
        else frame->setRightHand(hand);
    }

    for (auto& cb : frameCallbacks) {
        cb(frame);
    }
}

bool VRLeap::resetConnection() {

    bool result = true;

    string url = "ws://" + host + ":" + to_string(port) + "/v6.json";
    cout << "Connecting to Leap at " << url << endl;
    result &= webSocket.open(url);

    if (result) {
        result &= webSocket.sendMessage("{\"background\": true}");
    }

    return result;
}

void VRLeap::setPose(Vec3d pos, Vec3d dir, Vec3d up) {

    MatrixLookDir(transformation, pos, dir, up);

    transformed = true;
}


// ########### LeapFrame ###########

VRLeapFramePtr VRLeapFrame::create() {
    auto d = VRLeapFramePtr(new VRLeapFrame());
    return d;
}

HandPtr VRLeapFrame::getLeftHand() { return leftHand; }

HandPtr VRLeapFrame::getRightHand() { return rightHand; }

void VRLeapFrame::setLeftHand(std::shared_ptr<Hand> hand) { leftHand = hand; }

void VRLeapFrame::setRightHand(std::shared_ptr<Hand> hand) { rightHand = hand; }

HandPtr VRLeapFrame::Hand::clone() {
    auto copy = make_shared<VRLeapFrame::Hand>();

    copy->pose = pose;
    copy->joints = joints;
    copy->bases = bases;
    copy->extended = extended;
    copy->directions = directions;
    copy->pinchStrength = pinchStrength;
    copy->grabStrength = grabStrength;
    copy->confidence = confidence;

    return copy;
}

void VRLeapFrame::Hand::transform(Matrix4d transformation) {

    cerr << "VRLeapFrame::Hand::transform not yet implemented." << endl;

    for (auto& p : pose) {
        //TODO: transform poses
        pose[0] = transformation * pose[0];
        cout << p << endl;
    }
    cout << endl;

    for (int i = 0; i < 5; ++i) {
        for (auto& j : joints[i]) {
            // TODO: transform joints
        }
        for (auto& b: bases[i]) {
            for (auto& p : b) {
                // TODO: transform bases
            }
        }
        // TODO: transform directions
    }
}


OSG_END_NAMESPACE;
