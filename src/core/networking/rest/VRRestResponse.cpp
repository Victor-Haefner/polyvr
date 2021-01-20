#include "VRRestResponse.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRRestResponse& p) { return "RestResponse"; }

VRRestResponse::VRRestResponse() {}
VRRestResponse::~VRRestResponse() {}

VRRestResponsePtr VRRestResponse::create() { return VRRestResponsePtr( new VRRestResponse() ); }
VRRestResponsePtr VRRestResponse::ptr() { return static_pointer_cast<VRRestResponse>(shared_from_this()); }

void VRRestResponse::setStatus(string s) { status = s; }
void VRRestResponse::setData(string s) { data = s; }
void VRRestResponse::appendData(string s) { data += s; }

string VRRestResponse::getStatus() { return status; }
string VRRestResponse::getData() { return data; }
