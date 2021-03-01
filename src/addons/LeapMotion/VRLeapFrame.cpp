#include "VRLeapFrame.h"
#include "core/utils/toString.h"
#include "core/objects/VRTransform.h"

using namespace OSG;

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

VRLeapFramePtr VRLeapFrame::ptr() { return static_pointer_cast<VRLeapFrame>( shared_from_this() ); }

void VRLeapFrame::Hand::transform(Pose transformation) {

    // transform pose
    pose->set(transformation.transform(pose->pos()), transformation.transform(pose->dir(), false), transformation.transform(pose->up(), false));

    // transform joint positions
    for (auto& finger : joints) {
        for (auto& j : finger) { j = transformation.transform(j); }
    }

    // transform basis vectors for each bone
    for (auto& finger : bases) {
        for (auto& bone : finger) {
            bone.set(transformation.transform(bone.pos()), transformation.transform(bone.dir(), false), transformation.transform(bone.up(), false));
        }
    }

}

void VRLeapFrame::Pen::transform(Pose transformation) {
    pose->setPos( transformation.transform(pose->pos()) );
    pose->setDir( transformation.transform(pose->dir(), false) );
}

VRLeapFrame::Pen::Pen() { pose = Pose::create(); }
VRLeapFrame::Pen::~Pen() {}



