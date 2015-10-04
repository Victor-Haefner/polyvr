#ifndef VRSELECTION_H_INCLUDED
#define VRSELECTION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <vector>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelection {
    private:
        vector<VRGeometryWeakPtr> partials;
        vector<VRGeometryWeakPtr> selected;

        virtual bool vertSelected(Vec3f p);
        virtual bool objSelected(VRGeometryPtr geo);
        virtual bool partialSelected(VRGeometryPtr geo);

    public:
        VRSelection();

        void add(VRGeometryPtr geo);
        void apply(VRObjectPtr tree);
        void clear();

        vector<VRGeometryWeakPtr> getPartials();
        vector<VRGeometryWeakPtr> getSelected();
        vector<int> getSelectedVertices(VRGeometryPtr geo);
};

OSG_END_NAMESPACE;

#endif // VRSELECTION_H_INCLUDED
