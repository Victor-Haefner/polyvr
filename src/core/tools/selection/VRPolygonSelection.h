#ifndef VRPOLYGONSELECTION_H_INCLUDED
#define VRPOLYGONSELECTION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <string>
#include "VRSelection.h"
#include "core/math/frustum.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPolygonSelection : public VRSelection {
    private:
        pose origin;
        frustum selection;
        frustum convex_hull;
        vector<frustum> convex_decomposition;
        bool closed = false;
        VRGeometryPtr shape;

        bool vertSelected(Vec3d p);
        bool objSelected(VRGeometryPtr geo);
        bool partialSelected(VRGeometryPtr geo);

        void updateShape(frustum f);

    public:
        VRPolygonSelection();
        void clear();

        static shared_ptr<VRPolygonSelection> create();

        void setOrigin(pose orig);
        void addEdge(Vec3d dir);
        void close(VRObjectPtr world);

        bool isClosed();
        VRGeometryPtr getShape();
};

OSG_END_NAMESPACE;

#endif // VRPOLYGONSELECTION_H_INCLUDED
