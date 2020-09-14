#include "VRExport.h"
#include "VRPLY.h"
#include "GLTF/GLTF.h"
#ifndef WITHOUT_DWG
#include "VRDWG.h"
#endif
//#include "VRCOLLADA.h"
//#include "VRSTEP.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include "core/objects/object/VRObject.h"
#include "core/objects/OSGObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/system/VRSystem.h"

using namespace OSG;


VRExport::VRExport() {}

VRExport* VRExport::get() { static VRExport* s = new VRExport(); return s; }

void VRExport::write(VRObjectPtr obj, string path) {
    if (!obj) return;

    string ext = getFileExtension(path);
    cout << "VRExport::write '" << obj->getName() << "' to '" << path << "', extension: " << ext << endl;
    if (ext == ".ply") { writePly( dynamic_pointer_cast<VRGeometry>(obj), path); }
    if (ext == ".gltf") { writeGLTF(obj, path); }
#ifndef WITHOUT_DWG
    if (ext == ".dwg") { writeDWG(obj, path); }
#endif

    if (ext == ".wrl" || ext == ".wrz" || ext == ".obj" || ext == ".osb" || ext == ".osg")
        SceneFileHandler::the()->write(obj->getNode()->node, path.c_str());

    //SceneFileHandler::the()->print();
}
