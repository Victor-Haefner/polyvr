#ifndef VRROBOTARM_H_INCLUDED
#define VRROBOTARM_H_INCLUDED

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTransform;
class VRAnimation;

class VRRobotArm {
    private:
        VRAnimation* anim = 0;

        int N = 5;
        float grab = 1;

        vector<VRTransform*> parts;
        vector<float> angles;
        vector<float> angle_offsets;
        vector<float> lengths;
        vector<int> axis;

        void applyAngles();
        void calcReverseKinematics(Vec3f pos, Vec3f dir);

    public:
        VRRobotArm();

        void setParts(vector<VRTransform*> parts);
        void setAngleOffsets(vector<float> offsets);
        void setAxis(vector<int> axis);
        void setLengths(vector<float> lengths);

        void moveTo(Vec3f pos, Vec3f dir);
        void setGrab(float g);
        void toggleGrab();
};

OSG_END_NAMESPACE;

#endif // VRROBOTARM_H_INCLUDED
