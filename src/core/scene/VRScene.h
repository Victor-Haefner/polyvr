#ifndef VRSCENE_H_INCLUDED
#define VRSCENE_H_INCLUDED

#include "VRObjectManager.h"
#include "VRCameraManager.h"
#include "VRAnimationManager.h"
#include "VRPhysicsManager.h"
#include "VRCallbackManager.h"
#include "VRRenderManager.h"
#include "VRMaterialManager.h"
#include "VRThreadManager.h"
#include "core/scripting/VRScriptManager.h"
#include "core/navigation/VRNavigator.h"
#include "core/networking/VRNetworkManager.h"
#include "core/utils/VRName.h"
#include "core/utils/VRFlags.h"
#include "VRBackground.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRVisualLayer;

class VRScene : public VRObjectManager,
                public VRCameraManager,
                public VRAnimationManager,
                public VRPhysicsManager,
                public VRCallbackManager,
                public VRThreadManager,
                public VRScriptManager,
                public VRNavigator,
                public VRNetworkManager,
                public VRMaterialManager,
                public VRBackground,
                public VRRenderManager,
                public VRFlags,
                public VRName {
    private:
        string path;
        string icon;
        //physics run in own thread
        int physicsThreadID;

        VRVisualLayer* referentials_layer = 0;
        VRVisualLayer* cameras_layer = 0;
        VRVisualLayer* lights_layer = 0;

        VRTogglePtr layer_ref_toggle;
        VRTogglePtr layer_cam_toggle;
        VRTogglePtr layer_light_toggle;

    public:
        VRScene();
        ~VRScene();

        void setPath(string path);
        void setIcon(string icon);
        string getPath();
        string getWorkdir();
        string getFile();
        string getFileName();
        string getIcon();

        void initDevices();

        void add(VRObjectPtr obj, int parentID = -1);
        void add(NodeMTRecPtr n);

        VRObjectPtr get(int ID);
        VRObjectPtr get(string name);
        VRObjectPtr getRoot();
        VRObjectPtr getSystemRoot();

        void setActiveCamera(string name = "");

        void printTree();
        void showReferentials(bool b, VRObjectPtr o);
        void showLights(bool b);
        void showCameras(bool b);

        void update();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

typedef std::shared_ptr<VRScene> VRScenePtr;
typedef std::weak_ptr<VRScene> VRSceneWeakPtr;

OSG_END_NAMESPACE;

#endif // VRSCENE_H_INCLUDED
