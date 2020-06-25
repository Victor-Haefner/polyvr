#ifndef VRSCENELOADER_H_INCLUDED
#define VRSCENELOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;

/**
    The class VRSceneLoader loads the scene elements parsing a XML file. How to write proper XML files see ....
     You can load scene using the function parseSceneFromXML with parameters the path to XML document && the name of the new scene.
            VRSceneLoader::get()->parseSceneFromXML("scene_file_path.xml", "scene_name");
*/

class VRSceneLoader {

    private:
        VRScene* scene;
        map<string, string> attr;
        vector<string> fileFilter;

        typedef map<XMLElementPtr, map<string, string> > children_attribs;

        void optimizeGraph(VRObjectPtr obj);
        VRSceneLoader();

        //parser callback for the xml scene import
        void parseScene(XMLElementPtr node, XMLElementPtr xmlparent, VRObjectPtr parent = 0);

    public:
        static VRSceneLoader* get();
        ~VRSceneLoader();

        void saveScene(string file, XMLElementPtr guiN = 0, string encryption = "");
        bool loadScene(string file, string encryptionKey = "");
        VRObjectPtr importScene(string file, string encryptionKey = "", bool offLights = false);
};

OSG_END_NAMESPACE;

#endif // VRSCENELOADER_H_INCLUDED
