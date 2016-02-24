#ifndef VRSTORAGE_H_INCLUDED
#define VRSTORAGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include "VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

namespace xmlpp{ class Element; }

ptrFktFwd(VRStore, xmlpp::Element*);
ptrFktFwd(VRStorageFactory, OSG::VRStoragePtr&);

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRStorageBin {
    VRStoreCbPtr f1; // load
    VRStoreCbPtr f2; // save
};

class VRStorage {
    private:
        vector<VRUpdatePtr> f_update; // update
        map<string, VRStorageBin> storage;
        static map<string, VRStorageFactoryCbPtr> factory;

        template<class T> void typeFactoryCb(VRStoragePtr& s);

        template<typename T> void save_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void save_on_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void save_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void save_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void load_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void save_vec_cb(vector<std::shared_ptr<T> >* v, xmlpp::Element* e);
        template<typename T> void load_vec_cb(vector<std::shared_ptr<T> >* v, xmlpp::Element* e);

    protected:

        template<typename T> void store(string tag, T* t);
        template<typename T> void store(string tag, vector<std::shared_ptr<T> >& v);
        template<typename T> void storeMap(string tag, map<string, T*>* mt);
        template<typename T> void storeMap(string tag, map<int, T*>* mt);
        template<typename To, typename T> void storeObjName(string tag, To* o, T* t);
        void regStorageUpdateFkt(VRUpdatePtr u);
        template<class T> void regStorageType(string t);

    public:
        VRStorage();

        void save(xmlpp::Element* e);
        void saveUnder(xmlpp::Element* e);
        void load(xmlpp::Element* e);
        static VRStoragePtr createFromStore(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VRSTORAGE_H_INCLUDED
