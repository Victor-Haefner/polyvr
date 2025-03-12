#include "VRGizmo.h"

using namespace OSG;

VRGizmo::VRGizmo(string name) : VRTransform(name) {}
VRGizmo::~VRGizmo() {}

VRGizmoPtr VRGizmo::create(string name) { return VRGizmoPtr( new VRGizmo(name) ); }
VRGizmoPtr VRGizmo::ptr() { return static_pointer_cast<VRGizmo>(shared_from_this()); }
