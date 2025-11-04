#include "VRExport.h"
#include "VRPLY.h"
#include "GLTF/GLTF.h"
#ifndef WITHOUT_E57
#include "E57/E57.h"
#endif
#include "COLLADA/VRCOLLADA.h"
#include "GIS/VRGDAL.h"
#ifndef WITHOUT_DWG
#include "VRDWG.h"
#endif
#ifndef WITHOUT_VTK
#include "VRVTK.h"
#endif
//#include "VRCOLLADA.h"
//#include "VRSTEP.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include "core/objects/object/VRObject.h"
#include "core/objects/OSGObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRPointCloud.h"
#include "core/utils/system/VRSystem.h"

using namespace OSG;


VRExport::VRExport() {}

VRExport* VRExport::get() { static VRExport* s = new VRExport(); return s; }

void VRExport::write(VRObjectPtr obj, string path, map<string, string> options) {
    if (!obj) return;

    string ext = getFileExtension(path);
    cout << "VRExport::write '" << obj->getName() << "' to '" << path << "', extension: " << ext << endl;
    if (ext == ".ply") { writePly( dynamic_pointer_cast<VRGeometry>(obj), path); }
    if (ext == ".gltf") { writeGLTF(obj, path); }
#ifndef WITHOUT_DWG
    if (ext == ".dwg") { writeDWG(obj, path); }
#endif
#ifndef WITHOUT_E57
    if (ext == ".e57") { writeE57(dynamic_pointer_cast<VRPointCloud>(obj), path); }
#endif

    if (ext == ".wrl" || ext == ".wrz" || ext == ".obj" || ext == ".osb" || ext == ".osg")
        SceneFileHandler::the()->write(obj->getNode()->node, path.c_str());

#ifndef WITHOUT_COLLADA
    if (ext == ".dae") { writeCollada(obj, path, options); }
#endif
#ifndef WITHOUT_GDAL
    if (ext == ".shp") { writeSHP(obj, path, options); }
#endif
#ifndef WITHOUT_VTK
    if (ext == ".vtk") { writeVTK(obj, path, options); return; }
#endif
}
