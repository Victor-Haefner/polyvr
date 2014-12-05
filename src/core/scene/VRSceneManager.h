#ifndef VRSCENEMANAGER_H_INCLUDED
#define VRSCENEMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRThreadManager.h"
#include "VRCallbackManager.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;

void glutUpdate();

class VRSceneManager : public VRThreadManager, public VRCallbackManager {
    private:
        map<string, VRScene*> scenes;
        string active;
        string original_workdir;

        VRSceneManager();
        void operator= (VRSceneManager v);

    public:
        static VRSceneManager* get();
        ~VRSceneManager();

        void addScene(VRScene* s);
        void removeScene(VRScene* s);
        void newScene(string name);

        void setActiveScene(VRScene* s);
        void setActiveSceneByName(string s);

        void setWorkdir(string path);
        string getOriginalWorkdir();

        //void printTree() { scenes[active]->printTree();}

        int getSceneNum();
        VRScene* getScene(string s);
        static VRScene* getCurrent();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSCENEMANAGER_H_INCLUDED
