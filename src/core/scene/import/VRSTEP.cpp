#include "VRSTEP.h"

#include <STEPfile.h>
#include <schema.h>

#include <thread>
#include <unistd.h>
#include <memory>

/*

IMPORTANT: ..not compiling? you need to install the stepcode package!
open a terminal
cd ~/Project/polyvr/dependencies
sudo git pull
cd ubuntu_14.04
sudo gdebi -n libstepcode-dev.deb

*/

using namespace std;
using namespace OSG;

void VRSTEP::loadT(string file, STEPfilePtr sfile, bool* done) {
    sfile->ReadExchangeFile(file);
    registry->ResetSchemas();
    registry->ResetEntities();
    *done = true;
}

VRSTEP::VRSTEP() {
    registry = RegistryPtr( new Registry( SchemaInit ) ); // schema
    instances = InstMgrPtr( new InstMgr() ); // instances
    sfile = STEPfilePtr( new STEPfile( *registry, *instances, "", false ) ); // file
}

void VRSTEP::open(string file) {
    bool done = false;
    thread t(&VRSTEP::loadT, this, file, sfile, &done);

    while(!done) {
        auto p = sfile->GetReadProgress();
        cout << "progress " << p << endl;
        sleep(1);
    }

    t.join();
}


string VRSTEP::offset(int lvl) {
    string s;
    for ( int i=0; i< lvl; i++) s += "    ";
    return s;
}

bool VRSTEP::parseVec3f(STEPentity* se) {
    if (resVec3f.count(se)) return true;
    if (se->AttributeCount() != 2) return false;
    auto attr = &se->attributes[1];
    if (attr->BaseType() != 2) return false;
    auto sa = attr->Aggregate(); if (!sa) return false;
    if (sa->EntryCount() != 3) return false;
    Vec3f v;

    RealNode* sn = (RealNode *)sa->GetHead();
    for (int i=0; i<3; i++) {
        v[i] = sn->value;
        sn = (RealNode *)sn->NextNode();
    }

    resVec3f[se] = v;
    return true;
}

bool VRSTEP::parsePose(STEPentity* se) {
    if (resPose.count(se)) return true;
    if (se->AttributeCount() != 4) return false;
    vector<STEPentity*> ents(3);
    vector<STEPattribute*> attribs(3);
    for (int i=0; i<3; i++) {
        attribs[i] = &se->attributes[i+1];
        if (attribs[i]->BaseType() != 256) return false;
    }
    for (int i=0; i<3; i++) {
        ents[i] = attribs[i]->Entity();
        bool b1 = parseVec3f(ents[i]);
        if (!b1) return false;
    }
    resPose[se] = pose( resVec3f[ents[0]], resVec3f[ents[1]], resVec3f[ents[2]] );
    return true;
}

void VRSTEP::traverseEntity(STEPentity *se, int lvl) {
    string key = se->EntityName();
    if (histogram.count(key) == 0) histogram[key] = 0;
    histogram[key] += 1;

    if ( parseVec3f(se) ) return;
    if ( parsePose(se) ) return;

    string redBeg  = "\033[0;38;2;255;150;150m";
    string colEnd = "\033[0m";

    int ID = se->STEPfile_id;
    bool k = ID == 1837 || ID == 1836;

    if (k) cout << offset(lvl) << "Entity: " << redBeg << se->EntityName() << "(id: " << se->STEPfile_id << ")" << colEnd << endl;
    else cout << offset(lvl) << "Entity: " << se->EntityName() << "(id: " << se->STEPfile_id << ")" << endl;
    cout << offset(lvl) << "Description: " << se->getEDesc()->Description() << " Entity Type: " << se->getEDesc()->Type() << endl;

    STEPattribute* attr;
    se->ResetAttributes();
    while ( (attr = se->NextAttribute()) != NULL ) {
        cout << offset(lvl+1) << "A: " << attr->Name() << ": " << attr->asStr() << " TypeName: " << attr->TypeName() << " Type: " << attr->Type() << endl;
        if ( attr->Type() == 256 ) {
            if (!attr->IsDerived()) traverseEntity( attr->Entity(),lvl+2);
            //else cout << offset(lvl+1) << "        ********* DERIVED *********" << endl;
        }
        STEPaggregate* sa = attr->Aggregate();
        if (sa) traverseAggregate(sa, attr->BaseType(), lvl+2);
    }
}

void VRSTEP::traverseAggregate(STEPaggregate *sa, int atype, int lvl) {
    string s; sa->asStr(s);
    cout << offset(lvl) << "Aggregate: " << s << endl;

    STEPentity *sse;
    PrimitiveType etype, ebtype;

    for( EntityNode* sn = (EntityNode *)sa->GetHead(); sn != NULL; sn = (EntityNode *)sn->NextNode()) {
        switch (atype) {
            case ENTITY_TYPE: // 100
                sse = (STEPentity *)sn->node;
                etype = sse->getEDesc()->Type();
                ebtype = sse->getEDesc()->BaseType();
                switch (etype) {
                    case SET_TYPE:
                    case LIST_TYPE: traverseAggregate((STEPaggregate *)sse, ebtype, lvl+2); break;
                    case ENTITY_TYPE: traverseEntity(sse,lvl+2); break;
                    default: cout << offset(lvl+1) << "entity Type not handled:" << etype << endl;
                }
                break;
            case INTEGER_TYPE: // 1
            case REAL_TYPE: // 2
            case BOOLEAN_TYPE: // 4
            case LOGICAL_TYPE: // 8
            case STRING_TYPE: // 10
            case BINARY_TYPE: // 20
            case ENUM_TYPE: // 40
            case SELECT_TYPE: // 80
            case AGGREGATE_TYPE: // 200
            case NUMBER_TYPE: // 400
            default: cout << offset(lvl+1) << "aggregate Type not handled:" << atype << endl;
            ;
        }
    }
}

void VRSTEP::build() {
    for( int i=0; i<instances->InstanceCount(); i++) {
        STEPentity* se = instances->GetSTEPentity(i);
        if (se->STEPfile_id == 1840) break;
        traverseEntity(se,1);
    }

    for (auto k : histogram) cout << "H " << k.first << " " << k.second << endl;
}

VRTransformPtr VRSTEP::load(string file) {
    open(file);
    build();
    return 0;
}




