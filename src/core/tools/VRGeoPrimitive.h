#ifndef VRGEOPRIMITIVE_H_INCLUDED
#define VRGEOPRIMITIVE_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/tools/selection/VRSelectionFwd.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeoPrimitive : public VRGeometry {
    private:
        bool selected = false;

        vector<VRHandlePtr> handles;
        VRSelectorPtr selector;
        VRAnalyticGeometryPtr params_geo;

        void update(int i, float v);
        void setupHandles();

    public:
        VRGeoPrimitive(string name);

        static VRGeoPrimitivePtr create(string name = "None");
        VRGeoPrimitivePtr ptr();

        vector<VRHandlePtr> getHandles();

        void select(bool b); // activates editing handles

        void setPrimitive(string primitive, string args = ""); // hook on virtual function VRGeometry::setPrimitive
        VRAnalyticGeometryPtr getLabels();
};

OSG_END_NAMESPACE;

#endif // VRGEOPRIMITIVE_H_INCLUDED
