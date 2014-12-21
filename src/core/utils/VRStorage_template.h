#include "VRStorage.h"
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

template<typename T>
void VRStorage::load_cb(T* t, string tag, xmlpp::Element* e) {
    if (e->get_attribute(tag) == 0) return;
    toValue( e->get_attribute(tag)->get_value(), *t);
}

template<typename T>
void VRStorage::save_cb(T* t, string tag, xmlpp::Element* e) {
    e->set_attribute(tag, toString(*t));
}

template<typename T>
void VRStorage::save_on_cb(T* t, string tag, xmlpp::Element* e) {
    if (*t == 0) return;
    e->set_attribute(tag, (*t)->getName());
}

template<typename T>
void VRStorage::save_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e) {
    for (auto t : *mt) {
        xmlpp::Element* ei = e->add_child(tag);
        //xmlpp::Element* ei = e->add_child(t.second->getName());
        t.second->save(ei);
    }
}

template<typename T>
void VRStorage::load_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e) {
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string name = el->get_name();
        if (el->get_attribute("base_name")) name = el->get_attribute("base_name")->get_value();
        T* o = new T(name);
        o->load(el);

        name = o->getName();
        if (mt->count(name)) delete o;
        else (*mt)[name] = o;
    }
}

template<typename T>
void VRStorage::store(string tag, T* t) {
    bin b;
    b.f1 = boost::bind( &VRStorage::load_cb<T>, this, t, tag, _1 );
    b.f2 = boost::bind( &VRStorage::save_cb<T>, this, t, tag, _1 );
    storage[tag] = b;
}

template<typename To, typename T>
void VRStorage::storeObjName(string tag, To* o, T* t) {
    bin b;
    b.f1 = boost::bind( &VRStorage::load_cb<T>, this, t, tag, _1 );
    b.f2 = boost::bind( &VRStorage::save_on_cb<To>, this, o, tag, _1 );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<string, T*>* mt) {
    bin b;
    b.f1 = boost::bind( &VRStorage::load_map_cb<T>, this, mt, tag, _1 );
    b.f2 = boost::bind( &VRStorage::save_map_cb<T>, this, mt, tag, _1 );
    storage[tag] = b;
}

OSG_END_NAMESPACE;
