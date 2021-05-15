#ifndef VRCHARACTER_H_INCLUDED
#define VRCHARACTER_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "VRCharacterFwd.h"

struct WalkMotion;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCharacter : public VRGeometry {
    private:
        VRSkeletonPtr skeleton;
        VRSkinPtr skin;
        map<string, VRBehaviorPtr> behaviors;
        VRUpdateCbPtr updateCb;
        VRAnimationPtr walkAnim;
        WalkMotion* motion = 0;
        //map<string, VRBehavior::ActionPtr> actions;
        void update();
        void pathWalk(float t);

    public:
        VRCharacter(string name );
        ~VRCharacter();

        static VRCharacterPtr create(string name = "JohnDoe");
        VRCharacterPtr ptr();

        void setSkeleton(VRSkeletonPtr s);
        VRSkeletonPtr getSkeleton();

        void move(string endEffector, PosePtr pose);
        PathPtr moveTo(Vec3d p);

        void addBehavior(VRBehaviorPtr b);
        //void addAction(VRBehavior::ActionPtr a);

        void simpleSetup();
};

OSG_END_NAMESPACE;

#endif // VRCHARACTER_H_INCLUDED
