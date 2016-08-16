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
        string type = "Node";
        int persistency = 666;
        vector<VRUpdatePtr> f_setup; // setup
        vector<VRUpdatePtr> f_setup_after; // setup after tree loaded
        map<string, VRStorageBin> storage;
        static map<string, VRStorageFactoryCbPtr> factory;

        template<class T> static void typeFactoryCb(VRStoragePtr& s);

        template<typename T> void save_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void save_on_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void save_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void save_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void save_int_objmap_cb(map<int, std::shared_ptr<T> >* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void load_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_int_objmap_cb(map<int, std::shared_ptr<T> >* mt, string tag, xmlpp::Element* e);
        template<typename T> void save_vec_cb(vector<std::shared_ptr<T> >* v, xmlpp::Element* e);
        template<typename T> void load_vec_cb(vector<std::shared_ptr<T> >* v, xmlpp::Element* e);
        template<typename T> void save_obj_cb(std::shared_ptr<T>* v, string tag, xmlpp::Element* e);
        template<typename T> void load_obj_cb(std::shared_ptr<T>* v, string tag, xmlpp::Element* e);

    protected:

        template<typename T> void store(string tag, T* t);
        template<typename T> void storeObj(string tag, std::shared_ptr<T>& o);
        template<typename T> void storeObjVec(string tag, vector<std::shared_ptr<T> >& v);
        template<typename T> void storeMap(string tag, map<string, T*>* mt);
        template<typename T> void storeMap(string tag, map<int, T*>* mt);
        template<typename T> void storeMap(string tag, map<int, std::shared_ptr<T> >* mt);
        template<typename To, typename T> void storeObjName(string tag, To* o, T* t);

        void setStorageType(string t);
        void regStorageSetupFkt(VRUpdatePtr u);
        void regStorageSetupAfterFkt(VRUpdatePtr u);

        xmlpp::Element* getChild(xmlpp::Element* e, string c);

    public:
        VRStorage();

        void save(xmlpp::Element* e, int p = 0);
        xmlpp::Element* saveUnder(xmlpp::Element* e, int p = 0);
        void load(xmlpp::Element* e);
        void loadChildFrom(xmlpp::Element* e);

        static int getPersistency(xmlpp::Element* e);
        static VRStoragePtr createFromStore(xmlpp::Element* e);
        template<class T> static void regStorageType(string t);

        void setPersistency(int p);
        int getPersistency();
};

OSG_END_NAMESPACE;

#endif // VRSTORAGE_H_INCLUDED
