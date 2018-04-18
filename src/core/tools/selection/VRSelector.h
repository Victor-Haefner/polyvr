#ifndef VRSELECTOR_H_INCLUDED
#define VRSELECTOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"
#include "VRSelection.h"
#include "VRSelectionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelector {
    public:
        enum VISUAL {
            OUTLINE = 0,
            OVERLAY = 1
        };

    private:
        Color3f color = Color3f(0.2, 0.65, 0.9);
        float transparency = 1;
        int width = 5;
        bool smooth = true;
        VISUAL visual = OUTLINE;

        struct MatStore {
            VRGeometryWeakPtr geo;
            VRMaterialPtr mat;
            MatStore(VRGeometryPtr geo);
        };
        vector<MatStore> orig_mats;
        VRObjectPtr selected;
        VRSelectionPtr selection;
        VRGeometryPtr subselection;

        void deselect();
        VRMaterialPtr getMat();

    public:
        VRSelector();

        static VRSelectorPtr create();

        void select(VRObjectPtr obj, bool append = false, bool recursive = true);
        VRObjectPtr getSelected();
        void set(VRSelectionPtr s);
        void add(VRSelectionPtr s);
        VRSelectionPtr getSelection();
        void update();
        void clear();

        void setVisual(VISUAL v);
        void setColor(Color3f c, float t = 1);
        void setBorder(int width, bool smooth = true);
};

OSG_END_NAMESPACE;

#endif // VRSELECTOR_H_INCLUDED
