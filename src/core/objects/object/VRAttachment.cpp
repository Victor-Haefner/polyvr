#include "VRAttachment.h"
#include "VRObjectT.h"
#include "core/utils/VRStorage_template.h"

using namespace std;

typedef VRAttachment::base* VRAttachmentBasePtr;

template<> string toString(const VRAttachmentBasePtr& data) {
    if (!data) return "";
    return data->asString();
}

template<> int toValue(stringstream& ss, VRAttachmentBasePtr& data) {
    if (data) data->fromString( ss.str() );
    return true;
}

VRAttachment::base::~base() {}

VRAttachment::VRAttachment(string name) : VRName() { setName(name); store("value", &data); }
VRAttachment::~VRAttachment() { if (data) delete data; }

string VRAttachment::asString() {
    if (data) return data->asString();
    return "";
}

void VRAttachment::fromString(string s) {
    if (data) data->fromString(s);
}
