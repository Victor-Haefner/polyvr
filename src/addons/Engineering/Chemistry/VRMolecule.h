#ifndef VRMOLECULE_H_INCLUDED
#define VRMOLECULE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


struct PeriodicTableEntry {
    int valence_electrons = 0;
    Vec3f color;

    PeriodicTableEntry();
    PeriodicTableEntry( int valence_electrons, Vec3f color);
};

class VRAtom;
struct VRBond {
    VRAtom* atom = 0;
    int type = 1;
    bool extra = false;

    VRBond(int t, VRAtom* a);
};

class VRAtom {
    public:
        string type;
        PeriodicTableEntry params;

        int ID = 0; // ID in molecule
        bool full = false; // all valence electrons bound
        Matrix transformation;

        int bound_valence_electrons = 0;

        vector<VRBond> bonds;
        string geo;

    public:
        VRAtom(string type, int ID);

        PeriodicTableEntry getParams();
        Matrix getTransformation();
        vector<VRBond> getBonds();
        int getID();

		void computeGeo();
		void computePositions();
		bool append(VRBond bond);

		void print();
};

class VRMolecule : public VRGeometry {
    private:
        vector<VRAtom*> atoms;

        VRGeometry* bonds_geo;

        static string a_vp;
        static string a_fp;
        static string a_gp;

        static string b_vp;
        static string b_fp;
        static string b_gp;

		void addAtom(VRBond b);
		void addAtom(string a, int b);
		void addAtom(int a, int b);
		void updateGeo();
		vector<string> parse(string mol, bool verbose = false);

    public:
        VRMolecule(string definition);

        void set(string definition);
        void setRandom(int N);
};

OSG_END_NAMESPACE;

#endif // VRMOLECULE_H_INCLUDED
