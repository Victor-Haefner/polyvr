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

#include <memory>
#include <string>
#include <map>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSTEP {
    private:
        struct ClosedShell { vector<int> faces; }; //face
        struct AdvancedBrepShapeRepresentation { string name; vector<int> items; }; //representation_item

    public:
        typedef shared_ptr<Registry> RegistryPtr;
        typedef shared_ptr<InstMgr> InstMgrPtr;
        typedef shared_ptr<STEPfile> STEPfilePtr;

        struct Type {
            string path;
            shared_ptr< VRFunction<STEPentity*> > cb;
        };

    private:
        RegistryPtr registry;
        InstMgrPtr instances;
        STEPfilePtr sfile;

        map<string, int> histogram;

        string redBeg  = "\033[0;38;2;255;150;150m";
        string colEnd = "\033[0m";

        map<int, void*> resMap;
        map<int, VRGeometryPtr> resObject;
        bool parseClosedShell(STEPentity* se);
        bool parseAdvancedBrepShapeRepresentation(STEPentity* se);

        map<string, Type> types;
        template<class T> void addType(string type, string path);
        template<class T> void parse(STEPentity* e, string path);

        void loadT(string file, STEPfilePtr sfile, bool* done);
        void open(string file);
        string indent(int lvl);
        vector<int> getAggregateEntities(STEPattribute* attr);

        void traverseEntity(STEPentity* se, int lvl);
        void traverseSelect(SDAI_Select* s, int lvl, STEPattribute* attr);
        void traverseAggregate(STEPaggregate* sa, int type, int lvl);
        void build();

    public:
        VRSTEP();

        VRTransformPtr load(string file);
};

OSG_END_NAMESPACE;


#endif // VRSTEP_H_INCLUDED
