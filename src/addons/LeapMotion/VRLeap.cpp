#include "VRLeap.h"
#include "core/objects/VRTransform.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/setup/devices/VRSignal.h"
#include "core/objects/OSGObject.h"
#include "core/utils/VRGlobals.h"
#include "core/tools/VRAnalyticGeometry.h"
#include "core/math/boundingbox.h"
#include "core/scene/VRScene.h"
#include "addons/LeapMotion/VRHandGeo.h"
#include <boost/thread/recursive_mutex.hpp>

using namespace OSG;

Vec3d VRLeapHistory::add(Vec3d v, float f) { // TODO
    return v;

    // extend history
    I++;
    if (I >= N) I = 0;
    if (I >= (int)history.size()) history.push_back( make_pair(f,v) );
    else history[I] = make_pair(f,v);
    cout << "  " << I << "  " << history[I].second << endl;

    // get mean over K last values
    Vec3d mean;
    float weight = 0;
    for (int j=0; j<K; j++) {
        int k = I-j;
        while (k < 0) k += history.size();
        float w = history[k].first;
        mean += history[k].second*w;
        weight += w;
    }
    //return mean*(1.0/weight);
    return history[0].second;
}


VRLeap::VRLeap() : VRDevice("leap") {
    mutex = new boost::recursive_mutex();
    transformation = Pose::create();

    //TODO: Debugging only
    numPens = 0;

    // left hand beacons
    //addBeacon(Vec3d(0,1,-1)); // left root
    addBeacon(Vec3d(0,0,-1)); // left root
    for (int i = 1; i <= 5; ++i) {
            addBeacon(Vec3d(0,1,-1));
            getBeacon(i)->switchParent(getBeacon(0));
    }

    // right hand beacons
    //addBeacon(Vec3d(0,1,-1)); // right root
    addBeacon(Vec3d(0,0,-1)); // right root
    for (int i = 7; i <= 11; ++i) {
            addBeacon(Vec3d(0,1,-1));
            getBeacon(i)->switchParent(getBeacon(6));
    }

#ifndef WITHOUT_JSONCPP
    auto cb = [&](Json::Value msg) {
        newFrame(msg);
    };

    webSocket.registerJsonCallback(cb);
#endif

    store("host", &host);
    store("port", &port);
    store("transformation", &transformation);

    // TODO: apparently needs to be a StorageCb instead of UpdateCb.
    regStorageSetupFkt( VRStorageCb::create("leap setup", bind(&VRLeap::setup, this)) );
//    regStorageSetupFkt( VRUpdateCb::create("leap setup", bind(&VRLeap::setup, this)) );

    //reconnect();
    enableAvatar("ray", 0);
    enableAvatar("ray", 6);
}

VRLeap::~VRLeap() {
    cout << "~VRLeap" << endl;
    delete mutex;
    frameCallbacks.clear();
    webSocket.close();
}

VRLeapPtr VRLeap::create() {
    auto d = VRLeapPtr(new VRLeap());
    d->initIntersect(d);
    d->clearSignals();
    return d;
}

void VRLeap::setup() {
    reconnect();
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


#ifndef WITHOUT_JSONCPP
void VRLeap::updateHandFromJson(Json::Value& handData, Json::Value& pointableData, HandPtr hand) {

        auto pos = handData["palmPosition"];
        hand->pose->setPos( Vec3d(pos[0].asFloat(), pos[1].asFloat(), pos[2].asFloat())* 0.001f );
        auto dir = handData["direction"];
        hand->pose->setDir( Vec3d(dir[0].asFloat(), dir[1].asFloat(), dir[2].asFloat()) );
        auto normal = handData["palmNormal"];
        hand->pose->setUp( Vec3d(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat()) );

        hand->pinchStrength = handData["pinchStrength"].asFloat();
        hand->grabStrength = handData["grabStrength"].asFloat();
        hand->confidence = handData["confidence"].asFloat();
        hand->isPinching = hand->isPinching ? hand->pinchStrength < dropThreshold : hand->pinchStrength > dragThreshold;

        int id = handData["id"].asInt();

        for (unsigned int j = 0; j < pointableData.size(); ++j) { // Get the corresponding fingers
            auto pointable = pointableData[j];

            if (pointable["handId"].asInt() != id) continue;

            int type = pointable["type"].asInt();

            // Setup joint positions
            // Joints are numbered from wrist outwards
            // arrays of 3 floats each
            vector<Json::Value> joints = {pointable["carpPosition"],    // the position of the base of metacarpal bone
                                          pointable["mcpPosition"],     // metacarpophalangeal joint, or knuckle, of the finger
                                          pointable["pipPosition"],     // proximal interphalangeal joint of the finger
                                          pointable["dipPosition"],     // the position of the base of the distal phalanx
                                          pointable["btipPosition"]};   // the position of the tip of the distal phalanx

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

            // Setup extension
            hand->extended[type] = pointable["extended"].asBool();
        }
}
#endif

VRTransformPtr VRLeap::getBeaconChild(int i) {
    boost::recursive_mutex::scoped_lock lock(*mutex);
    cout << static_pointer_cast<VRTransform>( getBeacon()->getChild(i) )->getFrom() << endl;
    return static_pointer_cast<VRTransform>( getBeacon()->getChild(i) );
}

void VRLeap::updateSceneData(vector<HandPtr> hands) {

    boost::recursive_mutex::scoped_lock lock(*mutex);

    // beaconRoot->Setup->Camera
    auto parent = getBeaconRoot()->getParent()->getParent();

    for (int i = 0; i < 2; ++i) {
        HandPtr hand = hands[i];
        int dnd_btn = i; // drag button 0 for left, 1 for right hand
        if (hand) {
            enableAvatar("ray", 6*i);

            // update beacons
            int rootIdx = i * 6;
            getBeacon(rootIdx)->setRelativePose(hand->pose, parent);
            //getBeacon(rootIdx)->setPose(hand->pose);

            int b = i * 5;
            for (size_t j = 0; j < hand->bases.size(); ++j) {
                PosePtr p = Pose::create(hand->joints[j][4], hand->bases[j].back().dir(), hand->bases[j].back().up());
                getBeacon(rootIdx+j+1)->setRelativePose(p, parent);
                //getBeacon(rootIdx+j+1)->setPose(p);
                b++;
            }

            // update buttons
            int dnd_state = hand->isPinching;

            //cout << "VRLeap::updateSceneData " << dnd_state << "   " << hand->pinchStrength << "   " << dropThreshold << endl;
            if (BStates[dnd_btn] != dnd_state || BStates.count(dnd_btn) == 0) {
                change_button(dnd_btn, dnd_state);
            }
        } else { // hand is not there, make it drop if needed
            disableAvatar("ray", 6*i);
            if (BStates[dnd_btn] != 0) {
                change_button(dnd_btn, 0);
            }
        }
    }
}

#ifndef WITHOUT_JSONCPP
void VRLeap::newFrame(Json::Value json) {

    // json format: https://developer.leapmotion.com/documentation/v2/cpp/supplements/Leap_JSON.html?proglang=cpp

    if (json.isMember("event")) { // second frame
        if (transformation->asMatrix() != Matrix4d::identity()) { transformed = true; }
        if (serial.empty()) {
            serial = json["event"]["state"]["id"].asString();
        }
        return;
    }

    VRLeapFramePtr frame = VRLeapFrame::create();
    hands = vector<HandPtr>(2, nullptr);//vector<HandPtr> hands(2, nullptr);

    for (unsigned int i = 0; i < json["hands"].size(); ++i) { // Get the hands

        auto newHand = json["hands"][i];
        auto hand = make_shared<VRLeapFrame::Hand>();

        // Setup new hand
        updateHandFromJson(newHand, json["pointables"], hand);

        if (transformed) {
            hand->transform(*transformation);
        }

        bool left = (newHand["type"].asString() == "left");

        if (left)   { hands[0] = hand; frame->setLeftHand(hand); }
        else        { hands[1] = hand; frame->setRightHand(hand); }
    }

    auto scene = VRScene::getCurrent();
    if (scene) {
        auto fkt = VRUpdateCb::create("leap_hands_update", bind(&VRLeap::updateSceneData, this, hands));
        VRScene::getCurrent()->queueJob(fkt);
    }

    //TODO: Debugging only!
    if ((int)json["pointables"].size() != numPens) {
        std::cout << "Number of recognized pens: " << json["pointables"].size() << ". Previously was: " << numPens << endl;
        numPens = json["pointables"].size();
    }

    // Get the currently recognized tools/pens
    for (unsigned int i = 0; i < json["pointables"].size(); ++i) {
        auto pointable = json["pointables"][i];

        if (!pointable["tool"].asBool()) continue;

        auto pen = make_shared<VRLeapFrame::Pen>();

        auto p = pointable["stabilizedTipPosition"];
        pen->pose->setPos(Vec3d(p[0].asFloat(), p[1].asFloat(), p[2].asFloat()) * 0.001);

        auto d = pointable["direction"];
        pen->pose->setDir(Vec3d(d[0].asFloat(), d[1].asFloat(), d[2].asFloat()));

        pen->length = pointable["length"].asFloat();
        pen->width = pointable["width"].asFloat();

        // only transform when not currently calibrating
        if (transformed && !calibrate) { pen->transform(*transformation); }

        frame->insertPen(pen);
    }

    if (calibrate) {
        vector<PenPtr> pens = frame->getPens();
        if (pens.size() == 2) {
            auto pose = computeCalibPose(pens);
            setPose(pose);
        }
    }

    for (auto& cb : frameCallbacks) cb(frame);
}
#endif
PosePtr VRLeap::computeCalibPose(vector<PenPtr>& pens) {
    PosePtr result = Pose::create();
    if (pens.size() != 2) { return result; }

    Vec3d pos0 = pens[0]->pose->pos();
    Vec3d pos1 = pens[1]->pose->pos();
    Vec3d dir0 = pens[0]->pose->dir();
    Vec3d dir1 = pens[1]->pose->dir();

    Vec3d position = ((pos0 - 0.15 * dir0) + (pos1 - 0.15 * dir1)) / 2.0;
    Vec3d tmpDir1 = pos1 - pos0; tmpDir1.normalize();
    Vec3d tmpDir2 = position - pos0; tmpDir2.normalize();

    Vec3d direction =  tmpDir2.cross(tmpDir1);
    Vec3d normal = direction.cross(tmpDir1);

    result->set(position, direction, normal);
    result->invert();
    return result;
}

void VRLeap::calibrateManual(Vec3d position, Vec3d direction, Vec3d normal) {
    PosePtr pose = Pose::create();
    pose->set(position, direction, normal);
    //pose->invert();

    setPose(pose);
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

    /**
    * drag left:  0
    * drag right: 1
    *
    * tap left:   2 (TODO)
    * tap right:  3
    *
    * beacons left: 1-5 (root: 0)
    * beacons right: 7-11 (root: 6)
    */

    // left
    newSignal( 0, 0)->add( getDrop() );
    newSignal( 0, 1)->add( addDrag( getBeacon(0) ) ); // 1

    // right
    newSignal( 1, 0)->add( getDrop() );
    newSignal( 1, 1)->add( addDrag( getBeacon(6) ) ); // 7

    // TODO: maybe use the drag and drop signals?
    dndCb = VRFunction<VRDeviceWeakPtr>::create("leapDnD", bind(&VRLeap::leapDnD, this, placeholders::_1));
    newSignal(0, 1)->add(dndCb);
    newSignal(1, 1)->add(dndCb);
    newSignal(0, 0)->add(dndCb);
    newSignal(1, 0)->add(dndCb);
}

VRIntersection findInside(VRObjectWeakPtr wtree, Vec3d point) {

    VRIntersection ins;
    auto tree = wtree.lock();
    if (!tree) return ins;
    if (!tree->getNode()) return ins;
    if (!tree->getNode()->node) return ins;

    unsigned int now = VRGlobals::CURRENT_FRAME;

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
        ins = findInside(t, caster->getWorldPosition());
        if (ins.hit) break;
    }

    VRIntersect::drag(ins, caster);
}

void VRLeap::setPose(PosePtr pose) {
    transformation = pose;
    getBeaconRoot()->setPose(pose);
    transformed = true;
}

PosePtr VRLeap::getPose() {
    return transformation;
}

void VRLeap::startCalibration() {
    calibrate = true;
}

void VRLeap::stopCalibration() {
    calibrate = false;
}

void VRLeap::setDragThreshold(float t){
    dragThreshold = t;
}

void VRLeap::setDropThreshold(float t){
    dropThreshold = t;
}

float VRLeap::getDragThreshold(){
    return dragThreshold;
}

float VRLeap::getDropThreshold(){
    return dropThreshold;
}

float VRLeap::getPinchStrength(int hand) {
    if (hands[hand] == nullptr) {
            return 0;
    } else return hands[hand]->pinchStrength;

}

float VRLeap::getGrabStrength(int hand) {
    if (hands[hand] == nullptr) {
            return 0;
    } else return hands[hand]->grabStrength;
}
float VRLeap::getConfidence(int hand) {
    if (hands[hand] == nullptr) {
            return 0;
    } else return hands[hand]->confidence;
}
bool VRLeap::getIsPinching(int hand) {
    if (hands[hand] == nullptr) {
            return 0;
    } else return hands[hand]->isPinching;
}

vector<PosePtr> VRLeap::getHandPose(){
    vector<PosePtr> res;
    for (auto h : hands) {
        if(!h) return res;
        res.push_back(h->pose);
    }
    return res;
}

void VRLeap::enableDnD(VRObjectPtr root) {
    doDnD = true;
    dndRoot = root;

}

void VRLeap::leapDnD(VRDeviceWeakPtr dev) {
    if (!doDnD) return;

    /*if (!ageo) {
        ageo = VRAnalyticGeometry::create();
        getBeacon()->getParent()->getParent()->getParent()->getParent()->addChild(ageo);
    }*/

    for (auto key : {0,1}) {
        int bID = key*6; // 0 and 6
        auto s = b_state(key);

        auto beacon = getBeacon(bID);
        if (s == 1) {
            Pnt3f p0 = Pnt3f(beacon->getWorldPosition());
            Vec3f d = Vec3f(0,-1,0);
            //ageo->setVector(key, Vec3d(p0), Vec3d(0,-1,0), Color3f(1,0,0));

            Line ray(p0, d);
            auto intersection = intersectRay(dndRoot, ray);
            if ( intersection.hit ) {
                auto i = intersection.object.lock();
                if (i) drag(i, bID);
            }
        } else drop(bID);
    }
}

VRLeapPtr VRLeap::ptr() { return static_pointer_cast<VRLeap>( shared_from_this() ); }

vector<VRObjectPtr> VRLeap::addHandsGeometry(){
    vector<VRObjectPtr> res;
    auto handLeft = VRHandGeo::create("leftHand"); //new VRHandGeo("leftHand");
    handLeft->setLeft();
    handLeft->connectToLeap(ptr());
    res.push_back(handLeft);
    getBeaconRoot()->addChild(handLeft);

    auto handRight = VRHandGeo::create("rightHand");
    handRight->setRight();
    handRight->connectToLeap(ptr());
    res.push_back(handRight);
    getBeaconRoot()->addChild(handRight);

    return res;
}

