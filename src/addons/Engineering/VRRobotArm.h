#ifndef VRROBOTARM_H_INCLUDED
#define VRROBOTARM_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/math/VRMathFwd.h"

#include <list>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRRobotArm {
    public:
        struct System {
            VRTransformPtr base;
            vector<VRTransformPtr> parts;
            VRAnalyticGeometryPtr ageo;

            vector<float> angles;
            vector<float> angle_targets;
            vector<float> angle_offsets = {0,0,-0.5,0,0,0};
            vector<int> angle_directions = {1,-1,1,1,1,1};
            vector<float> lengths = {0,0,0,0,0};
            vector<float> axis_offsets = {0,0};
            vector<int> axis = {1,0,0,2,0,2};

            System();
            virtual ~System();

            virtual void updateState() = 0;
            virtual void updateSystem() = 0;
            virtual void genKinematics() = 0;
            virtual vector<float> calcReverseKinematics(PosePtr p) = 0;
            virtual PosePtr calcForwardKinematics(vector<float> angles) = 0;
            virtual void updateAnalytics() = 0;
            virtual void applyAngles() = 0;

            double convertAngle(double a, int i);
        };

        struct job {
            PathPtr p = 0;
            PathPtr po = 0;
            float t0 = 0;
            float t1 = 1;
            float d = 1;
            bool loop = false;
            bool local = false;
            job(PathPtr p, PathPtr po = 0, float t0 = 0, float t1 = 1, float d = 1, bool loop = false, bool local = false) : p(p), po(po), t0(t0), t1(t1), d(d), loop(loop), local(local) {;}
        };

    private:
        shared_ptr<System> system;
        VRAnimationPtr anim = 0;
        VRAnimCbPtr animPtr;
        VRUpdateCbPtr updatePtr;
        PathPtr animPath = 0;
        PathPtr robotPath = 0;
        PathPtr orientationPath = 0;
        PosePtr lastPose = 0;
        PosePtr gToL = 0;
        PosePtr lToG = 0;
        VRMessageCbPtr eventCb = 0;

        list<job> job_queue;

        int N = 6;
        float grabDist = 0;
        float pathPos = 0;
        bool showModel = false;
        bool moving = false;
        float animSpeed = 1;
        float maxSpeed = 0.01;
        string type = "kuka";

        VRTransformPtr dragged = 0;

        void update();
        PosePtr getLastPose();
        void applyAngles();
        void updateLGTransforms();
        PosePtr calcForwardKinematics(vector<float> angles);
        vector<float> calcReverseKinematics(PosePtr p);
        void animOnPath(float t);
        void addJob(job j);

    public:
        VRRobotArm(string type);
        ~VRRobotArm();

        static shared_ptr<VRRobotArm> create(string type);
        void showAnalytics(bool b);

        void setAngleOffsets(vector<float> offsets);
        void setAngleDirections(vector<int> directions);
        void setAxis(vector<int> axis);
        void setLengths(vector<float> lengths);
        void setAxisOffsets(vector<float> offsets);
        void setSpeed(float s);
        void setMaxSpeed(float s);
        VRTransformPtr getKinematicBase();
        vector<VRTransformPtr> getKinematics();

        vector<float> getAngles();
        vector<float> getTargetAngles();
        PosePtr getPose();

        void move();
        void pause();
        void stop();
        bool isMoving();
        void setEventCallback(VRMessageCbPtr mCb);

        bool canReach(PosePtr p, bool local = false);
        void moveTo(PosePtr p, bool local = false);
        void setAngles(vector<float> angles, bool force = false);
        void setGrab(float g);
        void toggleGrab();

        void grab(VRTransformPtr obj);
        VRTransformPtr drop();

        void setPath(PathPtr p, PathPtr po = 0);
        PathPtr getPath();
        PathPtr getOrientationPath();
        void moveOnPath(float t0, float t1, bool loop = false, float durationMultiplier = 1, bool local = false);
};

typedef shared_ptr<VRRobotArm> VRRobotArmPtr;

OSG_END_NAMESPACE;

#endif // VRROBOTARM_H_INCLUDED
