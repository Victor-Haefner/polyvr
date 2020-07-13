#ifndef VROBJECTT_H_INCLUDED
#define VROBJECTT_H_INCLUDED

#include "VRObject.h"
#include "VRAttachment.h"
#include "core/utils/toString.h"

using namespace std;

template<typename T>
VRAttachment::attachment<T>::attachment(T& t) : data(t) {}

template<typename T>
VRAttachment::attachment<T>::~attachment() {}

template<typename T>
string VRAttachment::attachment<T>::asString() { return toString<T>(data); }

template<typename T>
void VRAttachment::attachment<T>::fromString(string s) { toValue<T>(s, data); }

template<typename T>
string VRAttachment::attachment<T>::typeName() { return ::typeName<T>(data); }

template<typename T>
void VRAttachment::set(T& t) {
    if (data != 0) delete data;
    data = new attachment<T>(t);
}

template<typename T>
T VRAttachment::get() {
    if (data == 0) return T();
    return ((attachment<T>*)(data))->data;
}

template<typename T>
void OSG::VRObject::addAttachment(string name, T t) {
    if (!attachments.count(name)) attachments[name] = new VRAttachment(name);
    attachments[name]->set(t);
}

template<typename T>
T OSG::VRObject::getAttachment(string name) { return attachments[name]->get<T>(); }

#endif // VROBJECTT_H_INCLUDED
