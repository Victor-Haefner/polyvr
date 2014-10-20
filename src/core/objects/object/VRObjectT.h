#ifndef VROBJECTT_H_INCLUDED
#define VROBJECTT_H_INCLUDED

#include "VRObject.h"
#include "VRAttachment.h"

using namespace std;

template<typename T>
VRAttachment::attachment<T>::attachment(T& t) : data(t) {}

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
    attachments[name] = new VRAttachment();
    attachments[name]->set(t);
}

template<typename T>
T OSG::VRObject::getAttachment(string name) { return attachments[name]->get<T>(); }

#endif // VROBJECTT_H_INCLUDED
