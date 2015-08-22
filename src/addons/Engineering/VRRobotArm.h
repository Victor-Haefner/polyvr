#ifndef VRROBOTARM_H_INCLUDED
#define VRROBOTARM_H_INCLUDED

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTransform;
class VRAnimation;
class VRAnalyticGeometry;
class path;

class VRRobotArm {
    private:
        struct job {
            path* p = 0;
            float t0 = 0;
            float t1 = 1;
            bool loop = false;
            float d = 1;
            job(path* p, float t0 = 0, float t1 = 1, float d = 1, bool loop = false) : p(p), t0(t0), t1(t1), loop(loop), d(d) {;}
        };

        VRAnalyticGeometry* ageo = 0;
        VRAnimation* anim = 0;
        path* animPath = 0;
        path* robotPath = 0;

        list<job> job_queue;

        int N = 5;
        float grab = 0;
        float pathPos = 0;

        vector<VRTransform*> parts;
        vector<float> angles;
        vector<float> angle_offsets;
        vector<int> angle_directions;
        vector<float> lengths;
        vector<int> axis;

        void applyAngles();
        void calcReverseKinematics(Vec3f pos, Vec3f dir, Vec3f up);
        void animOnPath(float t);
        void addJob(job j);

    public:
        VRRobotArm();

        void setParts(vector<VRTransform*> parts);
        void setAngleOffsets(vector<float> offsets);
        void setAngleDirections(vector<int> directions);
        void setAxis(vector<int> axis);
        void setLengths(vector<float> lengths);

        vector<float> getAngles();
        void getPose(Vec3f& pos, Vec3f& dir, Vec3f& up);

        void moveTo(Vec3f pos, Vec3f dir, Vec3f up);
        void setAngles(vector<float> angles);
        void setGrab(float g);
        void toggleGrab();

        void setPath(path* p);
        path* getPath();
        void moveOnPath(float t0, float t1, bool loop = false);
};

OSG_END_NAMESPACE;

#endif // VRROBOTARM_H_INCLUDED
