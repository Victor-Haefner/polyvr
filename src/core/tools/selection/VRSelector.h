#ifndef VRSELECTOR_H_INCLUDED
#define VRSELECTOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"
#include "VRSelection.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelector {
    private:
        Vec3f color;

        map<VRGeometryPtr, VRMaterialPtr> orig_mats;
        VRMaterialPtr getMat();

        VRObjectPtr selection = 0;
        bool hasSubselection = false;
        map<int, int> subselection;

        void deselect();

    public:
        VRSelector();

        void select(VRSelection selection);
        void select(VRObjectPtr obj);
        VRObjectPtr getSelection();

        void subselect(vector<int> verts, bool add);
        void clearSubselection();
        vector<int> getSubselection();

        void setColor(Vec3f c);
};

OSG_END_NAMESPACE;

#endif // VRSELECTOR_H_INCLUDED
