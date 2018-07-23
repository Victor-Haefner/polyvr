#ifndef VRSCENELOADER_H_INCLUDED
#define VRSCENELOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

#include "core/objects/VRObjectFwd.h"

namespace xmlpp{ class Element; }

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

        typedef map<xmlpp::Element*, map<string, string> > children_attribs;

        void optimizeGraph(VRObjectPtr obj);
        VRSceneLoader();

        //parser callback for the xml scene import
        void parseScene(xmlpp::Element* node, xmlpp::Element* xmlparent, VRObjectPtr parent = 0);

    public:
        static VRSceneLoader* get();
        ~VRSceneLoader();

        void saveScene(string file, xmlpp::Element* guiN = 0, string encryption = "");
        void loadScene(string file);
};

OSG_END_NAMESPACE;

#endif // VRSCENELOADER_H_INCLUDED
