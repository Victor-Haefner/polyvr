#ifndef VRCRYSTAL_H_INCLUDED
#define VRCRYSTAL_H_INCLUDED

#include "VRMolecule.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCrystal : public VRMolecule {
    private:
        vector< pair<int,Vec3d> > cell;

    public:
        VRCrystal(string name);
        ~VRCrystal();

        static VRCrystalPtr create(string name = "crystal");

        void loadCell(string path);
        void setSize(Vec3i s);
};

OSG_END_NAMESPACE;

#endif // VRCRYSTAL_H_INCLUDED
