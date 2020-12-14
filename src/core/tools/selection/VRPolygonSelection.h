#ifndef VRPOLYGONSELECTION_H_INCLUDED
#define VRPOLYGONSELECTION_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <string>
#include "VRSelection.h"
#include "core/math/frustum.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPolygonSelection : public VRSelection {
    private:
        Pose origin;
        Frustum selection;
        Frustum convex_hull;
        vector<Frustum> convex_decomposition;
        bool closed = false;
        VRGeometryPtr shape;

        bool vertSelected(Vec3d p);
        bool objSelected(VRGeometryPtr geo);
        bool partialSelected(VRGeometryPtr geo);

        void updateShape(Frustum f);

    public:
        VRPolygonSelection();
        void clear();

        static shared_ptr<VRPolygonSelection> create();

        void setOrigin(Pose orig);
        void addEdge(Vec3d dir);
        void close(VRObjectPtr world);

        bool isClosed();
        VRGeometryPtr getShape();

        Frustum getSelectionFrustum();
};

OSG_END_NAMESPACE;

#endif // VRPOLYGONSELECTION_H_INCLUDED
