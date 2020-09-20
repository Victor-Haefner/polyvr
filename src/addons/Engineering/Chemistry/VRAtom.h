#ifndef VRATOM_H_INCLUDED
#define VRATOM_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGMatrix.h>
#include "addons/Engineering/VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

struct PeriodicTableEntry {
    int valence_electrons = 0;
    Color3f color;
    float radius = 1;

    PeriodicTableEntry();
    PeriodicTableEntry( int valence_electrons, float radius, Color3f color);
};

class VRAtom {
    public:
        static map<string, PeriodicTableEntry> PeriodicTable;
        static map<string, vector<Matrix4d> > AtomicStructures;

        string type;
        PeriodicTableEntry params;

        int ID = 0; // ID in molecule
        bool full = false; // all valence electrons bound
        Matrix4d transformation;

        int bound_valence_electrons = 0;
        unsigned int recFlag = 0;

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

		void propagateTransformation(Matrix4d& T, unsigned int flag, bool self = true);

		void print();
		static void initAtomicTables();
};

OSG_END_NAMESPACE;

#endif // VRATOM_H_INCLUDED
