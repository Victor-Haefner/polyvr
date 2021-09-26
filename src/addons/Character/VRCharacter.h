#ifndef VRCHARACTER_H_INCLUDED
#define VRCHARACTER_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "VRCharacterFwd.h"

struct WalkMotion;
struct MoveTarget;

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
        map<string, MoveTarget*> target;
        //map<string, VRBehavior::ActionPtr> actions;
        void update();
        void pathWalk(float t);
        void moveEE(float t, string endEffector);

    public:
        VRCharacter(string name );
        ~VRCharacter();

        static VRCharacterPtr create(string name = "JohnDoe");
        VRCharacterPtr ptr();

        void setSkeleton(VRSkeletonPtr s);
        VRSkeletonPtr getSkeleton();

        PathPtr move(string endEffector, PosePtr pose, float s);
        PathPtr moveTo(Vec3d p, float s);
        PathPtr grab(Vec3d p, float s);

        void addBehavior(VRBehaviorPtr b);
        //void addAction(VRBehavior::ActionPtr a);

        void simpleSetup();
};

OSG_END_NAMESPACE;

#endif // VRCHARACTER_H_INCLUDED
