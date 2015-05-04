#ifndef VRSCENELOADER_H_INCLUDED
#define VRSCENELOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;
class VRObject;
class VRTransform;

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
        bool ihr_flag = false; // ignore heavy ressources

        typedef map<xmlpp::Element*, map<string, string> > children_attribs;

        void optimizeGraph(VRObject* obj);
        int fileSize(string path);

        VRSceneLoader();

        //parser callback for the xml scene import
        void parseScene(xmlpp::Element* node, xmlpp::Element* xmlparent, VRObject* parent = 0);

    public:
        static VRSceneLoader* get();
        ~VRSceneLoader();

        VRTransform* load3DContent(string filename, VRObject* parent = 0, bool reload = false);

        void ingoreHeavyRessources();

        void saveScene(string file, xmlpp::Element* guiN = 0);
        void loadScene(string file);

};

OSG_END_NAMESPACE;

#endif // VRSCENELOADER_H_INCLUDED
