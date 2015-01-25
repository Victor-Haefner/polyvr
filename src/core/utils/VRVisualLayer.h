#ifndef VRVISUALLAYER_H_INCLUDED
#define VRVISUALLAYER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;

class VRVisualLayer {
    private:
        VRObject* anchor;
        static map<string, VRVisualLayer*> layers;
        VRFunction<bool>* callback = 0;

    public:
        VRVisualLayer(string name);
        ~VRVisualLayer();

        static vector<string> getLayers();
        static VRVisualLayer* getLayer(string l);
        static void anchorLayers(VRObject* root);
        static void clearLayers();

        void setVisibility(bool b);
        bool getVisibility();

        void addObject(VRObject* obj);

        void setCallback(VRFunction<bool>* fkt);
};

OSG_END_NAMESPACE;

#endif // VRVISUALLAYER_H_INCLUDED

// init layers in scene manager
// gui dropdown list gets layer names
