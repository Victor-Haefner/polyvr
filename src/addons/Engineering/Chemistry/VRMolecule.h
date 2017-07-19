#ifndef VRMOLECULE_H_INCLUDED
#define VRMOLECULE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

class VRNumberingEngine;

OSG_BEGIN_NAMESPACE;
using namespace std;

struct PeriodicTableEntry {
    int valence_electrons = 0;
    Vec3d color;
    float radius = 1;

    PeriodicTableEntry();
    PeriodicTableEntry( int valence_electrons, float radius, Vec3d color);
};

class VRAtom;
struct VRBond {
    VRAtom* atom1 = 0;
    VRAtom* atom2 = 0;
    int type = 1;
    int slot = 0;
    bool extra = false;
    Pnt3d p1, p2;

    VRBond();
    VRBond(int t, int s, VRAtom* a2, VRAtom* a1);
};

class VRAtom {
    public:
        string type;
        PeriodicTableEntry params;

        int ID = 0; // ID in molecule
        bool full = false; // all valence electrons bound
        Matrix4d transformation;

        int bound_valence_electrons = 0;
        uint recFlag = 0;

        map<int, VRBond> bonds;
        string geo;

    public:
        VRAtom(string type, int ID);
        ~VRAtom();

        PeriodicTableEntry getParams();
        Matrix4d getTransformation();
        void setTransformation(Matrix4d m);
        map<int, VRBond>& getBonds();
        int getID();
        void setID(int ID);

		void computeGeo();
		void computePositions();

		bool append(VRAtom* b, int bType, bool extra = false);
		void detach(VRAtom* a);

		void propagateTransformation(Matrix4d& T, uint flag, bool self = true);

		void print();
};

class VRMolecule : public VRGeometry {
    private:
        string definition;
        map<int, VRAtom*> atoms;

        VRGeometryPtr bonds_geo = 0;
        VRGeometryPtr coords_geo = 0;
        VRNumberingEnginePtr labels = 0;
        bool doLabels = false;
        bool doCoords = false;

        static string a_vp;
        static string a_fp;
        static string a_gp;

        static string b_vp;
        static string b_fp;
        static string b_gp;

		void addAtom(VRAtom* b, int bType, bool extra = false);
		void addAtom(string a, int b);
		void addAtom(int a, int b);
		void updateLabels();
		void updateCoords();

		int getID();
		vector<string> parse(string mol, bool verbose = false);

		uint getFlag();

    public:
        VRMolecule(string name);

        static VRMoleculePtr create(string name = "molecule");
        VRMoleculePtr ptr();

        void set(string definition);
        void setRandom(int N);
        string getDefinition();

        VRAtom* getAtom(int ID);

        void setLocalOrigin(int ID);

        void substitute(int a, VRMoleculePtr m, int b);
        void attachMolecule(int a, VRMoleculePtr m, int b);
        void rotateBond(int a, int b, float f);
        void changeBond(int a, int b, int t);
		void remAtom(int ID);

		void updateGeo();

        void showLabels(bool b);
        void showCoords(bool b);
};

OSG_END_NAMESPACE;

#endif // VRMOLECULE_H_INCLUDED
