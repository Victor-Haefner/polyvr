#ifndef VRSCENE_H_INCLUDED
#define VRSCENE_H_INCLUDED

#include "VRObjectManager.h"
#include "VRLightManager.h"
#include "VRCameraManager.h"
#include "VRAnimationManager.h"
#include "VRPhysicsManager.h"
#include "VRCallbackManager.h"
#include "VRRenderManager.h"
#include "core/scripting/VRScriptManager.h"
#include "core/navigation/VRNavigator.h"
#include "core/networking/VRNetworkManager.h"
#include "core/utils/VRName.h"
#include "VRBackground.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;

class VRScene : public VRObjectManager,
                public VRLightManager,
                public VRCameraManager,
                public VRAnimationManager,
                public VRPhysicsManager,
                public VRCallbackManager,
                public VRScriptManager,
                public VRNavigator,
                public VRNetworkManager,
                public VRName,
                public VRBackground,
                public VRRenderManager {
    private:
        string path;
        string icon;

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

        void add(VRObject* obj, int parentID = -1);
        void add(NodeRecPtr n);

        VRObject* get(int ID);
        VRObject* get(string name);
        VRObject* getRoot();

        void setActiveCamera(int i);

        void printTree();

        void showReferentials(bool b, VRObject* o = 0);
        void showLightsCameras(bool b);

        void update();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VRSCENE_H_INCLUDED
