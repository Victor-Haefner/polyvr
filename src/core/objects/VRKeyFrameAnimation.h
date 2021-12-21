#ifndef VRKEYFRAMEANIMATION_H_INCLUDED
#define VRKEYFRAMEANIMATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include "VRObjectFwd.h"
#include "VRAnimation.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRKeyFrameAnimation : public VRAnimation {
	public:
        struct Source {
            int count = 0;
            int stride = 0;
            vector<float> data;
            vector<string> strData;
        };

        struct Sampler {
            string property;
            VRTransformWeakPtr target;
            map<string, string> sources;
        };

	private:
        map<string, Source> sources;
        map<string, Sampler> samplers;

        Matrix4d extractMatrix(int n1, int n2, float k, Source& sourceOut);

	public:
		VRKeyFrameAnimation(string name = "");
		~VRKeyFrameAnimation();

		static VRKeyFrameAnimationPtr create(string name);
		VRKeyFrameAnimationPtr ptr();

		map<string, Sampler> getSamplers();
		map<string, Source> getSources();

		void addSource(string ID, int count, int stride, vector<float>& data);
		void addSource(string ID, int count, int stride, vector<string>& data);
		void addSampler(string ID, string property, VRTransformPtr target, map<string, string>& sources);

        void animTransform(float t, string samplerID);
};

OSG_END_NAMESPACE;

#endif //VRKEYFRAMEANIMATION_H_INCLUDED
