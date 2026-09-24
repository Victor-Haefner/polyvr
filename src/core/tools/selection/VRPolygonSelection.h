#ifndef VRPOLYGONSELECTION_H_INCLUDED
#define VRPOLYGONSELECTION_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include "core/tools/selection/VRSelectionFwd.h"
#include <string>
#include "VRSelection.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPolygonSelection : public VRSelection {
    private:
        PosePtr origin;
        FrustumPtr selection;
        FrustumPtr convex_hull;
        vector<FrustumPtr> convex_decomposition;
        bool closed = false;
        VRGeometryPtr shape;

        bool vertSelected(Vec3d p) override;
        bool objSelected(VRGeometryPtr geo) override;
        bool partialSelected(VRGeometryPtr geo) override;

        void updateShape(FrustumPtr f);

    public:
        VRPolygonSelection();
        void clear();

        static VRPolygonSelectionPtr create();

        void setOrigin(PosePtr orig);
        void addEdge(Vec3d dir);
        void close(VRObjectPtr world);

        bool isClosed();
        VRGeometryPtr getShape();

        FrustumPtr getSelectionFrustum();
};

OSG_END_NAMESPACE;

#endif // VRPOLYGONSELECTION_H_INCLUDED
