#include "VRRestServer.h"
#include "core/utils/toString.h"

using namespace OSG;

VRRestServer::VRRestServer(string n) : name(n) {}
VRRestServer::~VRRestServer() {}

VRRestServerPtr VRRestServer::create(string name) { return VRRestServerPtr( new VRRestServer(name) ); }
VRRestServerPtr VRRestServer::ptr() { return static_pointer_cast<VRRestServer>(shared_from_this()); }
