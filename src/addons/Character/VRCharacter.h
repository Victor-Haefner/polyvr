#ifndef VRCHARACTER_H_INCLUDED
#define VRCHARACTER_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "VRCharacterFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCharacter : public VRGeometry {
    private:
        VRSkeletonPtr skeleton;
        map<string, VRBehaviorPtr> behaviors;
        //map<string, VRBehavior::ActionPtr> actions;

        void updateGeo();

    public:
        VRCharacter(string name );
        ~VRCharacter();

        static VRCharacterPtr create(string name = "JohnDoe");
        VRCharacterPtr ptr();

        void setSkeleton(VRSkeletonPtr s);
        VRSkeletonPtr getSkeleton();

        void move(string endEffector, PosePtr pose);

        void addBehavior(VRBehaviorPtr b);
        //void addAction(VRBehavior::ActionPtr a);

        void simpleSetup();

        void overrideSim();
};

OSG_END_NAMESPACE;

#endif // VRCHARACTER_H_INCLUDED
