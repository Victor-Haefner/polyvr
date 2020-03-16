#include "VRAttachment.h"
#include "VRObjectT.h"

using namespace std;



VRAttachment::base::~base() {}

VRAttachment::VRAttachment() {}
VRAttachment::~VRAttachment() { if (data) delete data; }

string VRAttachment::asString() {
    if (data) return data->asString();
    return "";
}
