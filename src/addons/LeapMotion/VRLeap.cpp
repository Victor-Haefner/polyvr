#include "VRLeap.h"
#include <core/objects/VRTransform.h>
#include <core/utils/toString.h>
#include <core/utils/VRStorage_template.h>
#include <core/setup/devices/VRSignal.h>
#include <core/objects/OSGObject.h>
#include <core/utils/VRGlobals.h>
#include <core/math/boundingbox.h>


OSG_BEGIN_NAMESPACE ;

VRLeap::VRLeap() : VRDevice("leap") {

    for (int i = 0; i < 9; ++i) { addBeacon(); }

    auto cb = [&](Json::Value msg) {
        newFrame(msg);
    };

    webSocket.registerJsonCallback(cb);

    store("host", &host);
    store("port", &port);
    //store("serial", &serial);
    store("transformation", &transformation);

    reconnect();
}

VRLeapPtr VRLeap::create() {
    auto d = VRLeapPtr(new VRLeap());
    d->initIntersect(d);
    d->clearSignals();
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


void VRLeap::updateHandFromJson(Json::Value& handData, Json::Value& pointableData, HandPtr hand) {

        auto pos = handData["palmPosition"];
        hand->pose.setPos( Vec3d(pos[0].asFloat(), pos[1].asFloat(), pos[2].asFloat())* 0.001f );
        auto dir = handData["direction"];
        hand->pose.setDir( Vec3d(dir[0].asFloat(), dir[1].asFloat(), dir[2].asFloat()) );
        auto normal = handData["palmNormal"];
        hand->pose.setUp( Vec3d(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat()) );

        hand->pinchStrength = handData["pinchStrength"].asFloat();
        hand->grabStrength = handData["grabStrength"].asFloat();
        hand->confidence = handData["confidence"].asFloat();

        int id = handData["id"].asInt();

        for (uint j = 0; j < pointableData.size(); ++j) { // Get the corresponding fingers
            auto pointable = pointableData[j];

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
                current.setDir ( Vec3d(bases_[1][0].asFloat(), bases_[1][1].asFloat(), bases_[1][2].asFloat()) );
                current.setUp ( Vec3d(bases_[2][0].asFloat(), bases_[2][1].asFloat(), bases_[2][2].asFloat()) );
                hand->bases[type].push_back(current);
            }

            // Setup extension and direction
            hand->extended[type] = pointable["extended"].asBool();
            auto direction = pointable["direction"];
            hand->directions[type] = Vec3d(direction[0].asFloat(), direction[1].asFloat(), direction[2].asFloat());
        }
}

void VRLeap::newFrame(Json::Value json) {

    // json format: https://developer.leapmotion.com/documentation/v2/cpp/supplements/Leap_JSON.html?proglang=cpp

    if (json.isMember("event")) { // second frame
        if (transformation.asMatrix() != Matrix4d::identity()) { transformed = true; }
        if (serial.empty()) {
            serial = json["event"]["state"]["id"].asString();
        }
        return;
    }

    VRLeapFramePtr frame = VRLeapFrame::create();
    vector<HandPtr> hands(2, nullptr);

    for (uint i = 0; i < json["hands"].size(); ++i) { // Get the hands

        auto newHand = json["hands"][i];
        auto hand = make_shared<VRLeapFrame::Hand>();

        // Setup new hand
        updateHandFromJson(newHand, json["pointables"], hand);

        if (transformed) {
            hand->transform(transformation);
        }

        bool left = (newHand["type"].asString() == "left");

        if (left)   hands[0] = hand;
        else        hands[1] = hand;
    }

    for (int i = 0; i < 2; ++i) {
        HandPtr hand = hands[i];

        int dnd_btn = i; // drag button 0 for left, 1 for right hand
        if (hand) {

            // set hands of frame
            if (i)  frame->setRightHand(hand);
            else    frame->setLeftHand(hand);

            // update beacons
            int b = i * 5; // left start at 0, right start at 5
            for (int j = 0; j < hand->directions.size(); ++j) {
                //PosePtr p = Pose::create(hand->joints[j][4], hand->bases[j].back().dir(), hand->bases[j].back().up());
                PosePtr p = Pose::create(hand->joints[j][4]);
                //Pose p(hand->joints[j][4]);
                //editBeacon(b)->setWorldPose(p);
                editBeacon(b)->setPose(p);
                b++;
            }

            // update buttons
            int dnd_state;
            if (BStates[dnd_btn]) dnd_state = (hand->pinchStrength < dropThreshold) ? 0 : 1;
            else                  dnd_state = (hand->pinchStrength > dragThreshold) ? 1 : 0;
            if (BStates[dnd_btn] != dnd_state || BStates.count(dnd_btn) == 0) {
                change_button(dnd_btn, dnd_state);
            }

        } else { // hand is not there, make it drop if needed
            if (BStates[dnd_btn] != 0) {
                change_button(dnd_btn, 0);
            }
        }
    }

    for (uint i = 0; i < json["pointables"].size(); ++i) { // Get the tools/pens
        auto pointable = json["pointables"][i];

        if (!pointable["tool"].asBool()) continue;

        auto pen = make_shared<VRLeapFrame::Pen>();

        auto tipPosition = pointable["stabilizedTipPosition"];
        pen->tipPosition = Vec3d(tipPosition[0].asFloat(), tipPosition[1].asFloat(), tipPosition[2].asFloat()) * 0.001;

        auto direction = pointable["direction"];
        pen->direction = Vec3d(direction[0].asFloat(), direction[1].asFloat(), direction[2].asFloat());

        pen->length = pointable["length"].asFloat();
        pen->width = pointable["width"].asFloat();

        // only transform when not currently calibrating
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
    Vec3d tmpDir1 = pos1 - pos0; tmpDir1.normalize();
    Vec3d tmpDir2 = position - pos0; tmpDir2.normalize();

    Vec3d direction =  tmpDir2.cross(tmpDir1);
    Vec3d normal = direction.cross(tmpDir1);

    result.set(position, direction, normal);
    result.invert();

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

    result = webSocket.open(url);

    if (result) {
        result = webSocket.sendMessage("{\"background\": true}");
        connectionStatus = "connected to " + getAddress();
    } else {
        connectionStatus = "connection error (daemon running?)";
    }

    return result;
}

void VRLeap::clearSignals() {
    VRDevice::clearSignals();

    // left
    addSignal( 0, 0)->add( getDrop() );
    addSignal( 0, 1)->add( addDrag( getBeacon(0) ) );

    // right
    addSignal( 1, 0)->add( getDrop() );
    addSignal( 1, 1)->add( addDrag( getBeacon(5) ) );
}

VRIntersection findInside(VRObjectWeakPtr wtree, Vec3d point) {

    VRIntersection ins;
    auto tree = wtree.lock();
    if (!tree) return ins;
    if (!tree->getNode()) return ins;
    if (!tree->getNode()->node) return ins;

    uint now = VRGlobals::CURRENT_FRAME;

    vector<VRObjectPtr> children = tree->getChildren(true, "Geometry");

    VRObjectPtr insideObject = nullptr;
    for (auto o : children) {
        auto bb = o->getBoundingbox();
        if (bb->isInside(point)) {
                insideObject = o;
                break;
        }
    }

    if (insideObject) {
        ins.hit = true;
        ins.time = now;
        ins.object = insideObject;
        ins.name = insideObject->getName();
    }

    return ins;
}

void VRLeap::dragCB(VRTransformWeakPtr wcaster, VRObjectWeakPtr wtree, VRDeviceWeakPtr dev) {

    vector<VRObjectPtr> trees;
    if (auto sp = wtree.lock()) trees.push_back(sp);
    else for (auto grp : dynTrees) {
        for (auto swp : grp.second) if (auto sp = swp.second.lock()) trees.push_back(sp);
    }

    VRIntersection ins;
    auto caster = wcaster.lock();

    for (auto t : trees) {
        ins = findInside(t, caster->getFrom());
        if (ins.hit) break;
    }

    drag(ins.object, caster);
}

void VRLeap::setPose(Pose pose) {
    transformation = pose;
    transformed = true;
}

void VRLeap::setPose(Vec3d pos, Vec3d dir, Vec3d up) {
    setPose(Pose(pos, dir, up));
}

Pose VRLeap::getTransformation() {
    return transformation;
}

void VRLeap::startCalibration() {
    calibrate = true;
}

void VRLeap::stopCalibration() {
    calibrate = false;
}


OSG_END_NAMESPACE;
