#ifndef VRSCENEMANAGER_H_INCLUDED
#define VRSCENEMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRSceneFwd.h"
#include "VRThreadManager.h"
#include "VRCallbackManager.h"
#include "core/networking/VRNetworkManager.h"
#include "PolyVRFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void glutUpdate();

class VRSceneManager : public VRThreadManager, public VRCallbackManager, public VRNetworkManager {
    private:
        VRProjectsListPtr examples;
        VRProjectsListPtr projects;

        VRScenePtr current;
        string original_workdir;
        VRSignalPtr on_scene_load = 0;
        VRSignalPtr on_scene_close = 0;
        VRThreadCbPtr sceneUpdateCb;

        double targetFPS = 60.0;

        void searchExercisesAndFavorites();

    public:
        VRSceneManager();
        ~VRSceneManager();

        static VRSceneManagerPtr create();
        static VRSceneManager* get();

        void setScene(VRScenePtr s);
        void newEmptyScene(string name);
        void newScene(string name);
        void closeScene();
        void reloadScene();
        void loadScene(string path, bool write_protected = false, string encryptionKey = "");

        void setWorkdir(string path);
        string getOriginalWorkdir();
        void storeFavorites();
        void addFavorite(string path);
        void remFavorite(string path);

        VRSignalPtr getSignal_on_scene_load();
        VRSignalPtr getSignal_on_scene_close();

        VRProjectsListPtr getFavoritePaths();
        VRProjectsListPtr getExamplePaths();

        VRScenePtr getCurrent();

        void updateSceneThread(VRThreadWeakPtr tw);
        void updateScene();
        void update();

        void setTargetFPS(double fps);
};

OSG_END_NAMESPACE;

#endif // VRSCENEMANAGER_H_INCLUDED
