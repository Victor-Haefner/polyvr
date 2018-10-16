#ifndef VRGEOPRIMITIVE_H_INCLUDED
#define VRGEOPRIMITIVE_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/tools/selection/VRSelectionFwd.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeoPrimitive : public VRTransform {
    private:
        bool selected = false;
        bool ownsGeometry = false;
        float size = 0.1;

        vector<VRHandlePtr> handles;
        VRSelectorPtr selector;
        VRGeometryWeakPtr geometry;
        VRAnnotationEnginePtr params_geo;

        void update(int i, VRHandleWeakPtr hw, float v);
        void setupHandles();

    public:
        VRGeoPrimitive(string name);

        static VRGeoPrimitivePtr create(string name = "None");
        VRGeoPrimitivePtr ptr();

        VRHandlePtr getHandle(int i);
        vector<VRHandlePtr> getHandles();

        void select(bool b); // activates editing handles
        void setHandleSize(float size);

        void setGeometry(VRGeometryPtr geo);
        void setPrimitive(string params);
        VRAnnotationEnginePtr getLabels();
};

OSG_END_NAMESPACE;

#endif // VRGEOPRIMITIVE_H_INCLUDED
