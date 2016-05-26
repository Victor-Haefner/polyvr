#ifndef VRSTEP_H_INCLUDED
#define VRSTEP_H_INCLUDED

class STEPfile;
class Registry;
class InstMgr;
class SDAI_Application_instance;
typedef SDAI_Application_instance STEPentity;
class STEPaggregate;
class SDAI_Select;
class STEPattribute;
class STEPcomplex;

#include <memory>
#include <string>
#include <map>
#include <tuple>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiTreeExplorer;

class VRSTEP {
    public:
        typedef shared_ptr<Registry> RegistryPtr;
        typedef shared_ptr<InstMgr> InstMgrPtr;
        typedef shared_ptr<STEPfile> STEPfilePtr;

        struct Type {
            bool print = false;
            string path; // args
            string cpath; // complex entity args
            shared_ptr< VRFunction<STEPentity*> > cb;
        };

        struct Node {
            bool traversed = 0;
            string type = "ENTITY";
            string a_val = "NONE";
            string a_name = "NONE";
            STEPentity* entity = 0;
            STEPaggregate* aggregate = 0;
            SDAI_Select* select = 0;
            map<STEPentity*, Node*> parents;
            vector<Node*> childrenV;
            map<STEPentity*, Node*> children;

            void addChild(Node*);
            STEPentity* key();
        };

        map<STEPentity*, Node*> nodes;

        struct Instance {
            string type;
            STEPentity* entity = 0;
            int ID = -1;
            void* data = 0;
            template<size_t i, class... Args>
            typename std::tuple_element<i, tuple<Args...> >::type get() {
                auto t = (tuple<Args...>*)data;
                return std::get<i>(*t);
            }

            operator bool() const { return data != 0; }
        };

        struct Edge;
        struct Bound;
        struct Surface;

        vector<STEPentity*> unfoldComplex(STEPentity* e);
        void on_explorer_select(VRGuiTreeExplorer* e);

    public:
        RegistryPtr registry;
        InstMgrPtr instMgr;
        STEPfilePtr sfile;

        map<string, bool> blacklist;
        int blacklisted = 0;
        string options;

        string redBeg  = "\033[0;38;2;255;150;150m";
        string greenBeg  = "\033[0;38;2;150;255;150m";
        string blueBeg  = "\033[0;38;2;150;150;255m";
        string colEnd = "\033[0m";

        map<STEPentity*, int> explRowIds;
        map<STEPentity*, Instance> instances;
        map<int, Instance> instancesById;
        map<string, vector<Instance> > instancesByType;
        map<STEPentity*, VRTransformPtr> resGeos;
        VRTransformPtr resRoot;

        map<string, Type> types;
        Instance& getInstance(STEPentity* e);
        template<class T> void addType(string type, string path, string cpath, bool print = false);
        template<class T> void parse(STEPentity* e, string path, string cpath, string type);
        template<typename T> bool query(STEPentity* e, string path, T& t, string type);
        STEPentity* getSelectEntity(SDAI_Select* s, string ID);

        void loadT(string file, STEPfilePtr sfile, bool* done);
        void open(string file);

        void registerEntity(STEPentity* se, bool complexPass = 0);
        void parseEntity(STEPentity* se, bool complexPass = 0);
        void traverseEntity(STEPentity* se, int lvl, VRSTEP::Node* parent, bool complexPass = 0);
        void traverseSelect(SDAI_Select* s, string ID, int lvl, VRSTEP::Node* parent);
        void traverseAggregate(STEPaggregate* sa, int atype, STEPattribute* attr, int lvl, VRSTEP::Node* parent);
        void explore(VRSTEP::Node* node, int parent = 0);

        void buildGeometries();
        void buildScenegraph();
        void build();

    public:
        VRSTEP();

        void load(string file, VRTransformPtr res, string options);
};

OSG_END_NAMESPACE;


#endif // VRSTEP_H_INCLUDED
