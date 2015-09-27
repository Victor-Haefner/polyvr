#ifndef BASEWORLDOBJECT_H
#define BASEWORLDOBJECT_H

namespace OSG { class VRGeometry; }
using namespace std;

namespace realworld {
    class BaseWorldObject {
        public:
            VRGeometryPtr mesh;
    };
}

#endif // BASEWORLDOBJECT_H

