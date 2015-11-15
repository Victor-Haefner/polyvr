#ifndef VRSTEP_H_INCLUDED
#define VRSTEP_H_INCLUDED

class STEPfile;
class Registry;
class InstMgr;
class SDAI_Application_instance;
class STEPaggregate;
#include <memory>

using namespace std;

class VRSTEP {
    public:
        typedef shared_ptr<Registry> RegistryPtr;
        typedef shared_ptr<InstMgr> InstMgrPtr;
        typedef shared_ptr<STEPfile> STEPfilePtr;

    private:
        RegistryPtr registry = 0;
        InstMgrPtr instances = 0;
        STEPfilePtr sfile = 0;

        void loadT(string file, STEPfilePtr sfile, bool* done);

    public:
        VRSTEP();

        void load(string file);


        string offset(int lvl);

        void printEntity(SDAI_Application_instance *se, int lvl);

        void printEntityAggregate(STEPaggregate *sa, int lvl);

        void build2();


        void build();

        void import(string file);
};


#endif // VRSTEP_H_INCLUDED
