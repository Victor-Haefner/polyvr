#ifndef VRNATUREMANAGER_H_INCLUDED
#define VRNATUREMANAGER_H_INCLUDED

#include "../nature/VRTree.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNatureManager {

    map<VRScene*, list<VRTree*> > trees;

    private:
        VRNatureManager() { ; }
        void operator= (VRNatureManager v) {;}

    public:
        static VRNatureManager* get() {
            static VRNatureManager* singleton_opt = 0;
            if (singleton_opt == 0) singleton_opt = new VRNatureManager();
            return singleton_opt;
        }

        void addTree(VRScene* scene) {
            VRTree* t = new VRTree();
            trees[scene].push_back(t);
            scene->add(t);
        }
};

OSG_END_NAMESPACE;


#endif // VRNATUREMANAGER_H_INCLUDED
