#ifndef VRGEODATA_H_INCLUDED
#define VRGEODATA_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGeoData {
    private:
        struct Data;
        shared_ptr<Data> data;

        void updateType(int t, int N);

    public:
        VRGeoData();

        void reset();
        bool valid();

        int pushVert(Pnt3f p, Vec3f n);
        void pushQuad(int i, int j, int k, int l);
        void pushTri(int i, int j, int k);

        void apply(VRGeometryPtr geo);
        VRGeometryPtr asGeometry(string name);
};

OSG_END_NAMESPACE;

#endif // VRGEODATA_H_INCLUDED
