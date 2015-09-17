#ifndef VRVISUALLAYER_H_INCLUDED
#define VRVISUALLAYER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRName.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;

class VRVisualLayer : public VRName {
    private:
        VRObject* anchor;
        static map<string, VRVisualLayer*> layers;
        VRToggleWeakPtr callback;
        string icon;

    public:
        VRVisualLayer(string name, string icon);
        ~VRVisualLayer();

        static vector<string> getLayers();
        static VRVisualLayer* getLayer(string l);
        static void anchorLayers(VRObject* root);
        static void clearLayers();

        void setVisibility(bool b);
        bool getVisibility();

        void addObject(VRObject* obj);

        void setCallback(VRToggleWeakPtr fkt);

        string getIconName();
};

OSG_END_NAMESPACE;

#endif // VRVISUALLAYER_H_INCLUDED

// init layers in scene manager
// gui dropdown list gets layer names
