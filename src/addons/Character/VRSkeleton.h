#ifndef VRSKELETON_H_INCLUDED
#define VRSKELETON_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/math/pose.h"
#include "VRCharacterFwd.h"

#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGQuaternion.h>

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

        struct Bone {
            Vec3d p1;
            Vec3d p2;
            Vec3d dir;
            Vec3d up;
            float length = 0;
            size_t ID = 0;
            bool isStart = false;
            bool isEnd = false;
        };

        typedef shared_ptr<Configuration> ConfigurationPtr;

    private:
        FABRIKPtr fabrik;
        map<string, int> joints;
        map<string, PosePtr> targets;

    public:
        VRSkeleton();
        ~VRSkeleton();

        static VRSkeletonPtr create();

        FABRIKPtr getKinematics();

        int addJoint(string name, PosePtr p);
        void addChain(string name, vector<int> jIDs);
        void addConstraint(string name, Vec4d angles);
        void addTarget(string name, PosePtr p);

        PosePtr getTarget(string name);
        int getJointID(string name);
        vector<Bone> getBones();

        void clear();
        void setupSimpleHumanoid();

        void asGeometry(VRGeoData& data);
        void setupGeometry();
        void updateGeometry();

        void resolveKinematics();
};

OSG_END_NAMESPACE;


#endif // VRSKELETON_H_INCLUDED
