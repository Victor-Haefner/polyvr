#include "VRPipeSystem.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRPipeSystem& m) { return "PipeSystem"; }

struct VRPipeSegment {

};

struct VRPipeNode {

};

VRPipeSystem::VRPipeSystem() {}
VRPipeSystem::~VRPipeSystem() {}

VRPipeSystemPtr VRPipeSystem::create() { return VRPipeSystemPtr( new VRPipeSystem() ); }
VRPipeSystemPtr VRPipeSystem::ptr() { return static_pointer_cast<VRPipeSystem>(shared_from_this()); }
