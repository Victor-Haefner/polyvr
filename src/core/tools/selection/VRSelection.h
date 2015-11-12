#ifndef VRSELECTION_H_INCLUDED
#define VRSELECTION_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>
#include "core/objects/VRObjectFwd.h"
#include "core/math/boundingbox.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelection;
typedef shared_ptr<VRSelection> VRSelectionPtr;

class VRSelection {
    public:
        struct selection_atom {
            VRGeometryWeakPtr geo;
            bool partial = false;
            vector<int> subselection;
        };

    protected:
        map<VRGeometry*, selection_atom> selected;
        boundingbox bbox;

        virtual bool vertSelected(Vec3f p);
        virtual bool objSelected(VRGeometryPtr geo);
        virtual bool partialSelected(VRGeometryPtr geo);

        void updateSubselection();
        void updateSubselection(VRGeometryPtr geo);

    public:
        VRSelection();

        static VRSelectionPtr create();

        void add(VRGeometryPtr geo, vector<int> subselection = vector<int>());
        void apply(VRObjectPtr tree, bool force = false);
        void append(VRSelectionPtr sel);
        void clear();

        vector<VRGeometryWeakPtr> getPartials();
        vector<VRGeometryWeakPtr> getSelected();
        vector<int> getSubselection(VRGeometryPtr geo);
        map< VRGeometryPtr, vector<int> > getSubselections();
};

OSG_END_NAMESPACE;

#endif // VRSELECTION_H_INCLUDED
