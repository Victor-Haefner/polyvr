#ifndef VRCOLLADA_KINEMATICS_H_INCLUDED
#define VRCOLLADA_KINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/import/VRImportFwd.h"

#include <string>
#include <map>

struct AnimationLibrary;
struct kin_scene;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCOLLADA_Kinematics : public std::enable_shared_from_this<VRCOLLADA_Kinematics> {
	private:
        map<string, VRAnimationPtr> library_animations;
        string currentAnimation;
        string currentSubAnimation;

        // deprecated
        AnimationLibrary parseColladaAnimations(string data);
        void buildAnimations(AnimationLibrary& lib, VRObjectPtr objects);
        kin_scene parseColladaKinematics(string data);
        void printAllKinematics(const kin_scene& scene);
        void buildKinematics(const kin_scene& scene, VRObjectPtr objects);

	public:
		VRCOLLADA_Kinematics();
		~VRCOLLADA_Kinematics();

		static VRCOLLADA_KinematicsPtr create();
		VRCOLLADA_KinematicsPtr ptr();

		void apply(); // deprecated

		void newAnimation(string id, string name);
		void endAnimation();
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_KINEMATICS_H_INCLUDED
