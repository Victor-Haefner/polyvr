#ifndef VRMOLECULE_H_INCLUDED
#define VRMOLECULE_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "addons/Engineering/VREngineeringFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

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

class VRMolecule : public VRGeometry {
    private:
        string definition;
        map<int, VRAtom*> atoms;
        map<int, VRAtom*> nonFullAtoms;

        VRGeometryPtr bonds_geo = 0;
        VRGeometryPtr coords_geo = 0;
        VRNumberingEnginePtr labels = 0;
        bool doLabels = false;
        bool doCoords = false;
        VRMoleculeMatPtr material;

    protected:
		void updateLabels();
		void updateCoords();

		int getID();
		vector<string> parse(string mol, bool verbose = false);

		unsigned int getFlag();

    public:
        VRMolecule(string name);

        static VRMoleculePtr create(string name = "molecule");
        VRMoleculePtr ptr();

        void set(string definition);
        void setRandom(int N);
        string getDefinition();

		int addAtom(string a);
		void connectAtom(VRAtom* b, int bType, bool extra = false);
		void connectAtom(int a, int b);
        VRAtom* getAtom(int ID);
        Vec3d getAtomPosition(int ID);
        void setAtomPosition(int ID, Vec3d pos);

        void setLocalOrigin(int ID);

        void substitute(int a, VRMoleculePtr m, int b);
        void attachMolecule(int a, VRMoleculePtr m, int b);
        void rotateBond(int a, int b, float f);
        void changeBond(int a, int b, int t);
		void remAtom(int ID);

		void updateGeo();
		void update();

        void showLabels(bool b);
        void showCoords(bool b);
};

OSG_END_NAMESPACE;

#endif // VRMOLECULE_H_INCLUDED
