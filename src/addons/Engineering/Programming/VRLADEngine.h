#ifndef VRLADENGINE_H_INCLUDED
#define VRLADENGINE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Engineering/VREngineeringFwd.h"

#include <string>
#include <vector>
#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRLADEngine : public std::enable_shared_from_this<VRLADEngine> {
    public:
        class CompileUnit;
        class Wire;
        class Access;
        class Part;
        typedef shared_ptr<CompileUnit> CompileUnitPtr;
        typedef shared_ptr<Wire> WirePtr;
        typedef shared_ptr<Access> AccessPtr;
        typedef shared_ptr<Part> PartPtr;

        class Access {
            public:
                string ID;
                CompileUnitPtr cu;
                vector<string> components;
                string variable;
                bool constant = false;

            public:
                Access(string ID, CompileUnitPtr cu);
                string toString();
        };

        class Wire {
            public:
                string name = "wire";
                string ID = ID;
                CompileUnitPtr cu;
                vector<string> inputs;
                vector<string> outputs;
                string accessID;
                string operand;
                bool powerrail = false;
                int lastComputationResult = 0;

            public:
                Wire(string ID, CompileUnitPtr cu);

                void addInput(string ID);
                void addOutput(string ID);
                void addOperand(string ID);

                string toString();
        };

        class Part {
            public:
                string ID;
                CompileUnitPtr cu = 0;
                string name;
                vector<string> inputs;
                vector<string> outputs;
                vector<string> operands;
                bool negated = false;
                int lastComputationResult = 0;

            public:
                Part(string ID, CompileUnitPtr cu, string name);

                vector<pair<WirePtr,bool>> getOperands();
                vector<pair<VRLADVariablePtr,bool>> getVariables();
                pair<VRLADVariablePtr, bool> getVariable(int i = -1);
                bool isOperand();
                bool isBlock();
                int computeOperandOutput(int value = 0);
                int computeBlockOutput(int value = 0);
                string toString();
        };

        class CompileUnit {
            public:
                string ID;
                map<string, PartPtr> parts;
                map<string, WirePtr> wires;
                map<string, AccessPtr> accesses;
                map<string, VRLADVariablePtr> variables;
                vector<string> poweredWireIDs;

            public:
                CompileUnit(string ID);
                string toString();
        };


	private:
		map<string, CompileUnitPtr> compileUnits;
		CompileUnitPtr unit2E;

		VRElectricSystemPtr esystem;

	public:
		VRLADEngine();
		~VRLADEngine();

		static VRLADEnginePtr create();
		VRLADEnginePtr ptr();

		void setElectricEngine(VRElectricSystemPtr esystem);
		void read();
		void iterate();
};

OSG_END_NAMESPACE;

#endif //VRLADENGINE_H_INCLUDED
