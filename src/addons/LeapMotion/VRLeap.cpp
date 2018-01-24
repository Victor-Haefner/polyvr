#include "VRLeap.h"
#include "core/objects/VRTransform.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"

OSG_BEGIN_NAMESPACE ;

VRLeap::VRLeap() : VRDevice("leap") {

    auto cb = [&](Json::Value msg) {
        newFrame(msg);
    };

    webSocket.registerJsonCallback(cb);

    store("host", &host);
    store("port", &port);
    store("serial", &serial);
    store("transformation", &transformation);

    reconnect();
}

VRLeapPtr VRLeap::create() {
    auto d = VRLeapPtr(new VRLeap());
    return d;
}

string VRLeap::getHost() {
    return host;
}

void VRLeap::setHost(string newHost) {
    if (newHost == host) return;
    host = newHost;
}

int VRLeap::getPort() {
    return port;
}

void VRLeap::setPort(int newPort) {
    if (newPort == port) return;
    port = newPort;
}

string VRLeap::getAddress() { return host + ":" + to_string(port); }
void VRLeap::setAddress(string a) { host = splitString(a, ':')[0]; port = toInt(splitString(a, ':')[1]); }

void VRLeap::registerFrameCallback(function<void(VRLeapFramePtr)> func) {
    frameCallbacks.push_back(func);
}

void VRLeap::clearFrameCallbacks() { frameCallbacks.clear(); }

void VRLeap::newFrame(Json::Value json) {

    // json format: https://developer.leapmotion.com/documentation/v2/cpp/supplements/Leap_JSON.html?proglang=cpp

    if (serial.empty()) {
        if (json.isMember("event")) {
            serial = json["event"]["state"]["id"].asString();
            return;
        }
    }

    VRLeapFramePtr frame = VRLeapFrame::create();

    for (uint i = 0; i < json["hands"].size(); ++i) { // Get the hands
        auto newHand = json["hands"][i];

        auto hand = make_shared<VRLeapFrame::Hand>();
        // Setup new hand
        auto pos = newHand["palmPosition"];
        hand->pose.setPos( Vec3d(pos[0].asFloat(), pos[1].asFloat(), pos[2].asFloat())* 0.001f );
        auto dir = newHand["direction"];
        hand->pose.setDir( Vec3d(dir[0].asFloat(), dir[1].asFloat(), dir[2].asFloat()) );
        auto normal = newHand["palmNormal"];
        hand->pose.setUp( Vec3d(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat()) );

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
                hand->joints[type].push_back( Vec3d(jnt[0].asFloat(), jnt[1].asFloat(), jnt[2].asFloat()) * 0.001f);
            }

            // Setup joint poses
            Json::Value bases = pointable["bases"];

            for (auto& bases_ : bases) {
                Pose current;
                current.setPos( Vec3d(bases_[0][0].asFloat(), bases_[0][1].asFloat(), bases_[0][2].asFloat()) );
                current.setDir( Vec3d(bases_[1][0].asFloat(), bases_[1][1].asFloat(), bases_[1][2].asFloat()) );
                current.setUp ( Vec3d(bases_[2][0].asFloat(), bases_[2][1].asFloat(), bases_[2][2].asFloat()) );
                hand->bases[type].push_back(current);
            }

            // Setup extension and direction
            hand->extended[type] = pointable["extended"].asBool();
            auto direction = pointable["direction"];
            hand->directions[type] = Vec3d(direction[0].asFloat(), direction[1].asFloat(), direction[2].asFloat());
        }

        if (transformed) {
            hand->transform(transformation);
        }

        if (newHand["type"].asString() == "left") frame->setLeftHand(hand);
        else frame->setRightHand(hand);
    }

    for (uint i = 0; i < json["pointables"].size(); ++i) { // Get the tools
        auto pointable = json["pointables"][i];

        if (!pointable["tool"].asBool()) continue;

        auto pen = make_shared<VRLeapFrame::Pen>();

        auto tipPosition = pointable["stabilizedTipPosition"];
        pen->tipPosition = Vec3d(tipPosition[0].asFloat(), tipPosition[1].asFloat(), tipPosition[2].asFloat()) * 0.001;

        auto direction = pointable["direction"];
        pen->direction = Vec3d(direction[0].asFloat(), direction[1].asFloat(), direction[2].asFloat());

        pen->length = pointable["length"].asFloat();
        pen->width = pointable["width"].asFloat();

        if (transformed && !calibrate) { pen->transform(transformation); }

        frame->insertPen(pen);
    }

    if (calibrate) {
        Pose pose;
        vector<PenPtr> pens = frame->getPens();
        if (pens.size() == 2) {
            pose = computeCalibPose(pens);
            setPose(pose);
        }
    }

    for (auto& cb : frameCallbacks) cb(frame);
}

Pose VRLeap::computeCalibPose(vector<PenPtr>& pens) {
    Pose result;
    if (pens.size() != 2) { return result; }

    Vec3d pos0 = pens[0]->tipPosition;
    Vec3d pos1 = pens[1]->tipPosition;
    Vec3d dir0 = pens[0]->direction;
    Vec3d dir1 = pens[1]->direction;

    Vec3d position = ((pos0 - 0.15 * dir0) + (pos1 - 0.15 * dir1)) / 2.0;
    Vec3d direction = pos1 - pos0;
    direction.normalize();
    Vec3d normal = (position - pos0).cross(direction);
    normal.normalize();

    result.set(position, direction, normal);

    return result;
}

string VRLeap::getConnectionStatus() {
    return connectionStatus;
}

string VRLeap::getSerial() {
    return serial;
}

bool VRLeap::reconnect() {
    bool result = true;

    string url = "ws://" + host + ":" + to_string(port) + "/v6.json";
    cout << "Connecting to Leap " + getName() + " at " << url << endl;

    result = webSocket.open(url);

    if (result) {
        result = webSocket.sendMessage("{\"background\": true}");
        connectionStatus = "connected to " + getAddress();
    } else {
        connectionStatus = "connection error (daemon running?)";
    }

    return result;
}

void VRLeap::setPose(Pose pose) {
    transformation = pose.asMatrix();
    transformed = true;
}

void VRLeap::setPose(Vec3d pos, Vec3d dir, Vec3d up) {
    setPose(Pose(pos, dir, up));
}

Matrix4d VRLeap::getTransformation() {
    return transformation;
}

void VRLeap::startCalibration() {
    calibrate = true;
}

void VRLeap::stopCalibration() {
    calibrate = false;
}


OSG_END_NAMESPACE;
