#ifndef VRGIZMO_H_INCLUDED
#define VRGIZMO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRToolsFwd.h"
#include "core/objects/VRTransform.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGizmo : public VRTransform {
	private:
	public:
		VRGizmo(string name);
		~VRGizmo();

		static VRGizmoPtr create(string name = "gizmo");
		VRGizmoPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRGIZMO_H_INCLUDED
