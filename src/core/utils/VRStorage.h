#ifndef VRSTORAGE_H_INCLUDED
#define VRSTORAGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <boost/function.hpp>
#include <map>

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRStorage {
    private:
        struct bin {
            boost::function<void (xmlpp::Element*)> f1; // load
            boost::function<void (xmlpp::Element*)> f2; // save
        };

        map<string, bin > storage;
        map<string, bin >::iterator itr;

        template<typename T> void save_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void save_on_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void save_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void save_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_cb(T* t, string tag, xmlpp::Element* e);
        template<typename T> void load_str_map_cb(map<string, T*>* mt, string tag, xmlpp::Element* e);
        template<typename T> void load_int_map_cb(map<int, T*>* mt, string tag, xmlpp::Element* e);

    protected:

        template<typename T> void store(string tag, T* t);
        template<typename T> void storeMap(string tag, map<string, T*>* mt);
        template<typename T> void storeMap(string tag, map<int, T*>* mt);
        template<typename To, typename T> void storeObjName(string tag, To* o, T* t);

    public:
        VRStorage();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VRSTORAGE_H_INCLUDED
