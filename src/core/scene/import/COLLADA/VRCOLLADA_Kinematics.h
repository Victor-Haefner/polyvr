#ifndef VRCOLLADA_KINEMATICS_H_INCLUDED
#define VRCOLLADA_KINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/import/VRImportFwd.h"

#include <string>
#include <vector>
#include <map>

struct AnimationLibrary;
struct kin_scene;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCOLLADA_Kinematics : public std::enable_shared_from_this<VRCOLLADA_Kinematics> {
	private:
        struct Source {
            int count = 0;
            int stride = 0;
            vector<float> data;
        };

        struct Sampler {
            string target;
            map<string, string> sources;
        };

        VRObjectPtr root;

        map<string, VRAnimationPtr> library_animations;
        string currentAnimation;
        string currentSubAnimation;

        map<string, Sampler> sampler;
        map<string, Source> sources;
        string currentSampler;
        string currentSource;

        void animTransform(float t, VRTransformWeakPtr target, Source sourceIn, Source sourceOut);

        // deprecated
        AnimationLibrary parseColladaAnimations(string data);
        void buildAnimations(AnimationLibrary& lib, VRObjectPtr objects);
        kin_scene parseColladaKinematics(string data);
        void printAllKinematics(const kin_scene& scene);
        void buildKinematics(const kin_scene& scene, VRObjectPtr objects);

	public:
		VRCOLLADA_Kinematics(VRObjectPtr r);
		~VRCOLLADA_Kinematics();

		static VRCOLLADA_KinematicsPtr create(VRObjectPtr r);
		VRCOLLADA_KinematicsPtr ptr();

		void apply(); // deprecated

		void newAnimation(string id, string name);
		void endAnimation();

		void newSampler(string id);
		void newSource(string id);
        void setSourceData(string data);
        void handleAccessor(string count, string stride);
        void handleChannel(string source, string target);
        void handleInput(string type, string sourceID);
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_KINEMATICS_H_INCLUDED
