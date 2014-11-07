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

class VRAtom {
    private:
        string type;
        PeriodicTableEntry params;

        int ID = 0; // ID in molecule
        bool full = false; // all valence electrons bound
        Matrix transformation;

        vector<VRAtom*> bonds;
        string geo;

    public:
        VRAtom(string type, int ID);

        PeriodicTableEntry getParams();
        Matrix getTransformation();
        vector<VRAtom*> getBonds();
        int getID();

		void computeGeo();
		void computePositions();
		bool append(VRAtom* at);

		void print();
};

class VRMolecule : public VRGeometry {
    private:
        vector<VRAtom*> atoms;

        VRGeometry* bonds_geo;

        static string vp;
        static string fp;
        static string gp;

		void addAtom(string a);
		void updateGeo();
		vector<string> parse(string mol);

    public:
        VRMolecule(string definition);

        void set(string definition);
        void setRandom(int N);
};

OSG_END_NAMESPACE;

#endif // VRMOLECULE_H_INCLUDED
