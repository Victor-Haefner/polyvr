#include "VRCollaboration.h"

using namespace OSG;

VRCollaboration::VRCollaboration() {}
VRCollaboration::~VRCollaboration() {}

VRCollaborationPtr VRCollaboration::create() { return VRCollaborationPtr( new VRCollaboration() ); }
VRCollaborationPtr VRCollaboration::ptr() { return static_pointer_cast<VRCollaboration>(shared_from_this()); }
