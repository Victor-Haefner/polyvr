#ifndef VRSKELETON_H_INCLUDED
#define VRSKELETON_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/math/pose.h"
#include "VRCharacterFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSkeleton : public VRGeometry {
    public:
        struct Configuration : public VRName {
            map<int,Vec3d> joints;

            Configuration(string name);
            ~Configuration();

            static shared_ptr<Configuration> create(string name);

            void setPose(int i, Vec3d p);

            Configuration interpolate(Configuration& other);
        };

        typedef shared_ptr<Configuration> ConfigurationPtr;

    private:

        struct Bone {
            Pose pose;
            float length;
        };

        struct Joint {
            Vec3d pos;
            int bone1;
            int bone2;
            VRConstraintPtr constraint;
        };

        struct EndEffector {
            string name;
            int ID = -1;
            PosePtr target;
        };

        struct ChainData {
            vector<int> chainedBones;
            vector<int> joints;
            vector<float> d;
            Vec3d targetPos;
            float Dtarget;
        };

        struct SystemData {
            int bone;
            vector<int> joints;
            map<int,map<int,float>> d;
        };

        GraphPtr armature;
        map<int, Bone > bones;
        map<int, Joint> joints;
        int rootBone = -1;
        map<string, EndEffector> endEffectors;

        VRUpdateCbPtr simCB; // sim override

        VRGeometryPtr jointsGeo;

        void initMaterial(); // skeleton visualisation
        void updateJointPositions();
        vector<int> getBoneJoints(int bone);
        Vec3d& jointPos(int j);

        void simStep(map<string, ChainData>& ChainDataMap);
        void resolveKinematics();

    public:
        VRSkeleton();
        ~VRSkeleton();

        static VRSkeletonPtr create();

        int addBone(PosePtr pose, float length);
        int addJoint(int bone1, int bone2, VRConstraintPtr constraint);
        void setEndEffector(string label, int bone);
        void setRootBone(int bone);

        map<int, Vec3d> getJointsPositions();

        void clear();
        void setupSimpleHumanoid();

        void asGeometry(VRGeoData& data);
        void setupGeometry();
        void updateGeometry();

        void move(string endEffector, PosePtr pose);

        void overrideSim(VRUpdateCbPtr cb);
};

OSG_END_NAMESPACE;


#endif // VRSKELETON_H_INCLUDED
