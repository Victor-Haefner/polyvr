#ifndef VRMOLECULEMAT_H_INCLUDED
#define VRMOLECULEMAT_H_INCLUDED

#include "core/objects/material/VRMaterial.h"
#include "addons/Engineering/VREngineeringFwd.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRMoleculeMat {
    private:
        VRMaterialPtr mat1; // atoms
        VRMaterialPtr mat2; // bonds
        VRMaterialPtr mat3; // coords

        static string a_vp;
        static string a_fp;
        static string a_gp;

        static string b_vp;
        static string b_fp;
        static string b_gp;

    public:
        VRMoleculeMat();
        ~VRMoleculeMat();

        static VRMoleculeMatPtr create();

        VRMaterialPtr getAtomsMaterial();
        VRMaterialPtr getBondsMaterial();
        VRMaterialPtr getCoordsMaterial();

        void apply(VRGeometryPtr atoms, VRGeometryPtr bonds);
};

OSG_END_NAMESPACE;

#endif // VRMOLECULEMAT_H_INCLUDED
