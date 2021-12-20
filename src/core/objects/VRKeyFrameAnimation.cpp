#include "VRKeyFrameAnimation.h"

using namespace OSG;

VRKeyFrameAnimation::VRKeyFrameAnimation(string name) {}
VRKeyFrameAnimation::~VRKeyFrameAnimation() {}

VRKeyFrameAnimationPtr VRKeyFrameAnimation::create(string name) { return VRKeyFrameAnimationPtr( new VRKeyFrameAnimation(name) ); }
VRKeyFrameAnimationPtr VRKeyFrameAnimation::ptr() { return static_pointer_cast<VRKeyFrameAnimation>(shared_from_this()); }

void VRKeyFrameAnimation::addSource(string ID, int count, int stride, vector<float>& data) {
    sources[ID] = Source();
    sources[ID].count = count;
    sources[ID].stride = stride;
    sources[ID].data = data;
}

void VRKeyFrameAnimation::addSource(string ID, int count, int stride, vector<string>& data) {
    sources[ID] = Source();
    sources[ID].count = count;
    sources[ID].stride = stride;
    sources[ID].strData = data;
}

void VRKeyFrameAnimation::addSampler(string ID, string property, VRTransformPtr target, vector<string>& sources) {
    samplers[ID] = Sampler();
    samplers[ID].property = property;
    samplers[ID].target = target;
    samplers[ID].sources = sources;
}
