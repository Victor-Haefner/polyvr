#ifndef VRSCENEMANAGER_H_INCLUDED
#define VRSCENEMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRThreadManager.h"
#include "VRCallbackManager.h"
#include "core/networking/VRNetworkManager.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;

void glutUpdate();

class VRSceneManager : public VRThreadManager, public VRCallbackManager, public VRNetworkManager {
    private:
        vector<string> favorite_paths;
        vector<string> example_paths;
        map<string, VRScene*> scenes;
        string active;
        string original_workdir;

        VRSceneManager();
        void operator= (VRSceneManager v);

        void searchExercisesAndFavorites();

    public:
        static VRSceneManager* get();
        ~VRSceneManager();

        void addScene(VRScene* s);
        void loadScene(string path, bool write_protected = false);
        void removeScene(VRScene* s);
        void newScene(string name);

        void setActiveScene(VRScene* s);
        void setActiveSceneByName(string s);

        void setWorkdir(string path);
        string getOriginalWorkdir();
        void storeFavorites();
        void addFavorite(string path);
        void remFavorite(string path);

        vector<string> getFavoritePaths();
        vector<string> getExamplePaths();

        //void printTree() { scenes[active]->printTree();}

        int getSceneNum();
        VRScene* getScene(string s);
        static VRScene* getCurrent();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSCENEMANAGER_H_INCLUDED
