#include "VRPyMolecule.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;

simpleVRPyType(Molecule, New_VRObjects_ptr);
simpleVRPyType(Crystal, New_VRObjects_ptr);

PyMethodDef VRPyMolecule::methods[] = {
    {"set", PyWrap(Molecule, set, "Set the molecule from string - set('CH4')", void, string ) },
    {"setRandom", PyWrap(Molecule, setRandom, "Set a random molecule - setRandom(123)", void, int ) },
    {"showLabels", PyWrap(Molecule, showLabels, "Display the ID of each atom - showLabels(True)", void, bool ) },
    {"showCoords", PyWrap(Molecule, showCoords, "Display the coordinate system of each atom - showCoords(True)", void, bool ) },
    {"substitute", PyWrap(Molecule, substitute, "Substitute an atom of both molecules to append the second to this - substitute(int aID, mol b, int bID)", void, int, VRMoleculePtr, int ) },
    {"attachMolecule", PyWrap(Molecule, attachMolecule, "Attach a molecule to the second - attachMolecule(int aID, mol b, int bID)", void, int, VRMoleculePtr, int ) },
    {"rotateBond", PyWrap(Molecule, rotateBond, "Rotate the bond between atom a && b - rotateBond(int aID, int bID, float a)", void, int, int, float ) },
    {"changeBond", PyWrap(Molecule, changeBond, "Change the bond type between atom a && b to type t- changeBond(int aID, int bID, int t)", void, int, int, int ) },
    {"addAtom", PyWrap(Molecule, addAtom, "Add an atom, returns ID", int, string ) },
    {"remAtom", PyWrap(Molecule, remAtom, "Remove an atom by ID", void, int ) },
    {"setAtomPosition", PyWrap(Molecule, setAtomPosition, "Set the position of the atom by ID", void, int, Vec3d ) },
    {"getAtomPosition", PyWrap(Molecule, getAtomPosition, "Returns the position of the atom by ID", Vec3d, int ) },
    {"update", PyWrap(Molecule, update, "Update, call after changing configuration", void ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCrystal::methods[] = {
    {"loadCell", PyWrap(Crystal, loadCell, "Load elementary cell from file", void, string ) },
    {"setSize", PyWrap(Crystal, setSize, "Set crystal size in multiples of the elementary cell", void, Vec3i ) },
    {NULL}  /* Sentinel */
};
