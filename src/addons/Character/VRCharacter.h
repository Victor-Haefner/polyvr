#ifndef VRCHARACTER_H_INCLUDED
#define VRCHARACTER_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "VRCharacterFwd.h"
#include "core/math/graph.h"
#include "core/math/pose.h"
#include "core/utils/VRName.h"
#include "core/objects/geometry/VRConstraint.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSkeleton : public VRGeometry {
    public:
        struct Configuration : public VRName {
            map<int,Vec3f> joints;

            Configuration(string name);
            ~Configuration();

            static shared_ptr<Configuration> create(string name);

            void setPose(int i, Vec3f p);

            Configuration interpolate(Configuration& other);
        };

        struct Joint : VRConstraint {
            void update(Graph::node& n, bool changed);
            static shared_ptr<Joint> create();
            shared_ptr<Joint> duplicate();
        };

        typedef shared_ptr<Configuration> ConfigurationPtr;
        typedef shared_ptr<Joint> JointPtr;
        typedef Graph Armature;
        typedef shared_ptr<Armature> ArmaturePtr;

    private:
        int rootBone = -1;
        ArmaturePtr armature;
        map<int, JointPtr> joints;

        void initMaterial(); // skeleton visualisation

    public:
        VRSkeleton();
        ~VRSkeleton();

        static VRSkeletonPtr create();

        int addBone(int joint1, int joint2);
        int addJoint(JointPtr c, Vec3f p);

        //Graph::edge getBone(int id);
        VRConstraintPtr getJoint(int id);

        void simpleHumanoid();

        void asGeometry(VRGeoData& data);
        void setupGeometry();
        void updateGeometry();
};

class VRBehavior : public VRName {
    public:
        struct Action : public VRName {
            map<string,VRSkeleton::ConfigurationPtr> configurations;

            Action(string name);
            ~Action();

            static shared_ptr<Action> create(string name);

            void addConfiguration(VRSkeleton::ConfigurationPtr c);
        };

        typedef shared_ptr<Action> ActionPtr;

    private:
        VRSkeletonPtr skeleton;
        map<string, ActionPtr> actions; // perhaps a state machine??
        VRSkeleton::ConfigurationPtr current;
        VRAnimationPtr animation;

        void updateBones();

    public:
        VRBehavior(string name);
        ~VRBehavior();

        static VRBehaviorPtr create(string name);

        void setSkeleton(VRSkeletonPtr s);
};

class VRCharacter : public VRGeometry {
    private:
        VRSkeletonPtr skeleton;
        map<string, VRBehaviorPtr> behaviors;
        map<string, VRBehavior::ActionPtr> actions;

        void updateGeo();

    public:
        VRCharacter(string name );
        ~VRCharacter();

        static VRCharacterPtr create(string name = "JohnDoe");
        VRCharacterPtr ptr();

        void setSkeleton(VRSkeletonPtr s);
        VRSkeletonPtr getSkeleton();

        void addBehavior(VRBehaviorPtr b);
        void addAction(VRBehavior::ActionPtr a);

        void simpleSetup();
};

OSG_END_NAMESPACE;

#endif // VRCHARACTER_H_INCLUDED
