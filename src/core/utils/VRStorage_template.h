#include "VRStorage.h"
#include "toString.h"
#include "VRFunction.h"
#include <libxml++/nodes/element.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>

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
void VRStorage::save_str_map_cb(map<string, T*>* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = e->add_child(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(e);
        t.second->save(ei); // TODO: some classes like VRScript have a custom save method
    }
}

template<typename T>
void VRStorage::save_str_objmap_cb(map<string, std::shared_ptr<T> >* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = e->add_child(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(e);
    }
}

template<typename T>
void VRStorage::save_int_map_cb(map<int, T*>* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = e->add_child(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(e);
        ei->set_attribute("ID", toString(t.first));
    }
}

template<typename T>
void VRStorage::save_int_objmap_cb(map<int, std::shared_ptr<T> >* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = e->add_child(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(e);
        ei->set_attribute("ID", toString(t.first));
    }
}

template<typename T>
void VRStorage::load_int_objmap_cb(map<int, std::shared_ptr<T> >* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = getChild(e, tag);
    if (!e) return;
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string _ID;
        if (el->get_attribute("ID")) _ID = el->get_attribute("ID")->get_value();
        else { cout << "VRStorage::load_int_objmap_cb Error: object " << el->get_name() << " in map '" << tag << "' has no attribute ID!\n"; return; }
        int ID = toInt( _ID );
        if (mt->count(ID) == 0) {
            auto t = T::create();
            t->load(el);
            (*mt)[ID] = t;
        }
    }
}

template<typename T>
void VRStorage::load_str_objmap_cb(map<string, std::shared_ptr<T> >* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = getChild(e, tag);
    if (!e) return;
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string name = el->get_name();
        if (el->get_attribute("base_name")) name = el->get_attribute("base_name")->get_value();
        auto t = T::create(name);
        t->load(el);

        name = t->getName();
        if (!mt->count(name)) (*mt)[name] = t;
    }
}

template<typename T>
void VRStorage::load_str_map_cb(map<string, T*>* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = getChild(e, tag);
    if (!e) return;
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
void VRStorage::load_int_map_cb(map<int, T*>* mt, string tag, bool under, xmlpp::Element* e) {
    if (under) e = getChild(e, tag);
    if (!e) return;
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
void VRStorage::save_vec_cb(vector<std::shared_ptr<T> >* v, xmlpp::Element* e) {
    for (auto t : *v) t->saveUnder(e);
}

template<typename T>
void VRStorage::load_vec_cb(vector<std::shared_ptr<T> >* v, xmlpp::Element* e) {
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;
        auto t = T::create();
        t->load(el);
        v->push_back( t );
    }
}

template<typename T>
void VRStorage::save_obj_cb(std::shared_ptr<T>* v, string tag, xmlpp::Element* e) {
    (*v)->saveUnder(e);
}

template<typename T>
void VRStorage::load_obj_cb(std::shared_ptr<T>* v, string tag, xmlpp::Element* e) {
    for (auto n : e->get_children()) {
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;
        if (el->get_name() != tag) continue;

        *v = T::create();
        (*v)->load(el);
        return;
    }
}

template<typename T>
void VRStorage::storeObj(string tag, std::shared_ptr<T>& o) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_obj_cb<T>, this, &o, tag, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_obj_cb<T>, this, &o, tag, _1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeObjVec(string tag, vector<std::shared_ptr<T> >& v) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_vec_cb<T>, this, &v, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_vec_cb<T>, this, &v, _1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::store(string tag, T* t) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_cb<T>, this, t, tag, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_cb<T>, this, t, tag, _1 ) );
    storage[tag] = b;
}

template<typename To, typename T>
void VRStorage::storeObjName(string tag, To* o, T* t) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_cb<T>, this, t, tag, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_on_cb<To>, this, o, tag, _1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<string, std::shared_ptr<T> >* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_str_objmap_cb<T>, this, mt, tag, under, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_str_objmap_cb<T>, this, mt, tag, under, _1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<int, std::shared_ptr<T> >* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_int_objmap_cb<T>, this, mt, tag, under, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_int_objmap_cb<T>, this, mt, tag, under, _1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<string, T*>* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_str_map_cb<T>, this, mt, tag, under, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_str_map_cb<T>, this, mt, tag, under, _1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<int, T*>* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", boost::bind( &VRStorage::load_int_map_cb<T>, this, mt, tag, under, _1 ) );
    b.f2 = VRStoreCb::create("save", boost::bind( &VRStorage::save_int_map_cb<T>, this, mt, tag, under, _1 ) );
    storage[tag] = b;
}

template<class T>
void VRStorage::typeFactoryCb(VRStoragePtr& s) { s = T::create(); }

template<class T>
void VRStorage::regStorageType(string t) {
    factory[t] = VRStorageFactoryCb::create("factorycb", boost::bind( &VRStorage::typeFactoryCb<T>, _1 ) );
}

OSG_END_NAMESPACE;
