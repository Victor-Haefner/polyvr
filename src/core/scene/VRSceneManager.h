#ifndef VRSCENEMANAGER_H_INCLUDED
#define VRSCENEMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRSceneFwd.h"
#include "VRThreadManager.h"
#include "VRCallbackManager.h"
#include "core/networking/VRNetworkManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void glutUpdate();

class VRSceneManager : public VRThreadManager, public VRCallbackManager, public VRNetworkManager {
    private:
        vector<string> favorite_paths;
        vector<string> example_paths;
        VRScenePtr current;
        string original_workdir;
        VRSignalPtr on_scene_load = 0;
        VRSignalPtr on_scene_close = 0;
        VRThreadCbPtr sceneUpdateCb;

        VRSceneManager();
        void operator= (VRSceneManager v);

        void searchExercisesAndFavorites();

    public:
        static VRSceneManager* get();
        ~VRSceneManager();

        void setScene(VRScenePtr s);
        void newEmptyScene(string name);
        void newScene(string name);
        void closeScene();
        void loadScene(string path, bool write_protected = false);

        void setWorkdir(string path);
        string getOriginalWorkdir();
        void storeFavorites();
        void addFavorite(string path);
        void remFavorite(string path);

        VRSignalPtr getSignal_on_scene_load();
        VRSignalPtr getSignal_on_scene_close();

        vector<string> getFavoritePaths();
        vector<string> getExamplePaths();

        VRScenePtr getCurrent();

        void updateSceneThread(VRThreadWeakPtr tw);
        void updateScene();
        void update();
};

OSG_END_NAMESPACE;

#endif // VRSCENEMANAGER_H_INCLUDED
