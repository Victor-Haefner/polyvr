#ifndef VRKINEMATICS_H_INCLUDED
#define VRKINEMATICS_H_INCLUDED

#include <vector>
#include <OpenSG/OSGConfig.h>

#include "core/objects/VRTransform.h"
#include "core/math/VRMathFwd.h"
#include "core/math/graph.h"


using namespace std;
OSG_BEGIN_NAMESPACE;

class VRKinematics {
    public:
        struct Body {
            int ID;
            VRTransformPtr obj;
            vector<int> joints;

            Body() {};
            Body(int ID, VRTransformPtr obj) : ID(ID), obj(obj) {}
        };

        struct Joint {
            int ID;
            int body1;
            int body2;
            VRConstraintPtr c;

            Joint() {};
            Joint(int ID, int body1, int body2, VRConstraintPtr c) : ID(ID), body1(body1), body2(body2), c(c) {}
        };

    private:
        GraphPtr graph;
        map<int, Joint> joints; // map to graph nodes
        map<int, Body> bodies; // map to graph edges

        void physicalizeAll(bool dynamic);
        void physicalize(int nID, bool dynamic);

    public:
        VRKinematics();
        ~VRKinematics();

        static VRKinematicsPtr create();

        int addJoint(int nID1, int nID2, VRConstraintPtr c);
        int addBody(VRTransformPtr obj, bool dynamic = 1);
        VRTransformPtr getTransform(int nID);
        int addHinge(int nID1, int nID2, PosePtr d1, PosePtr d2, int axis, float minRange, float maxRange);
        int addBallJoint(int nID1, int nID2, PosePtr d1, PosePtr d2);
        int addFixedJoint(int nID1, int nID2, PosePtr d1, PosePtr d2);
        int addCustomJoint(int nID1, int nID2, PosePtr d1, PosePtr d2, vector<int> dofs, vector<float> minRange, vector<float> maxRange);
        GraphPtr getGraph();
        void setDynamic(int nID, bool dynamic);
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRKINEMATICS_H_INCLUDED
