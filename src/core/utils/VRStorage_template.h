#include "VRStorage.h"
#include "toString.h"
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
    auto sp = t->lock();
    if (sp == 0) return;
    e->set_attribute(tag, sp->getName());
}

template<typename T>
void VRStorage::save_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e) {
    for (auto t : *mt) {
        xmlpp::Element* ei = e->add_child(tag);
        t.second->save(ei);
    }
}

template<typename T>
void VRStorage::save_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e) {
    for (auto t : *mt) {
        xmlpp::Element* ei = e->add_child(tag);
        t.second->save(ei);
        ei->set_attribute("ID", toString(t.first));
    }
}

template<typename T>
void VRStorage::load_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e) {
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
void VRStorage::load_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e) {
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string _ID = el->get_attribute("ID")->get_value();
        int ID = toInt( _ID );
        if (mt->count(ID) == 0) {
            T* o = new T();
            o->load(el);
            (*mt)[ID] = o;
        }
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
    b.f1 = boost::bind( &VRStorage::load_str_map_cb<T>, this, mt, tag, _1 );
    b.f2 = boost::bind( &VRStorage::save_str_map_cb<T>, this, mt, tag, _1 );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<int, T*>* mt) {
    bin b;
    b.f1 = boost::bind( &VRStorage::load_int_map_cb<T>, this, mt, tag, _1 );
    b.f2 = boost::bind( &VRStorage::save_int_map_cb<T>, this, mt, tag, _1 );
    storage[tag] = b;
}

OSG_END_NAMESPACE;
