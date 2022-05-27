#ifndef VRPYENGINEERING_H_INCLUDED
#define VRPYENGINEERING_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRRobotArm.h"
#include "VRPipeSystem.h"
#include "VRNumberingEngine.h"
#include "Space/VRSpaceMission.h"
#include "Space/VRRocketExhaust.h"
#include "Wiring/VRWire.h"
#include "Wiring/VRWiringSimulation.h"
#include "Wiring/VRElectricSystem.h"
#include "Wiring/VRElectricVisualization.h"
#include "Wiring/VRElectricComponent.h"
#include "Programming/VRLADVariable.h"
#include "Programming/VRLADEngine.h"

struct VRPyNumberingEngine : VRPyBaseT<OSG::VRNumberingEngine> {
    static PyMethodDef methods[];
};

struct VRPyRobotArm : VRPyBaseT<OSG::VRRobotArm> {
    static PyMethodDef methods[];
};

struct VRPyPipeSystem : VRPyBaseT<OSG::VRPipeSystem> {
    static PyMethodDef methods[];
};

struct VRPyWire : VRPyBaseT<OSG::VRWire> {
    static PyMethodDef methods[];
};

struct VRPyWiringSimulation : VRPyBaseT<OSG::VRWiringSimulation> {
    static PyMethodDef methods[];
};

struct VRPyElectricSystem : VRPyBaseT<OSG::VRElectricSystem> {
    static PyMethodDef methods[];
};

struct VRPyElectricVisualization : VRPyBaseT<OSG::VRElectricVisualization> {
    static PyMethodDef methods[];
};

struct VRPyElectricComponent : VRPyBaseT<OSG::VRElectricComponent> {
    static PyMethodDef methods[];
};

struct VRPyLADVariable : VRPyBaseT<OSG::VRLADVariable> {
    static PyMethodDef methods[];
};

struct VRPyLADEngine : VRPyBaseT<OSG::VRLADEngine> {
    static PyMethodDef methods[];
};

struct VRPyRocketExhaust : VRPyBaseT<OSG::VRRocketExhaust> {
    static PyMethodDef methods[];
};

struct VRPySpaceMission : VRPyBaseT<OSG::VRSpaceMission> {
    static PyMethodDef methods[];
};

#endif //VRPYENGINEERING_H_INCLUDED
