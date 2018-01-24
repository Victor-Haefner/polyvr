#include "VRLeapFrame.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE ;


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
    copy->directions = directions;
    copy->pinchStrength = pinchStrength;
    copy->grabStrength = grabStrength;
    copy->confidence = confidence;

    return copy;
}

VRLeapFramePtr VRLeapFrame::ptr() { return static_pointer_cast<VRLeapFrame>( shared_from_this() ); }

void VRLeapFrame::Hand::transform(Matrix4d transformation) {

    Matrix4d dirTransformation = transformation;
    dirTransformation.invert();
    dirTransformation.transpose();

    // transform pose
    pose.set(transformation*pose.pos(), dirTransformation*pose.dir(), dirTransformation*pose.up());

    // transform joint positions
    for (auto& finger : joints) {
        for (auto& j : finger) { j = transformation * j; }
    }

    // transform basis vectors for each bone
    for (auto& finger : bases) {
        for (auto& bone : finger) {
            bone.set(transformation*bone.pos(), dirTransformation*bone.dir(), dirTransformation*bone.up());
        }
    }

    // transform directions
    for (auto& dir : directions) {
        dir = dirTransformation * dir;
    }

}

void VRLeapFrame::Pen::transform(Matrix4d transformation) {
    tipPosition = transformation * tipPosition;

    Matrix4d dirTransformation = transformation;
    dirTransformation.invert();
    dirTransformation.transpose();

    direction = dirTransformation * direction;
}



OSG_END_NAMESPACE;
