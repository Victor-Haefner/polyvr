#include "VRLADEngine.h"
#include "VRLADVariable.h"

#include "../Wiring/VRElectricSystem.h"

#include "core/utils/toString.h"
#include "core/utils/xml.h"

#include <functional>
#include <algorithm>

using namespace OSG;

VRLADEngine::VRLADEngine() {}
VRLADEngine::~VRLADEngine() {}

VRLADEnginePtr VRLADEngine::create() { return VRLADEnginePtr( new VRLADEngine() ); }
VRLADEnginePtr VRLADEngine::ptr() { return static_pointer_cast<VRLADEngine>(shared_from_this()); }

void VRLADEngine::setElectricEngine(VRElectricSystemPtr e) { esystem = e; }

VRLADEngine::CompileUnit::CompileUnit(string ID) : ID(ID) {}

string VRLADEngine::CompileUnit::toString() {
    string s = "CompileUnit: "+ID;
    s += "\n";
    for (auto c : parts) s += ", "+c.second->toString();
    s += "\n";
    for (auto c : wires) s += ", "+c.second->toString();
    s += "\n";
    for (auto c : accesses) s += ", "+c.second->toString();
    return s;
}

VRLADEngine::Part::Part(string ID, CompileUnitPtr cu, string name) : ID(ID), cu(cu), name(name) {}

template<class T>
bool has(vector<T>& v, T& t) {
    return (std::find(v.begin(), v.end(), t) != v.end());
}

vector<pair<VRLADEngine::WirePtr,bool>> VRLADEngine::Part::getOperands() {
    vector<pair<WirePtr,bool>> res;
    for (auto o : operands) {
        if (cu->wires.count(o)) {
            bool isIn = has(inputs,o);
            res.push_back( make_pair(cu->wires[o], isIn) );
        }
    }
    return res;
};

vector<pair<VRLADVariablePtr,bool>> VRLADEngine::Part::getVariables() {
    auto ops = getOperands();
    vector<pair<VRLADVariablePtr,bool>> res;
    for (auto o : ops) {
        auto op = o.first;
        bool isIn = o.second;
        if (op && cu->accesses.count(op->accessID)) {
            auto opAccess = cu->accesses[op->accessID];
            if (cu->variables.count(opAccess->variable)) {
                res.push_back( make_pair(cu->variables[opAccess->variable], isIn) );
            } else if (opAccess->constant) {
                auto v = VRLADVariable::create();
                v->setValue( opAccess->components[0] );
                res.push_back( make_pair(v, isIn) );
            }
        }
    }
    return res;
}

pair<VRLADVariablePtr, bool> VRLADEngine::Part::getVariable(int i) {
    auto vs = getVariables();
    if (i >= vs.size()) return make_pair<VRLADVariablePtr, bool>(0,0);
    return vs[i];
}

bool VRLADEngine::Part::isOperand() {
    static vector<string> operands = {"Contact", "PContact", "Coil", "RCoil", "Ge", "Lt"};
    return has(operands, name);
};

bool VRLADEngine::Part::isBlock() {
    static vector<string> blocks = {"Calc", "Move", "Round"};
    return has(blocks, name);
};

int VRLADEngine::Part::computeOperandOutput(int value) {
    auto variable = getVariable().first;
    if (!variable) {
        //print "Warning:", name, "has no variable!";
        return value;
    }

    if (name == "Coil" ) {
        if (variable->getName() == "Prc_Ext_Ok") return 0; // TODO: resolve this workaround!
        variable->setValue(::toString(value));
        return 0;
    }

    if (name == "RCoil" ) {
        variable->setValue(variable->getStartValue());
        return 0;
    }

    if (value == 0) return 0;

    int var = toFloat(variable->getValue());
    if (name == "Contact" || name == "PContact" ) {
        if (negated && var == 0) return value ;
        if (negated && var == 1) return 0;
        return var*value;
    }

    return 0;
}

int VRLADEngine::Part::computeBlockOutput(int value) {
    if (value == 0) return 0;

    vector<VRLADVariablePtr> inputs;
    vector<VRLADVariablePtr> outputs;
    for (auto v : getVariables()) {
        if (v.second) inputs.push_back(v.first);
        else outputs.push_back(v.first);
    }

    if (name == "Calc" ) {
        // berechnet anhand der input "operanden" und schreibt in output "operand"
        // get function from data!

        // hard coded test!
        float in1 = toFloat(inputs[0]->getValue());
        float in2 = toFloat(inputs[1]->getValue());
        outputs[0]->setValue( ::toString(in1 * in2) );
        return 1;
    }

    if (name == "Move" ) {
        outputs[0]->setValue( inputs[0]->getValue() );
        return 1;
    }

    if (name == "Round") { // TODO
        outputs[0]->setValue( inputs[0]->getValue() );
        return 1;
    }

    return 0;
}

string VRLADEngine::Part::toString() {
    string s = "Part: "+ID+" "+name+" Op:"+::toString(operands)+" Neg:"+::toString(negated)+" [";
    for (auto c : inputs) s += " "+c;
    s += " -> ";
    for (auto c : outputs) s += " "+c;
    s += "]";
    return s;
}

VRLADEngine::Access::Access(string ID, CompileUnitPtr cu) : ID(ID), cu(cu) {}

string VRLADEngine::Access::toString() {
    string s = "Access: "+ID;
    for (auto c : components) s += ", "+c;
    return s;
}

VRLADEngine::Wire::Wire(string ID, CompileUnitPtr cu) : ID(ID), cu(cu) {}

void VRLADEngine::Wire::addInput(string ID) {
    inputs.push_back(ID);
    if (cu->parts.count(ID)) cu->parts[ID]->outputs.push_back(ID);
}

void VRLADEngine::Wire::addOutput(string ID) {
    outputs.push_back(ID);
    if (cu->parts.count(ID)) cu->parts[ID]->inputs.push_back(ID);
}

void VRLADEngine::Wire::addOperand(string ID) {
    operand = ID;
    if (cu->parts.count(ID)) cu->parts[ID]->operands.push_back( ID );
}

string VRLADEngine::Wire::toString() {
    string s = "Wire: ID "+ID+" Ac"+accessID+" Op"+operand+" PR"+::toString(powerrail)+" [";
    for (auto c : inputs) s += " "+c;
    s += " -> ";
    for (auto c : outputs) s += " "+c;
    s += "] ";
    return s;
}

void VRLADEngine::read() {
	string namespace1 = "{http://www->siemens->com/automation/Openness/SW/NetworkSource/FlgNet/v1}";
	string namespace2 = "{http://www->siemens->com/automation/Openness/SW/Interface/v2}";
	string folder = "MA_Thesis/03_TIA_Portal/Gumball_Line_180117_V14->ap14/Extruder_Machine/";

	bool verbose = false; //set True for tick-prints

	auto getTag = [](XMLElementPtr node) {
//		if (not hasattr(node, "tag")) return "noTag";
		string tag = node->getName();
		if (tag[0] == '{') tag = splitString(tag, '}')[1];
		return tag;
	};

	function<void(XMLElementPtr, int, string)> printNode = [&](XMLElementPtr node, int depth = -1, string padding = "") {
		cout << padding << " " << getTag(node) << " " << ::toString(node->getAttributes()) << endl;
		if (depth == 0) return;
		for (auto child : node->getChildren() ) {
			printNode(child, depth-1, padding+" ");
		}
	};


	// get variables;
	auto getVariables = [&]() {
		// hardware variables, pins, sockets, etc->->;
		XML treeTagtable;
		treeTagtable.read(folder+"PLC-Variablen/Default tag table.xml"); //HMI_Ext_Start_M mit Adresse;
		auto rootTagTable = treeTagtable.getRoot();

		for (auto tag : rootTagTable->getChildren("SW.Tags.PlcTag", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("hardware");
			for (auto name : tag->getChildren("Name")) {
				variable->setName(name->getText());
			}
			for (auto logicalAddress : tag->getChildren("LogicalAddress")) {
				variable->setLogicalAddress(logicalAddress->getText());
			}
			for (auto dataType : tag->getChildren("DataTypeName")) {
				variable->setDataType(dataType->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}

		// program internal variables;
		XML procTable;
		procTable.read(folder+"Programmbausteine/003_Process_Data.xml");
		auto rootProcTable = procTable.getRoot();
		for (auto member : rootProcTable->getChildren(namespace2+"Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("internal");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			esystem->addVariable(variable->getName(), variable);
		}

		// hmi variables
		XML hmiTable;
		hmiTable.read(folder+"Programmbausteine/005_HMI_Data->xml")	; //HMI_Ext_Start mit Datatype;
		auto rootHmiTable = hmiTable.getRoot();
		for (auto member : rootHmiTable->getChildren(namespace2+"Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("hmi");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
			for (auto startValue : member->getChildren(namespace2+"StartValue")) {
				variable->setStartValue(startValue->getText());
				variable->setValue(startValue->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}

		// Alarms
		XML alarmsTable;
		alarmsTable.read(folder+"Programmbausteine/004_Alarms_Data->xml");
		auto rootAlarmsTable = alarmsTable.getRoot();
		for (auto member : rootAlarmsTable->getChildren(namespace2+"Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("alarm");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			esystem->addVariable(variable->getName(), variable);
		}

		//VFD Control Block;
		/*treeTagtable = ET->parse(folder+"Programmbausteine/003_VFD_Control_G120C_MM->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") {
			variable = LADVariable();
			variable->setSource("VFD");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			esystem->addVariable(variable->getName(), variable);
		}*/

		// VFD PAW
		XML pawTable;
		pawTable.read(folder+"Programmbausteine/003_VFD_PAW->xml")	;
		auto rootPawTable = pawTable.getRoot();
		for (auto member : rootPawTable->getChildren(namespace2+"Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("vfd");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
			for (auto startValue : member->getChildren(namespace2+"StartValue")) {
				variable->setStartValue(startValue->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}

		//VFD PEW
		XML pewTable;
		pewTable.read(folder+"Programmbausteine/003_VFD_PEW->xml")	;
		auto rootPewTable = pewTable.getRoot();
		for (auto member : rootPewTable->getChildren(namespace2+"Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("vfd");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
			for (auto startValue : member->getChildren(namespace2+"StartValue")) {
				variable->setStartValue(startValue->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}
	};

    XML processTable;
    processTable.read(folder+"Programmbausteine/003_Process->xml"); // HMI_Ext_Start + HMI_Ext_Start_M mit powered wires;

	// get compile units;
	auto getCompileUnits = [&]() {
		vector<XMLElementPtr> res;
		for (auto FC : processTable.getRoot()->getChildren("SW->Blocks->FC")) {
			for (auto objectList : FC->getChildren("ObjectList")) {
				for (auto compileUnit : objectList->getChildren("SW->Blocks->CompileUnit")) {
					res.push_back(compileUnit);
				}
			}
		}
		return res;
	};

	// get all compile units;
	auto setupCompileUnits = [&](map<string, VRLADVariablePtr> variables) {
		map<string, CompileUnitPtr> compileUnits;
		for (auto compileUnitNode : getCompileUnits()) {
            vector<XMLElementPtr> rparts;
            vector<XMLElementPtr> rwires;
            vector<XMLElementPtr> raccesses;
            for (auto attributeList : compileUnitNode->getChildren("AttributeList")) {
                for (auto networkSource : attributeList->getChildren("NetworkSource")) {
                    // StatementList ?
                    for (auto flgNet : networkSource->getChildren(namespace1+"FlgNet")) {
                        for (auto parts : flgNet->getChildren(namespace1+"Parts")) {
                            for (auto access : parts->getChildren(namespace1+"Access")) raccesses.push_back(access);
                            for (auto part : parts->getChildren(namespace1+"Part")) rparts.push_back(part);
                        }
                        for (auto wires : flgNet->getChildren(namespace1+"Wires")) {
                            for (auto wire : wires->getChildren(namespace1+"Wire")) rwires.push_back(wire);
                        }
                    }
                }
            }

			auto compileUnit = CompileUnitPtr( new CompileUnit(compileUnitNode->getAttribute("ID")) );
			compileUnit->variables = variables;

			for (auto partNode : rparts ) {
				auto part = PartPtr( new Part(partNode->getAttribute("UId"), compileUnit, partNode->getAttribute("Name")) );
				if (partNode->getChild(namespace1+"Negated")) part->negated = true;
				compileUnit->parts[part->ID] = part;
			}

			for (auto accessNode : raccesses ) {
				auto UId = accessNode->getAttribute("UId");
				auto access = AccessPtr( new Access(UId, compileUnit));
				auto Symbol = accessNode->getChild(namespace1+"Symbol");
				if (Symbol) {
					for (auto component : Symbol->getChildren(namespace1+"Component")) {
						auto name = component->getAttribute("Name");
						access->components.push_back(name);
						if (variables.count(name)) access->variable = name;
					}
				}

				auto Constant = accessNode->getChild(namespace1+"Constant"); //if there are constant accesses;
				if (Constant) {
					access->constant = true;
					for (auto value : Constant->getChildren(namespace1+"ConstantValue")) {
						access->components.push_back(value->getText());
					}
					for (auto value : Constant->getChildren(namespace1+"ConstantType")) {
						access->components.push_back(value->getText());
					}
				}

				compileUnit->accesses[access->ID] = access;
			}
			compileUnits[compileUnit->ID] = compileUnit;

			for (auto wireNode : rwires ) {
				auto wire = WirePtr( new Wire(wireNode->getAttribute("UId"), compileUnit) );
				auto identCon = wireNode->getChild(namespace1+"IdentCon");
				if (identCon) wire->accessID = identCon->getAttribute("UId");
				auto powerrail = wireNode->getChild(namespace1+"Powerrail");
				if (powerrail) {
					wire->powerrail = true;
					compileUnit->poweredWireIDs.push_back(wire->ID);
				}
				for (auto nameCon : wireNode->getChildren(namespace1+"NameCon")) {
					auto UId = nameCon->getAttribute("UId");
					auto name = nameCon->getAttribute("Name");
					if (startsWith(name, "pre")) wire->addOutput(UId) ; // TODO: check if correct;
					else if (startsWith(name, "in")) wire->addOutput(UId);
					else if (startsWith(name, "eno")) wire->addInput(UId);
					else if (startsWith(name, "en")) wire->addOutput(UId);
					else if (startsWith(name, "out")) wire->addInput(UId);
					else if (startsWith(name, "operand")) wire->addOutput(UId);
					if (compileUnit->accesses.count(wire->accessID)) wire->addOperand(UId);
				}
				compileUnit->wires[wire->ID] = wire;
			}
		}

		return compileUnits;
	};

	getVariables();
	compileUnits = setupCompileUnits(esystem->getLADVariables());

	//Test all variables for start function;
	unit2E = compileUnits["2E"];
	unit2E->variables["Button_Ext_Stop"]->setValue("1") ; // schalter am extruder, info muss aus ECAD kommen, E9->6;
	unit2E->variables["Prc_Ext_Ok"]->setValue("1") ; // viele inputs aus ECAD, E9->1, E9->2, E9->3, E9->6, E9->7, DB2->DBX4->0;
}

void VRLADEngine::iterate() { // TODO: once the import is finished tackle this
	/*auto getOutParts = [](wire) {
		vector<> res;
		for (ID : wire->outputs ) {
			if (ID in wire->cu->parts) res.push_back(wire->cu->parts[ID]);
		};
		return res;
	};

	auto getOutWires = [](part) {
		res = [];
		for (ID : part->outputs ) {
			if (ID in part->cu->wires) res.push_back(part->cu->wires[ID]);
		};
		return res;
	};

	auto getPowerWires = [](cu) {
		return [ cu->wires[wireID] for wireID in cu->poweredWireIDs if wireID in cu->wires ];
	};

	auto printStack = [](stack) {
		print len(stack), [ (c->name,c->lastComputationResult) for c in stack ];
	};

	auto getNextParts = [](wires) {
		res = [];
		for (wire : wires) res += getOutParts(wire);
		return res;
	};

	auto getNextWires = [](parts) {
		res = [];
		for (part : parts) res += getOutWires(part);
		return res;
	};

	auto cleanStack = [](stack) {
		stack2 = [];
		for (item : reversed(stack) {
			if (not item in stack2) ;
				stack2 = [item]+stack2;
			}
		};
		return stack2;
	};

	auto getCompileUnitEvalStack = [](cu) {
		nextWires = getPowerWires(cu);
		stack = nextWires;

		while (len(nextWires) {
			nextParts = getNextParts(nextWires);
			nextWires = getNextWires(nextParts);
			stack += nextParts + nextWires;
		};

		return cleanStack(stack)	;
	};

	auto computeWire = [](wire) {
		if (wire->powerrail ) {
			wire->lastComputationResult = 1;
			return;
		};

		Imax = 0;
		for (ID : wire->inputs ) {
			if (ID in wire->cu->parts) ;
				part = wire->cu->parts[ID];
				Imax = max(Imax,part->lastComputationResult);
			}
		};
		wire->lastComputationResult = Imax;
	};

	auto computePart = [](part) {
		Imax = 0;
		for (ID : part->inputs ) {
			if (ID in part->cu->wires) ;
				wire = part->cu->wires[ID];
				Imax = max(Imax,wire->lastComputationResult);
			}
		};
		if (part->isOperand()) Imax = part->computeOperandOutput(Imax);
		if (part->isBlock()) Imax = part->computeBlockOutput(Imax);
		part->lastComputationResult = Imax;
	};

	doContinue = True;

	while (doContinue ) {
		doContinue = False	;
		for (ID, cu : compileUnits ) {
			stack = getCompileUnitEvalStack(cu) ; // only once;

			if (not doContinue ) {
				state1 = [ item->lastComputationResult for item in stack ];
			};

			for (item : stack ) {
				if (item->name == "wire") computeWire(item);
				else: computePart(item);
			};

			if (not doContinue ) {
				state2 = [ item->lastComputationResult for item in stack ];
				for (s1,s2 : zip(state1, state2) {
					if (s1 != s2 ) {
						doContinue = True;
						break;
					}
				}
			}
		}
	}

	LADvizUpdate();*/
}
