#include "VRStorage.h"
#include "toString.h"
#include "VRFunction.h"
#include "xml.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<typename T>
void VRStorage::load_cb(T* t, string tag, VRStorageCbParams p) {
    if (!p.e->hasAttribute(tag)) return;
    toValue( p.e->getAttribute(tag), *t);
}

template<typename T>
void VRStorage::save_cb(T* t, string tag, VRStorageCbParams p) {
    if (t) p.e->setAttribute(tag, toString(*t));
}

template<typename T>
void VRStorage::save_on_cb(T* t, string tag, VRStorageCbParams p) {
    auto sp = t->lock();
    if (sp == 0) return;
    p.e->setAttribute(tag, sp->getName());
}

template<typename T>
void VRStorage::save_str_map_cb(map<string, T*>* mt, string tag, bool under, VRStorageCbParams p) {
    if (mt->size() == 0) return;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(p.e, p.p);
        //if (ei && t.second->overrideCallbacks) t.second->save(ei, p.p); // TODO: some classes like VRScript have a custom save method
        if (ei && t.second->overrideCallbacks) t.second->save(ei); // TODO: some classes like VRScript have a custom save method
    }
}

template<typename T>
void VRStorage::save_str_objmap_cb(map<string, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (mt->size() == 0) return;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(p.e, p.p);
        //if (ei && t.second->overrideCallbacks) t.second->save(ei, p.p); // TODO: some classes like VRScript have a custom save method
        if (ei && t.second->overrideCallbacks) t.second->save(ei); // TODO: some classes like VRScript have a custom save method
    }
}

template<typename T>
void VRStorage::save_int_map_cb(map<int, T*>* mt, string tag, bool under, VRStorageCbParams p) {
    if (mt->size() == 0) return;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(p.e, p.p);
        if (ei) ei->setAttribute("ID", toString(t.first));
    }
}

template<typename T>
void VRStorage::save_int_map2_cb(map<int, T>* mt, string tag, bool under, VRStorageCbParams p) {
    if (mt->size() == 0) return;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *mt) {
        auto ei = p.e->addChild("e");
        if (ei) {
            ei->setAttribute("ID", toString(t.first));
            ei->setAttribute("data", toString(t.second));
        }
    }
}

template<typename T>
void VRStorage::save_int_objmap_cb(map<int, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (mt->size() == 0) return;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(p.e, p.p);
        if (ei) ei->setAttribute("ID", toString(t.first));
    }
}

template<typename T>
void VRStorage::save_int_objumap_cb(unordered_map<int, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (mt->size() == 0) return;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *mt) {
        auto ei = t.second->saveUnder(p.e, p.p);
        if (ei) ei->setAttribute("ID", toString(t.first));
    }
}

int getID(XMLElementPtr el);

template<typename T>
void VRStorage::load_int_objmap_cb(map<int, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        int ID = getID(el);
        if (ID < 0) { cout << "VRStorage::load_int_objmap_cb Error: object " << el->getName() << " in map '" << tag << "' has no attribute ID!\n"; return; }
        if (mt->count(ID) == 0) {
            auto t = T::create();
            t->load(el);
            (*mt)[ID] = t;
        }
    }
}

template<typename T>
void VRStorage::load_int_objumap_cb(unordered_map<int, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        int ID = getID(el);
        if (ID < 0) { cout << "VRStorage::load_int_objmap_cb Error: object " << el->getName() << " in map '" << tag << "' has no attribute ID!\n"; return; }
        if (mt->count(ID) == 0) {
            auto t = T::create();
            t->load(el);
            (*mt)[ID] = t;
        }
    }
}

template<typename T>
void VRStorage::load_str_objmap_cb(map<string, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        string name = el->getName();
        if (el->hasAttribute("base_name")) name = el->getAttribute("base_name");
        auto t = T::create(name);
        t->load(el);

        name = t->getName();
        if (!mt->count(name)) (*mt)[name] = t;
    }
}

template<typename T>
void VRStorage::load_str_objumap_cb(unordered_map<string, std::shared_ptr<T> >* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        string name = el->getName();
        if (el->hasAttribute("base_name")) name = el->getAttribute("base_name");
        auto t = T::create(name);
        t->load(el);

        name = t->getName();
        if (!mt->count(name)) (*mt)[name] = t;
    }
}

template<typename T>
void VRStorage::load_str_map_cb(map<string, T*>* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        string name = el->getName();
        if (el->hasAttribute("base_name")) name = el->getAttribute("base_name");
        T* o = new T(name);
        o->load(el);

        name = o->getName();
        if (mt->count(name)) delete o;
        else (*mt)[name] = o;
    }
}

template<typename T>
void VRStorage::load_int_map_cb(map<int, T*>* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        int ID = getID(el);
        if (mt->count(ID) == 0) {
            T* o = new T();
            o->load(el);
            (*mt)[ID] = o;
        }
    }
}

template<typename T>
void VRStorage::load_int_map2_cb(map<int, T>* mt, string tag, bool under, VRStorageCbParams p) {
    if (under) p.e = p.e->getChild(tag);
    if (!p.e) return;
    for (auto el : p.e->getChildren()) {
        int ID = getID(el);
        if (mt->count(ID) == 0) {
            T o;
            toValue( el->getAttribute("data"), o);
            (*mt)[ID] = o;
        }
    }
}

template<typename T>
void VRStorage::save_obj_vec_cb(vector<std::shared_ptr<T> >* v, string tag, bool under, VRStorageCbParams p) {
    //cout << "storeObjVec " << tag << " " << v->size() << endl;
    if (under) p.e = p.e->addChild(tag);
    for (auto t : *v) t->saveUnder(p.e, p.p);
}

template<typename T>
void VRStorage::load_obj_vec_cb(vector<std::shared_ptr<T> >* v, string tag, bool under, VRStorageCbParams p) {
    if (p.e == 0) return;
    if (under) p.e = p.e->getChild(tag);
    if (p.e == 0) return;
    auto children = p.e->getChildren();
    bool doReload = (v->size() == children.size());
    for (uint i=0; i<children.size(); i++) {
        auto el = children[i];
        if (doReload) (*v)[i]->load(el);
        else {
            VRStoragePtr s = VRStorage::createFromStore(el, false);
            if (!s) s = T::create();
            auto c = static_pointer_cast<T>(s);
            if (!c) continue;
            if (el->getName() != s->type) continue;
            c->load(el);
            v->push_back( c );
        }
    }
}

template<typename T>
void VRStorage::save_vec_cb(vector<T>* v, string tag, VRStorageCbParams p) {
    p.e = p.e->addChild(tag);
    for (auto t : *v) {
        auto e2 = p.e->addChild("e");
        e2->setAttribute("val", toString(t));
    }
}

template<typename T>
void VRStorage::save_vec_on_cb(vector<T>* v, string tag, VRStorageCbParams p) {
    p.e = p.e->addChild(tag);
    for (auto t : *v) {
        auto sp = t.lock();
        if (sp == 0) continue;
        auto e2 = p.e->addChild("e");
        e2->setAttribute("val", sp->getName());
    }
}

template<typename T>
void VRStorage::load_vec_cb(vector<T>* v, string tag, VRStorageCbParams p) {
    if (p.e == 0) return;
    p.e = p.e->getChild(tag);
    if (p.e == 0) return;
    v->clear();
    for (auto el : p.e->getChildren()) {
        if (!el->hasAttribute("val")) continue;
        T t;
        toValue( el->getAttribute("val"), t);
        v->push_back( t );
    }
}

template<typename T>
void VRStorage::save_vec_vec_cb(vector<vector<T>>* v, string tag, VRStorageCbParams p) {
    p.e = p.e->addChild(tag);
    for (auto t : *v) save_vec_cb(&t, tag, p.e);
}

template<typename T>
void VRStorage::load_vec_vec_cb(vector<vector<T>>* v, string tag, VRStorageCbParams p) {
    if (p.e == 0) return;
    p.e = p.e->getChild(tag);
    if (p.e == 0) return;
    v->clear();
    for (auto el : p.e->getChildren()) {
        v->push_back(vector<T>());
        vector<T>* vv = &v->at(v->size()-1);
        for (auto el2 : el->getChildren()) {
            if (!el2->hasAttribute("val")) continue;
            T t;
            toValue( el2->getAttribute("val"), t);
            vv->push_back( t );
        }
    }
}

template<typename T>
void VRStorage::save_obj_cb(std::shared_ptr<T>* v, string tag, VRStorageCbParams p) {
    //cout << " storeObj " << tag << " " << endl;
    if (*v) (*v)->saveUnder(p.e, p.p, tag);
}

template<typename T>
void VRStorage::load_obj_cb(std::shared_ptr<T>* v, string tag, VRStorageCbParams p) {
    p.e = p.e->getChild(tag);
    if (!p.e) return;
    if (!*v) *v = T::create();
    (*v)->load(p.e);
}

template<typename T>
void VRStorage::storeObj(string tag, std::shared_ptr<T>& o) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_obj_cb<T>, this, &o, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_obj_cb<T>, this, &o, tag, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeObjVec(string tag, vector<std::shared_ptr<T> >& v, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_obj_vec_cb<T>, this, &v, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_obj_vec_cb<T>, this, &v, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeVec(string tag, vector<T>& v) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_vec_cb<T>, this, &v, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_vec_cb<T>, this, &v, tag, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeVecVec(string tag, vector<vector<T>>& v){
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_vec_vec_cb<T>, this, &v, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_vec_vec_cb<T>, this, &v, tag, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::store(string tag, T* t) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_cb<T>, this, t, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_cb<T>, this, t, tag, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeObjName(string tag, T* o, string* t) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_cb<string>, this, t, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_on_cb<T>, this, o, tag, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeObjNames(string tag, vector<T>* o, vector<string>* v) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_vec_cb<string>, this, v, tag, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_vec_on_cb<T>, this, o, tag, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, unordered_map<string, std::shared_ptr<T> >* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_str_objumap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_str_objumap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, unordered_map<int, std::shared_ptr<T> >* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_int_objumap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_int_objumap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<string, std::shared_ptr<T> >* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_str_objmap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_str_objmap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<int, std::shared_ptr<T> >* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_int_objmap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_int_objmap_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<string, T*>* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_str_map_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_str_map_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<int, T*>* mt, bool under) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_int_map_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_int_map_cb<T>, this, mt, tag, under, placeholders::_1 ) );
    storage[tag] = b;
}

template<typename T>
void VRStorage::storeMap(string tag, map<int, T>& mt) {
    VRStorageBin b;
    b.f1 = VRStoreCb::create("load", bind( &VRStorage::load_int_map2_cb<T>, this, &mt, tag, true, placeholders::_1 ) );
    b.f2 = VRStoreCb::create("save", bind( &VRStorage::save_int_map2_cb<T>, this, &mt, tag, true, placeholders::_1 ) );
    storage[tag] = b;
}

template<class T>
void VRStorage::typeFactoryCb(VRStoragePtr& s) { s = T::create(); }

template<class T>
void VRStorage::regStorageType(string t) {
    factory[t] = VRStorageFactoryCb::create("factorycb", bind( &VRStorage::typeFactoryCb<T>, placeholders::_1 ) );
}

OSG_END_NAMESPACE;
