#ifndef VRVISUALLAYER_H_INCLUDED
#define VRVISUALLAYER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/utils/VRName.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRVisualLayer : public VRName {
    private:
        VRObjectPtr anchor;
        static map<string, VRVisualLayerWeakPtr> layers;
        VRToggleWeakPtr callback;
        string icon;

        VRVisualLayer(string name, string icon);

    public:
        ~VRVisualLayer();

        static VRVisualLayerPtr getLayer(string l, string icon = "", bool create = false);
        static vector<string> getLayers();
        static void anchorLayers(VRObjectPtr root);
        static void clearLayers();

        void setVisibility(bool b);
        bool getVisibility();

        void addObject(VRObjectPtr obj);

        void setCallback(VRToggleWeakPtr fkt);

        string getIconName();
};

OSG_END_NAMESPACE;

#endif // VRVISUALLAYER_H_INCLUDED

// init layers in scene manager
// gui dropdown list gets layer names
