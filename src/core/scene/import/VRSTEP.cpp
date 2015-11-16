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

void VRSTEP::printEntity(SDAI_Application_instance *se, int lvl) {
    cout << offset(lvl) << "Entity:" << se->EntityName() << "(" << se->STEPfile_id << ")" << endl;
    cout << offset(lvl) << "Description:" << se->getEDesc()->Description() << endl;
    cout << offset(lvl) << "Entity Type:" << se->getEDesc()->Type() << endl;
    cout << offset(lvl) << "Atributes:" << endl;

    STEPattribute *attr;
    se->ResetAttributes();
    while ( (attr = se->NextAttribute()) != NULL ) {
        cout << offset(lvl) << attr->Name() << ": " << attr->asStr() << " TypeName: " << attr->TypeName() << " Type: " << attr->Type() << endl;
        if ( attr->Type() == 256 ) {
            if (attr->IsDerived()) cout << offset(lvl) << "        ********* DERIVED *********" << endl;
            else printEntity( attr->Entity(),lvl+2);
        } else if ((attr->Type() == SET_TYPE)||(attr->Type() == LIST_TYPE)) {
            STEPaggregate *sa = attr->Aggregate();
            if ( attr->BaseType() == ENTITY_TYPE ) printEntityAggregate(sa, lvl+2);
        }
    }
}

void VRSTEP::printEntityAggregate(STEPaggregate *sa, int lvl) {
    string s; sa->asStr(s);
    cout << offset(lvl) << "Aggregate:" << s << endl;

    EntityNode *sn = (EntityNode *)sa->GetHead();
    SDAI_Application_instance *sse;
    while ( sn != NULL) {
        sse = (SDAI_Application_instance *)sn->node;

        if (((sse->getEDesc()->Type() == SET_TYPE)||(sse->getEDesc()->Type() == LIST_TYPE))&&(sse->getEDesc()->BaseType() == ENTITY_TYPE)) {
            printEntityAggregate((STEPaggregate *)sse,lvl+2);
        } else if ( sse->getEDesc()->Type() == ENTITY_TYPE ) printEntity(sse,lvl+2);
        else cout << "Instance Type not handled:" << endl;
        sn = (EntityNode *)sn->NextNode();
    }
}

void VRSTEP::build2() {
    for( int i=0; i<instances->InstanceCount(); i++) {
        SDAI_Application_instance *se = instances->GetSTEPentity(i);
        printEntity(se,1);
    }
}


void VRSTEP::build() {

    map<int, string> vectors;

    for (int i=0; i<instances->InstanceCount(); i++) {
        SDAI_Application_instance* inst = instances->GetApplication_instance(i);
        const EntityDescriptor* desc = inst->getEDesc();
        string type = desc->Name();

        if ( type == "Direction" || type == "Cartesian_Point" ) {
            int ID = inst->GetFileId();
            vectors[ID] = string(inst->attributes[1].asStr());

            STEPattribute vec_attrib = inst->attributes[1];
            const AttrDescriptor* adescr = vec_attrib.getADesc();
            STEPaggregate* data = vec_attrib.Aggregate();
            if (data == 0) continue;

            STEPnode* d1 = (STEPnode*)data->GetHead();
            string tmp; data->asStr(tmp);
            //data->

            float f[3];
            for (int i=0; i<data->EntryCount(); i++) {
                SDAI_Application_instance* jj = (SDAI_Application_instance*)d1;

                //jj->ResetAttributes();
                cout << " vi attrib: " << jj->AttributeCount() << endl;
                /*for (int j=0; j<jj->AttributeCount(); j++) {
                    STEPattribute* attrib = jj->NextAttribute();
                    cout << " vi attrib: " << attrib->asStr() << endl;
                }*/
                d1 = (STEPnode*)d1->next;
            }

            cout << "vec: " << inst->GetFileId() << " " << inst->EntityName() << " N " << data->EntryCount() << endl;
            cout << "   " << adescr->Name() << " " << f[0] << " " << f[1] << " " << f[2] << endl;
            continue;
        }

        continue;

        cout << "instance: " << inst->GetFileId() << " " << inst->EntityName() << " of type " << desc->Name() << endl;

        inst->ResetAttributes();
        for (int j=0; j<inst->AttributeCount(); j++) {
            STEPattribute* attrib = inst->NextAttribute();
            cout << " attrib: " << attrib->asStr() << endl;
        }
    }
}

VRTransformPtr VRSTEP::load(string file) {
    open(file);
    build2();
    return 0;
}




