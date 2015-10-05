#ifndef VRSELECTION_H_INCLUDED
#define VRSELECTION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelection {
    public:
        struct selection_atom {
            VRGeometryWeakPtr geo;
            bool partial = false;
            vector<int> subselection;
        };

    protected:
        map<VRGeometry*, selection_atom> selected;

        virtual bool vertSelected(Vec3f p);
        virtual bool objSelected(VRGeometryPtr geo);
        virtual bool partialSelected(VRGeometryPtr geo);

    public:
        VRSelection();

        static shared_ptr<VRSelection> create();

        void add(VRGeometryPtr geo, vector<int> subselection = vector<int>());
        void apply(VRObjectPtr tree);
        void clear();

        vector<VRGeometryWeakPtr> getPartials();
        vector<VRGeometryWeakPtr> getSelected();
        vector<int> getSubselection(VRGeometryPtr geo);
};

OSG_END_NAMESPACE;

#endif // VRSELECTION_H_INCLUDED
