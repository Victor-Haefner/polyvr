#ifndef VRFABRIK_H_INCLUDED
#define VRFABRIK_H_INCLUDED

#include <map>

#include "core/utils/VRFwdDeclTemplate.h"
#include "core/math/pose.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

ptrFwd(FABRIK);

class FABRIK {
    private:
        struct Joint {
            int ID;
            string name;
            PosePtr p;
            vector<int> in;
            vector<int> out;
            PosePtr target;
            bool constrained = false;
            Vec4d constraintAngles;
        };

        struct Chain {
            string name;
            vector<int> joints;
            vector<float> distances;
        };

        struct step {
            int joint;
            int base;
            int i1;
            int i2;
            string chain;
            PosePtr target;
            bool fwd = false;
            bool mid = false;

            step(int j, int b, int i1, int i2, string c, PosePtr t, bool f, bool m) : joint(j), base(b), i1(i1), i2(i2), chain(c), target(t), fwd(f), mid(m) {};
        };

        map<int, Joint> joints;
        map<string, Chain> chains;
        vector<step> executionQueue;

        float tolerance = 0.01;

        Vec3d movePointTowards(int j, Vec3d target, float t);
        Vec3d moveToDistance(int j1, int j2, float d);
        void updateExecutionQueue();

    public:
        FABRIK();
        ~FABRIK();

        static FABRIKPtr create();

        void addJoint(int ID, PosePtr p);
        PosePtr getJointPose(int ID);

        void addChain(string name, vector<int> joints);
        vector<int> getChainJoints(string name);

        void addConstraint(int j, Vec4d angles);

        void forward(Chain& chain);
        void backward(Chain& chain);
        void chainIteration(Chain& chain);

        void setTarget(int i, PosePtr p);
        void iterate();
        void iterateChain(string chain);

        void visualize(VRGeometryPtr geo);
};

OSG_END_NAMESPACE;

#endif // VRFABRIK_H_INCLUDED
