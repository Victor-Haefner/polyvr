#ifndef VRKINEMATICS_H_INCLUDED
#define VRKINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "VRConstraint.h"
#include "core/math/VRMathFwd.h"
#include "core/math/graph.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRKinematics {
    public:
        struct Body {
            ;
        };

    private:
        Graph graph;
        map<int, VRConstraint> joints; // map to graph nodes
        map<int, Body> bodies; // map to graph edges

    public:
        VRKinematics();
        ~VRKinematics();

        static VRKinematicsPtr create();
};

OSG_END_NAMESPACE;

#endif // VRKINEMATICS_H_INCLUDED
