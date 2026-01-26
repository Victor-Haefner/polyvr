#include "OSGObject.h"

#include <OpenSG/OSGStringAttributeMap.h>

using namespace OSG;

OSGObject::OSGObject(NodeMTRecPtr node) {
    this->node = node;
}

OSGObject::~OSGObject() {
    ;
}

OSGObjectPtr OSGObject::create(NodeMTRecPtr node) { return shared_ptr<OSGObject>( new OSGObject(node) ); }

void OSGObject::setAttachment(string name, string value) {
    StringAttributeMapUnrecPtr aMap = 0;
    Attachment* att = node->findAttachment( StringAttributeMap::getClassType().getGroupId());
    if (!att) {
        aMap = StringAttributeMap::create();
        node->addAttachment(aMap);
    } else aMap = dynamic_cast<StringAttributeMap*>(att);
    if (aMap) aMap->setAttribute(name, value);
}

bool OSGObject::hasAttachment(string name) {
    Attachment* att = node->findAttachment( StringAttributeMap::getClassType().getGroupId());
    StringAttributeMapUnrecPtr aMap = dynamic_cast<StringAttributeMap*>(att);
    if (aMap && aMap->hasAttribute(name)) return true;
    return false;
}

string OSGObject::getAttachment(string name) {
    Attachment* att = node->findAttachment( StringAttributeMap::getClassType().getGroupId());
    StringAttributeMapUnrecPtr aMap = dynamic_cast<StringAttributeMap*>(att);
    if (aMap && aMap->hasAttribute(name)) return aMap->getAttribute(name);
    return "";
}
