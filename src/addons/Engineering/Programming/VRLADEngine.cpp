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


VRLADEngine::Component::Component(string ID, CompileUnitPtr cu, string name) : ID(ID), name(name), cu(cu) {}
VRLADEngine::Part::Part(string ID, CompileUnitPtr cu, string name) : Component(ID,cu,name) {}
VRLADEngine::Wire::Wire(string ID, CompileUnitPtr cu) : Component(ID,cu,"wire") {}

VRLADEngine::Component::~Component() {}
VRLADEngine::Part::~Part() {}
VRLADEngine::Wire::~Wire() {}

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
    //cout << "VRLADEngine::Part::getVariables ops: " << ops.size();
    for (auto o : ops) {
        auto op = o.first;
        bool isIn = o.second;
        //cout << " op(" << op->accessID;
        if (op && cu->accesses.count(op->accessID)) {
            auto opAccess = cu->accesses[op->accessID];
            //cout << ", opAccess " << opAccess->variable;
            if (cu->variables.count(opAccess->variable)) {
                res.push_back( make_pair(cu->variables[opAccess->variable], isIn) );
            } else if (opAccess->constant) {
                auto v = VRLADVariable::create();
                v->setValue( opAccess->components[0] );
                res.push_back( make_pair(v, isIn) );
            }
        }
        //cout << ")";
    }
    //cout << endl;
    return res;
}

pair<VRLADVariablePtr, bool> VRLADEngine::Part::getVariable(int i) {
    auto vs = getVariables();
    if (i < 0) i += vs.size();
    if (vs.size() == 0) return make_pair<VRLADVariablePtr, bool>(0,0);
    if (i < 0 || i >= int(vs.size())) return make_pair<VRLADVariablePtr, bool>(0,0);
    return vs[i];
}

bool VRLADEngine::Part::isOperand() {
    static vector<string> operandTypes = {"Contact", "PContact", "Coil", "RCoil", "Ge", "Lt"};
    return has(operandTypes, name);
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

void VRLADEngine::Wire::addInput(string ID) {
    inputs.push_back(ID);
    if (cu->parts.count(ID)) cu->parts[ID]->outputs.push_back(this->ID);
}

void VRLADEngine::Wire::addOutput(string ID) {
    outputs.push_back(ID);
    if (cu->parts.count(ID)) cu->parts[ID]->inputs.push_back(this->ID);
}

void VRLADEngine::Wire::addOperand(string ID) {
    operand = ID;
    if (cu->parts.count(ID)) cu->parts[ID]->operands.push_back(this->ID);
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
	string folder = "MA_Thesis/03_TIA_Portal/Gumball_Line_180117_V14.ap14/Extruder_Machine/";

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
			for (auto name : tag->getChildren("Name", true)) {
				variable->setName(name->getText());
			}
			for (auto logicalAddress : tag->getChildren("LogicalAddress", true)) {
				variable->setLogicalAddress(logicalAddress->getText());
			}
			for (auto dataType : tag->getChildren("DataTypeName", true)) {
				variable->setDataType(dataType->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}

		// program internal variables;
		XML procTable;
		procTable.read(folder+"Programmbausteine/003_Process_Data.xml");
		auto rootProcTable = procTable.getRoot();
		for (auto member : rootProcTable->getChildren("Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("internal");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			esystem->addVariable(variable->getName(), variable);
			//cout << "  aaa " << variable->getName() << ", " << member->toString() << endl;
			//for (auto c : member->getChildren()) cout << "       child: " << c->toString() << endl;
		}

		// hmi variables
		XML hmiTable;
		hmiTable.read(folder+"Programmbausteine/005_HMI_Data.xml")	; //HMI_Ext_Start mit Datatype;
		auto rootHmiTable = hmiTable.getRoot();
		for (auto member : rootHmiTable->getChildren("Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("hmi");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
			for (auto startValue : member->getChildren("StartValue", true)) {
				variable->setStartValue(startValue->getText());
				variable->setValue(startValue->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}

		// Alarms
		XML alarmsTable;
		alarmsTable.read(folder+"Programmbausteine/004_Alarms_Data.xml");
		auto rootAlarmsTable = alarmsTable.getRoot();
		for (auto member : rootAlarmsTable->getChildren("Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("alarm");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			esystem->addVariable(variable->getName(), variable);
		}

		//VFD Control Block;
		/*treeTagtable = ET->parse(folder+"Programmbausteine/003_VFD_Control_G120C_MM->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter("Member") {
			variable = LADVariable();
			variable->setSource("VFD");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			esystem->addVariable(variable->getName(), variable);
		}*/

		// VFD PAW
		XML pawTable;
		pawTable.read(folder+"Programmbausteine/003_VFD_PAW.xml")	;
		auto rootPawTable = pawTable.getRoot();
		for (auto member : rootPawTable->getChildren("Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("vfd");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
			for (auto startValue : member->getChildren("StartValue", true)) {
				variable->setStartValue(startValue->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}

		//VFD PEW
		XML pewTable;
		pewTable.read(folder+"Programmbausteine/003_VFD_PEW.xml")	;
		auto rootPewTable = pewTable.getRoot();
		for (auto member : rootPewTable->getChildren("Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource("vfd");
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
			if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
			for (auto startValue : member->getChildren("StartValue", true)) {
				variable->setStartValue(startValue->getText());
			}
			esystem->addVariable(variable->getName(), variable);
		}
	};

    XML processTable;
    processTable.read(folder+"Programmbausteine/003_Process.xml"); // HMI_Ext_Start + HMI_Ext_Start_M mit powered wires;

	// get compile units;
	auto getCompileUnits = [&]() {
		vector<XMLElementPtr> res;
		for (auto FC : processTable.getRoot()->getChildren("SW.Blocks.FC")) {
			for (auto objectList : FC->getChildren("ObjectList")) {
				for (auto compileUnit : objectList->getChildren("SW.Blocks.CompileUnit")) {
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
                    for (auto flgNet : networkSource->getChildren("FlgNet")) {
                        for (auto parts : flgNet->getChildren("Parts")) {
                            for (auto access : parts->getChildren("Access")) raccesses.push_back(access);
                            for (auto part : parts->getChildren("Part")) rparts.push_back(part);
                        }
                        for (auto wires : flgNet->getChildren("Wires")) {
                            for (auto wire : wires->getChildren("Wire")) rwires.push_back(wire);
                        }
                    }
                }
            }

			auto compileUnit = CompileUnitPtr( new CompileUnit(compileUnitNode->getAttribute("ID")) );
			compileUnit->variables = variables;

			for (auto partNode : rparts ) {
				auto part = PartPtr( new Part(partNode->getAttribute("UId"), compileUnit, partNode->getAttribute("Name")) );
				if (partNode->getChild("Negated")) part->negated = true;
				compileUnit->parts[part->ID] = part;
			}

			for (auto accessNode : raccesses ) {
				auto UId = accessNode->getAttribute("UId");
				auto access = AccessPtr( new Access(UId, compileUnit));
				auto Symbol = accessNode->getChild("Symbol");
				if (Symbol) {
					for (auto component : Symbol->getChildren("Component")) {
						auto name = component->getAttribute("Name");
						access->components.push_back(name);
						if (variables.count(name)) access->variable = name;
					}
				}

				auto Constant = accessNode->getChild("Constant"); //if there are constant accesses;
				if (Constant) {
					access->constant = true;
					for (auto value : Constant->getChildren("ConstantValue")) {
						access->components.push_back(value->getText());
					}
					for (auto value : Constant->getChildren("ConstantType")) {
						access->components.push_back(value->getText());
					}
				}

				compileUnit->accesses[access->ID] = access;
			}
			compileUnits[compileUnit->ID] = compileUnit;

			//cout << "    ----------- cu " << compileUnit->ID << ", N wires: " << rwires.size() << endl;

			for (auto wireNode : rwires ) {
				auto wire = WirePtr( new Wire(wireNode->getAttribute("UId"), compileUnit) );
				auto identCon = wireNode->getChild("IdentCon");
				if (identCon) wire->accessID = identCon->getAttribute("UId");
				auto powerrail = wireNode->getChild("Powerrail");
				if (powerrail) {
					wire->powerrail = true;
					compileUnit->poweredWireIDs.push_back(wire->ID);
				}
				for (auto nameCon : wireNode->getChildren("NameCon")) {
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
                //cout << "      add wire " << wire->ID << endl;
				compileUnit->wires[wire->ID] = wire;
			}
		}

		return compileUnits;
	};

	getVariables();
	compileUnits = setupCompileUnits(esystem->getLADVariables());

	//Test all variables for start function;
	unit2E = getCompileUnit("2E");
	//unit2E = compileUnits["2E"];
	if (unit2E) {
        unit2E->setVariable("Button_Ext_Stop", "1"); // schalter am extruder, info muss aus ECAD kommen, E9->6;
        unit2E->setVariable("Prc_Ext_Ok", "1"); // viele inputs aus ECAD, E9->1, E9->2, E9->3, E9->6, E9->7, DB2->DBX4->0;
        //unit2E->variables["Button_Ext_Stop"]->setValue("1") ; // schalter am extruder, info muss aus ECAD kommen, E9->6;
        //unit2E->variables["Prc_Ext_Ok"]->setValue("1") ; // viele inputs aus ECAD, E9->1, E9->2, E9->3, E9->6, E9->7, DB2->DBX4->0;
	}
}

VRLADEngine::CompileUnitPtr VRLADEngine::getCompileUnit(string name) {
    if (!compileUnits.count(name)) {
        cout << "WARNING in VRLADEngine::getCompileUnit, no compile unit found named " << name << endl;
        return 0;
    }
	return compileUnits[name];
}

void VRLADEngine::CompileUnit::setVariable(string var, string val) {
    if (!variables.count(var)) {
        cout << "WARNING in VRLADEngine::CompileUnit::setVariable, no variable found called " << var << endl;
        return;
    }
	auto V = variables["Button_Ext_Stop"];
	if (V) V->setValue("1");
	else cout << "WARNING in VRLADEngine::CompileUnit::setVariable, variable " << var << " is null" << endl;
}

void VRLADEngine::iterate() { // TODO: once the import is finished tackle this
	auto getOutParts = [](WirePtr wire) {
		vector<PartPtr> res;
		for (auto ID : wire->outputs ) {
			if (wire->cu->parts.count(ID)) res.push_back(wire->cu->parts[ID]);
		};
		return res;
	};

	auto getOutWires = [](PartPtr part) {
		vector<WirePtr> res;
		for (auto ID : part->outputs ) {
			if (part->cu->wires.count(ID)) res.push_back(part->cu->wires[ID]);
		};
		return res;
	};

	auto getPowerWires = [](CompileUnitPtr cu) {
		vector<WirePtr> res;
		for (auto wireID : cu->poweredWireIDs) {
            if (cu->wires.count(wireID)) res.push_back( cu->wires[wireID] );
		}
		return res;
	};

	auto printStack = [](vector<ComponentPtr> stack) {
	    cout << "   stack size: " << stack.size() << endl;
	    for (auto c : stack) {
            auto w = dynamic_pointer_cast<Wire>(c);
            auto p = dynamic_pointer_cast<Part>(c);
            cout << "    (" << c->name << ", " << c->ID << ", " << c->lastComputationResult << ", cu: " << c->cu->ID << ")";
            if (w) cout << ", wire: " << w->operand << ", " << w->accessID;
            if (p) {
                cout << ", part - operands: ";
                for (auto o : p->operands) cout << ", " << o << " check wire: " << p->cu->wires.count(o);
                cout << ", variables: ";
                for (auto v : p->getVariables()) cout << ", (" << v.first->getName() << ", " << v.second << ") ";
            }
            cout << endl;
	    }
	};

	auto getNextParts = [&](vector<WirePtr> wires) {
		vector<PartPtr> res;
		for (auto wire : wires) {
            for (auto part : getOutParts(wire)) {
                res.push_back(part);
            }
		}
		return res;
	};

	auto getNextWires = [&](vector<PartPtr> parts) {
		vector<WirePtr> res;
		for (auto part : parts) {
            for (auto wire : getOutWires(part)) {
                res.push_back(wire);
            }
		}
		return res;
	};

	auto cleanStack = [](vector<ComponentPtr> stack) {
		vector<ComponentPtr> stack2; // filter redundant in stack
		for (int i=stack.size()-1; i>= 0; i--) {
            auto item = stack[i];
			if (!has(stack2,item)) {
				stack2.push_back(item);
			}
		}

		vector<ComponentPtr> stack3; // reverse stack2
		for (int i=stack2.size()-1; i>= 0; i--) {
            stack3.push_back(stack2[i]);
		}
		return stack3;
	};

	auto getCompileUnitEvalStack = [&](CompileUnitPtr cu) {
		auto nextWires = getPowerWires(cu);
		vector<ComponentPtr> stack;
        for (auto w : nextWires) stack.push_back(w);

		while (nextWires.size()) {
			auto nextParts = getNextParts(nextWires);
			nextWires = getNextWires(nextParts);
			for (auto p : nextParts) stack.push_back(p);
			for (auto w : nextWires) stack.push_back(w);
		}

		return cleanStack(stack);
	};

	auto computeWire = [](WirePtr wire) {
		if (wire->powerrail) {
			wire->lastComputationResult = 1;
			return;
		}

		int Imax = 0;
		for (auto ID : wire->inputs) {
			if (wire->cu->parts.count(ID)) {
				auto part = wire->cu->parts[ID];
				Imax = max(Imax,part->lastComputationResult);
			}
		}
		wire->lastComputationResult = Imax;
	};

	auto computePart = [](PartPtr part) {
		int Imax = 0;
		for (auto ID : part->inputs) {
			if (part->cu->wires.count(ID)) {
				auto wire = part->cu->wires[ID];
				Imax = max(Imax,wire->lastComputationResult);
			}
		}
		if (part->isOperand()) Imax = part->computeOperandOutput(Imax);
		if (part->isBlock()) Imax = part->computeBlockOutput(Imax);
		part->lastComputationResult = Imax;
	};

	bool doContinue = true;

	cout << "VRLADEngine::iterate" << endl;

	while (doContinue) {
		doContinue = false;
        cout << " VRLADEngine::subiterate" << endl;
		for (auto cu : compileUnits) {
			auto stack = getCompileUnitEvalStack(cu.second); // only once
			vector<int> state1;
			vector<int> state2;

			//cout << "  iterate compile unit: " << cu.first << endl;
			//printStack(stack);

			if (!doContinue) {
                for (auto item : stack) state1.push_back( item->lastComputationResult );
			}

			for (ComponentPtr item : stack) {
                auto wire = dynamic_pointer_cast<Wire>(item);
                auto part = dynamic_pointer_cast<Part>(item);
				if (wire) computeWire(wire);
				if (part) computePart(part);
			}

			if (!doContinue) {
                for (auto item : stack) state2.push_back( item->lastComputationResult );

                int N = min(state1.size(), state2.size());
                for (int i=0; i<N; i++) {
					if (state1[i] != state2[i]) {
                        cout << " --> state change " << i << ", " << stack[i]->name << ", " << stack[i]->lastComputationResult << endl;
						doContinue = true;
						break;
					}
				}
			}
		}
	}

	//LADvizUpdate();
}
