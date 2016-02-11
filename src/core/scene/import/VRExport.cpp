#include "VRExport.h"
#include "VRPLY.h"
//#include "VRCOLLADA.h"
//#include "VRSTEP.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include <boost/filesystem.hpp>
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace OSG;


VRExport::VRExport() {}

VRExport* VRExport::get() { static VRExport* s = new VRExport(); return s; }

void VRExport::write(VRObjectPtr obj, string path) {
    auto geo = dynamic_pointer_cast<VRGeometry>(obj);

    auto bp = boost::filesystem::path(path);
    auto ext = bp.extension();
    if (ext.string() == "ply" && geo) { writePly(geo, path); }
    SceneFileHandler::the()->write(obj->getNode(), path.c_str());
}
