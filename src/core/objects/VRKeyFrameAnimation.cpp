#include "VRKeyFrameAnimation.h"
#include "core/utils/VRFunction.h"
#include "VRTransform.h"

using namespace OSG;

VRKeyFrameAnimation::VRKeyFrameAnimation(string name) {}
VRKeyFrameAnimation::~VRKeyFrameAnimation() {}

VRKeyFrameAnimationPtr VRKeyFrameAnimation::create(string name) { return VRKeyFrameAnimationPtr( new VRKeyFrameAnimation(name) ); }
VRKeyFrameAnimationPtr VRKeyFrameAnimation::ptr() { return static_pointer_cast<VRKeyFrameAnimation>(shared_from_this()); }

map<string, VRKeyFrameAnimation::Sampler> VRKeyFrameAnimation::getSamplers() { return samplers; }
map<string, VRKeyFrameAnimation::Source>  VRKeyFrameAnimation::getSources()  { return sources;  }

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

void VRKeyFrameAnimation::addSampler(string ID, string property, VRTransformPtr target, map<string, string>& srces) {
    samplers[ID] = Sampler();
    samplers[ID].property = property;
    samplers[ID].target = target;
    samplers[ID].sources = srces;

    string sInID = samplers[ID].sources["INPUT"];
    if (!sources.count(sInID)) return;
    Source& sourceIn  = sources[sInID];
    if (sourceIn.data.size() == 0) return;

    float T = sourceIn.data[ sourceIn.data.size()-1 ] - sourceIn.data[ 0 ];
    addCallback( VRAnimCb::create("key_frame_anim", bind(&VRKeyFrameAnimation::animTransform, this, placeholders::_1, ID)) );
    setDuration(T);
}


void VRKeyFrameAnimation::animTransform(float t, string samplerID) {
    if (!samplers.count(samplerID)) return;
    auto& sampler = samplers[samplerID];

    VRTransformWeakPtr target = sampler.target;
    string property = sampler.property;

    string sInID  = sampler.sources["INPUT"];
    string sOutID = sampler.sources["OUTPUT"];
    string sInterpID = sampler.sources["INTERPOLATION"];
    if (!sources.count(sInID)) return;
    if (!sources.count(sOutID)) return;
    if (!sources.count(sInterpID)) return;
    Source& sourceIn  = sources[sInID];
    Source& sourceOut = sources[sOutID];
    Source& sourceInterp = sources[sInterpID];
    if (sourceIn.data.size() == 0) return;

    auto obj = target.lock();
    if (!obj) return;

    float T = sourceIn.data[ sourceIn.data.size()-1 ] - sourceIn.data[0];
    float time = T*t + sourceIn.data[0];

    auto tItr = upper_bound(sourceIn.data.begin(), sourceIn.data.end(), time); // TODO: optimize by keeping track of n1/n2 ?
    int n2 = tItr - sourceIn.data.begin();
    int n1 = n2-1;

    float t1 = sourceIn.data[n1];
    float t2 = sourceIn.data[n2];
    float dt = t2-t1;
    float k = (time-t1)/dt; // key frame interpolation

    string interp = sourceInterp.strData[n1]; // TODO

    //cout << " animTransform " << t << " -> " << time << ", " << t1 << " / " << t2 << ", " << n1 << ", " << n2 << ", k: " << k << ", interp: " << interp << endl;

    if (property == "transform") {
        Matrix4d m = extractMatrix(n1, n2, k, sourceOut);
        obj->setMatrix(m);
    }

    if (property == "location.X" || property == "location.Y" || property == "location.Z") {
        float x = sourceOut.data[n1]*(1.0-k) + sourceOut.data[n2]*k; // linear
        Vec3d p = obj->getFrom();
        if (property == "location.X") p[0] = x;
        if (property == "location.Y") p[1] = x;
        if (property == "location.Z") p[2] = x;
        obj->setFrom(p);
    }

    if (property == "rotationX.ANGLE" || property == "rotationY.ANGLE" || property == "rotationZ.ANGLE") {
        float x = sourceOut.data[n1]*(1.0-k) + sourceOut.data[n2]*k; // linear
        Vec3d p = obj->getEuler();
        if (property == "rotationX.ANGLE") p[0] = x/180.0*Pi;
        if (property == "rotationY.ANGLE") p[1] = x/180.0*Pi;
        if (property == "rotationZ.ANGLE") p[2] = x/180.0*Pi;
        obj->setEuler(p);
    }
}

Matrix4d VRKeyFrameAnimation::extractMatrix(int n1, int n2, float k, Source& sourceOut) {
    if (sourceOut.stride != 16) return Matrix4d();
    int h = n1*16;
    int j = n2*16;
    float u = (1.0-k);
    auto& d = sourceOut.data;
    return Matrix4d( d[h+0] *u + d[j+0] *k, d[h+1] *u + d[j+1] *k, d[h+2] *u + d[j+2] *k, d[h+3] *u + d[j+3] *k,
                     d[h+4] *u + d[j+4] *k, d[h+5] *u + d[j+5] *k, d[h+6] *u + d[j+6] *k, d[h+7] *u + d[j+7] *k,
                     d[h+8] *u + d[j+8] *k, d[h+9] *u + d[j+9] *k, d[h+10]*u + d[j+10]*k, d[h+11]*u + d[j+11]*k,
                     d[h+12]*u + d[j+12]*k, d[h+13]*u + d[j+13]*k, d[h+14]*u + d[j+14]*k, d[h+15]*u + d[j+15]*k );
}
