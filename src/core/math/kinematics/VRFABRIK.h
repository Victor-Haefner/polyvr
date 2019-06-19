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
        };

        struct Chain {
            string name;
            vector<int> joints;
            vector<float> distances;
        };

        map<int, Joint> joints;
        map<string, Chain> chains;

        float tolerance = 0.01;

        Vec3d movePointTowards(Chain& chain, int i, Vec3d target, float t);
        Vec3d moveToDistance(Chain& chain, int i1, int i2, int dID);

    public:
        FABRIK();
        ~FABRIK();

        static FABRIKPtr create();

        void addJoint(int ID, PosePtr p, vector<int> in, vector<int> out);
        PosePtr getJointPose(int ID);

        void addChain(string name, vector<int> joints);
        vector<int> getChainJoints(string name);

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
