#include "VRAttachment.h"
#include "VRObjectT.h"
#include "core/utils/VRStorage_template.h"

using namespace std;

typedef VRAttachment::base* VRAttachmentBasePtr;

template<> string typeName(const VRAttachmentBasePtr& data) { return data->typeName(); }

template<> string toString(const VRAttachmentBasePtr& data) {
    if (!data) return "";
    return data->typeName() + "|" + data->asString();
}

template<> int toValue(stringstream& ss, VRAttachmentBasePtr& data) {
    string val = ss.str();
    if (val == "") return true;
    string type = splitString(val, '|')[0];
    int Ltype = 0;
    if (type == "string") { data = new VRAttachment::attachment<string>(val); Ltype = 7; }
    if (data) data->fromString( subString(val, Ltype, -1) );
    return true;
}

VRAttachment::base::~base() {}

VRAttachment::VRAttachment(string name) : VRName() {
    auto ns = setNameSpace("VRAttachment");
    ns->setUniqueNames(false);
    setName(name);
    store("value", &data);
}

VRAttachment::~VRAttachment() { if (data) delete data; }

string VRAttachment::asString() {
    if (data) return data->asString();
    return "";
}

void VRAttachment::fromString(string s) {
    if (data) data->fromString(s);
}
