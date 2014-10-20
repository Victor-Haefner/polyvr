#ifndef BASEWORLDOBJECT_H
#define BASEWORLDOBJECT_H

namespace OSG { class VRGeometry; }
using namespace std;

namespace realworld {
    class BaseWorldObject {
        public:
            VRGeometry* mesh;
    };
}

#endif // BASEWORLDOBJECT_H

