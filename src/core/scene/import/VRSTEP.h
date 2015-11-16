#ifndef VRSTEP_H_INCLUDED
#define VRSTEP_H_INCLUDED

class STEPfile;
class Registry;
class InstMgr;
class SDAI_Application_instance;
typedef SDAI_Application_instance STEPentity;
class STEPaggregate;

#include <memory>
#include <string>
#include <map>
#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSTEP {
    public:
        typedef shared_ptr<Registry> RegistryPtr;
        typedef shared_ptr<InstMgr> InstMgrPtr;
        typedef shared_ptr<STEPfile> STEPfilePtr;

    private:
        RegistryPtr registry;
        InstMgrPtr instances;
        STEPfilePtr sfile;

        map<string, int> histogram;

        map<void*, float> resScalar;
        map<void*, Vec3f> resVec3f;
        map<void*, pose> resPose;
        bool parseVec3f(STEPentity* se);
        bool parsePose(STEPentity* se);

        void loadT(string file, STEPfilePtr sfile, bool* done);
        void open(string file);
        string offset(int lvl);
        void traverseEntity(STEPentity* se, int lvl);
        void traverseAggregate(STEPaggregate* sa, int type, int lvl);
        void build();

    public:
        VRSTEP();

        VRTransformPtr load(string file);
};

OSG_END_NAMESPACE;


#endif // VRSTEP_H_INCLUDED
