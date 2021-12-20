#ifndef VRKEYFRAMEANIMATION_H_INCLUDED
#define VRKEYFRAMEANIMATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRObjectFwd.h"
#include "VRAnimation.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRKeyFrameAnimation : public VRAnimation {
	private:
        struct Source {
            int count = 0;
            int stride = 0;
            vector<float> data;
            vector<string> strData;
        };

        struct Sampler {
            string property;
            VRTransformWeakPtr target;
            vector<string> sources;
        };

        map<string, Source> sources;
        map<string, Sampler> samplers;

	public:
		VRKeyFrameAnimation(string name = "");
		~VRKeyFrameAnimation();

		static VRKeyFrameAnimationPtr create(string name);
		VRKeyFrameAnimationPtr ptr();

		void addSource(string ID, int count, int stride, vector<float>& data);
		void addSource(string ID, int count, int stride, vector<string>& data);

		void addSampler(string ID, string property, VRTransformPtr target, vector<string>& sources);
};

OSG_END_NAMESPACE;

#endif //VRKEYFRAMEANIMATION_H_INCLUDED
