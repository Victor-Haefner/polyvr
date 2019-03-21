#include "VRLeapFrame.h"
#include "core/utils/toString.h"
#include "core/objects/VRTransform.h"

using namespace OSG;

template<> string typeName(const VRLeapFrame& o) { return "LeapFrame"; }


VRLeapFramePtr VRLeapFrame::create() {
    auto d = VRLeapFramePtr(new VRLeapFrame());
    return d;
}

HandPtr VRLeapFrame::getLeftHand() { return leftHand; }
HandPtr VRLeapFrame::getRightHand() { return rightHand; }
void VRLeapFrame::setLeftHand(HandPtr hand) { leftHand = hand; }
void VRLeapFrame::setRightHand(HandPtr hand) { rightHand = hand; }

void VRLeapFrame::insertPen(PenPtr pen) { pens.push_back(pen); }
vector<PenPtr> VRLeapFrame::getPens() { return pens; }

HandPtr VRLeapFrame::Hand::clone() {
    auto copy = make_shared<VRLeapFrame::Hand>();

    copy->pose = pose;
    copy->joints = joints;
    copy->bases = bases;
    copy->extended = extended;
    copy->pinchStrength = pinchStrength;
    copy->grabStrength = grabStrength;
    copy->confidence = confidence;

    return copy;
}

/*
InteractionBoxPtr VRLeapFrame::InteractionBox::clone() {
    auto copy = make_shared<VRLeapFrame::InteractionBox>();

    copy->center = center;
    copy->depth = depth;
    copy->height = height;
    copy->width = width;

    return copy;
};
*/

VRLeapFramePtr VRLeapFrame::ptr() { return static_pointer_cast<VRLeapFrame>( shared_from_this() ); }

Pose getDirTransform(Pose transformation) {
    Matrix4d dirTransformation = transformation.asMatrix();
    dirTransformation.invert();
    dirTransformation.transpose();
    Pose dirTransform(dirTransformation);
    return dirTransform;
}

void VRLeapFrame::Hand::transform(Pose transformation) {

    Pose dirTransform = getDirTransform(transformation);

    // transform pose
    pose->set(transformation.transform(pose->pos()), dirTransform.transform(pose->dir()), dirTransform.transform(pose->up()));

    // transform joint positions
    for (auto& finger : joints) {
        for (auto& j : finger) { j = transformation.transform(j); }
    }

    // transform basis vectors for each bone
    for (auto& finger : bases) {
        for (auto& bone : finger) {
            bone.set(transformation.transform(bone.pos()), dirTransform.transform(bone.dir()), dirTransform.transform(bone.up()));
        }
    }

}

void VRLeapFrame::Pen::transform(Pose transformation) {
    transformation.transform(tipPosition);

    Pose dirTransform = getDirTransform(transformation);
    dirTransform.transform(direction);
}



