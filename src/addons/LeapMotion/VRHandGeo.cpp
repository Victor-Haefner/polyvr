#include "VRHandGeo.h"
#include <core/objects/geometry/OSGGeometry.h>
#include <core/math/pose.h>
#include <OpenSG/OSGSimpleGeometry.h>

using namespace OSG;

VRHandGeo::VRHandGeo(string name) : VRGeometry(name), bones(5) {
    setPrimitive("Sphere", "0.01 2"); // palm geometry
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 4; ++j) {
            VRGeometryPtr geo = VRGeometry::create(name + "_bone_" + to_string(i) + "_" + to_string(j));
            bones[i].push_back(geo);
        }
        VRGeometryPtr geo = VRGeometry::create(name + "_dir_" + to_string(i));
        geo->setPrimitive("Arrow", "0.1 0.02 0.005 0.03 0.005");
        directions.push_back(geo);
    }
    pinch = VRGeometry::create(name + "_pinch", true);
    pinch->setPrimitive("Sphere", "0.025 2");
}

VRHandGeoPtr VRHandGeo::create(string name) {
    VRHandGeoPtr ptr = VRHandGeoPtr(new VRHandGeo(name));
    for (auto& b_ : ptr->bones) {
        for (auto& b : b_) {
            ptr->addChild(b);
        }
    }
//    for (auto& d : ptr->directions) {
//        ptr->addChild(d);
//    }
    ptr->addChild(ptr->pinch);
    ptr->hide();
    return ptr;
}

void VRHandGeo::connectToLeap(VRLeapPtr leap) {
    function<void(VRLeapFramePtr)> cb = [this](VRLeapFramePtr frame) {
        update(frame);
    };
    leap->registerFrameCallback(cb);
}


void VRHandGeo::updateChange() {
    mutex.lock();

    if (visible != isVisible()) { toggleVisible(); }

    if (handData && isVisible()) {

        // Palm
        auto p = handData->pose;
        cout << p.toString() << endl;
        //setPose(p);

        // Bones
        for (int i = 0; i < 5; ++i) {
            Vec3d p0 = (handData->joints[i][0] + handData->joints[i][1]) / 2;
            Vec3d p1 = (handData->joints[i][1] + handData->joints[i][2]) / 2;
            Vec3d p2 = (handData->joints[i][2] + handData->joints[i][3]) / 2;
            Vec3d p3 = (handData->joints[i][3] + handData->joints[i][4]) / 2;
            float l0 = (handData->joints[i][0] - handData->joints[i][1]).length();
            float l1 = (handData->joints[i][1] - handData->joints[i][2]).length();
            float l2 = (handData->joints[i][2] - handData->joints[i][3]).length();
            float l3 = (handData->joints[i][3] - handData->joints[i][4]).length();

            // The thumb does not have a base metacarpal bone and therefore contains a valid, zero length bone at that location.
            if (l0 > 0) {
                bones[i][0]->setMesh(OSGGeometry::create(makeCylinderGeo(l0, 0.0075, 4, true, true, true)));
                bones[i][0]->setWorldPose(Pose::create(p0, handData->bases[i][0].dir(), handData->bases[i][0].up()));
            } else {
                bones[i][0]->hide();
            }

            bones[i][1]->setMesh(OSGGeometry::create(makeCylinderGeo(l1, 0.0075, 4, true, true, true)));
            bones[i][1]->setWorldPose(Pose::create(p1, handData->bases[i][1].dir(), handData->bases[i][1].up()));

            bones[i][2]->setMesh(OSGGeometry::create(makeCylinderGeo(l2, 0.0075, 4, true, true, true)));
            bones[i][2]->setWorldPose(Pose::create(p2, handData->bases[i][2].dir(), handData->bases[i][2].up()));

            bones[i][3]->setMesh(OSGGeometry::create(makeCylinderGeo(l3, 0.0075, 4, true, true, true)));
            bones[i][3]->setWorldPose(Pose::create(p3, handData->bases[i][3].dir(), handData->bases[i][3].up()));
        }
/*
        // Directions
        for (int i = 0; i < 5; ++i) {
            if (handData->extended[i]) {
                Vec3d d = handData->directions[i];
                directions[i]->setWorldPosition(handData->joints[i][4] + (0.1 * d));
                directions[i]->setWorldDir(handData->directions[i]);
                directions[i]->setWorldUp(handData->bases[i][1].up());
                if (!directions[i]->isVisible()) { directions[i]->show(); }
            } else {
                if (directions[i]->isVisible()) { directions[i]->hide(); }
            }
        }

        // Pinch
        if (handData->pinchStrength > 0.98f) {
            pinch->setWorldPosition(handData->joints[0][4]); // thumb tip
            if (!pinch->isVisible()) pinch->show();
        } else {
            if (pinch->isVisible()) pinch->hide();
        }
*/
    }


    mutex.unlock();
   // VRTransform::updateChange();
}

void VRHandGeo::update(VRLeapFramePtr frame) {

    cout << "update" << endl;

    //boost::mutex::scoped_lock lock(mutex);
    mutex.lock();

    if (isLeft) {
        handData = frame->getLeftHand();
    } else {
        handData = frame->getRightHand();
    }
    if (!handData) {
        if (visible) {
            visible = false;
        }
    } else {
        if (!visible) visible = true;
    }

    mutex.unlock();
    reg_change();
}

void VRHandGeo::setLeft() { isLeft = true; }

void VRHandGeo::setRight() { isLeft = false; }

