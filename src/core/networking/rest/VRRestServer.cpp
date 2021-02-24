#include "VRRestServer.h"
#include "core/utils/toString.h"

using namespace OSG;

VRRestServer::VRRestServer() {}
VRRestServer::~VRRestServer() {}

VRRestServerPtr VRRestServer::create() { return VRRestServerPtr( new VRRestServer() ); }
VRRestServerPtr VRRestServer::ptr() { return static_pointer_cast<VRRestServer>(shared_from_this()); }
