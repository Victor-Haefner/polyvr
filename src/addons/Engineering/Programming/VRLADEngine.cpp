#include "VRLADEngine.h"
#include "VRLADVariable.h"

#include "../Wiring/VRElectricSystem.h"

#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/utils/system/VRSystem.h"

#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/tools/VRAnnotationEngine.h"

#include <functional>
#include <algorithm>

using namespace OSG;

VRLADEngine::VRLADEngine() {}
VRLADEngine::~VRLADEngine() {}

VRLADEnginePtr VRLADEngine::create() { return VRLADEnginePtr( new VRLADEngine() ); }
VRLADEnginePtr VRLADEngine::ptr() { return static_pointer_cast<VRLADEngine>(shared_from_this()); }

void VRLADEngine::setElectricEngine(VRElectricSystemPtr e) { esystem = e; }

vector<string> VRLADEngine::getCompileUnits() {
    vector<string> res;
    for (auto c : compileUnits) res.push_back(c.first);
    return res;
}

vector<string> VRLADEngine::getCompileUnitWires(string cuID, bool powered) {
    vector<string> res;
    if (!compileUnits.count(cuID)) return res;
    auto cu = compileUnits[cuID];
    if (!powered) for (auto w : cu->wires) res.push_back(w.first);
    else for (auto w : cu->poweredWireIDs) if (cu->wires.count(w)) res.push_back(w);
    return res;
}

vector<string> VRLADEngine::getCompileUnitParts(string cuID) {
    vector<string> res;
    if (!compileUnits.count(cuID)) return res;
    auto cu = compileUnits[cuID];
    for (auto w : cu->parts) res.push_back(w.first);
    return res;
}

vector<string> VRLADEngine::getCompileUnitVariables(string cuID) {
    vector<string> res;
    if (!compileUnits.count(cuID)) return res;
    auto cu = compileUnits[cuID];
    for (auto w : cu->variables) res.push_back(w.first);
    return res;
}

vector<string> VRLADEngine::getCompileUnitWireOutParts(string cuID, string wID) {
    vector<string> res;
    if (!compileUnits.count(cuID)) return res;
    auto cu = compileUnits[cuID];
    if (!cu->wires.count(wID)) return res;
    auto wire = cu->wires[wID];
    for (auto ID : wire->outputs) {
        if (cu->parts.count(ID)) res.push_back(ID);
    }
    return res;
}

vector<string> VRLADEngine::getCompileUnitPartOutWires(string cuID, string pID) {
    vector<string> res;
    if (!compileUnits.count(cuID)) return res;
    auto cu = compileUnits[cuID];
    if (!cu->parts.count(pID)) return res;
    auto part = cu->parts[pID];
    for (auto ID : part->outputs) {
        if (cu->wires.count(ID)) res.push_back(ID);
    }
    return res;
}

int VRLADEngine::getCompileUnitWireSignal(string cuID, string wID) {
    if (!compileUnits.count(cuID)) return 0;
    auto cu = compileUnits[cuID];
    if (!cu->wires.count(wID)) return 0;
    return cu->wires[wID]->lastComputationResult;
}

VRLADVariablePtr VRLADEngine::getCompileUnitPartVariable(string cuID, string pID) {
    if (!compileUnits.count(cuID)) return 0;
    auto cu = compileUnits[cuID];
    if (!cu->parts.count(pID)) return 0;
    return cu->parts[pID]->getVariable().first;
}

string VRLADEngine::getCompileUnitPartName(string cuID, string pID) {
    if (!compileUnits.count(cuID)) return 0;
    auto cu = compileUnits[cuID];
    if (!cu->parts.count(pID)) return 0;
    return cu->parts[pID]->name;
}

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

int VRLADEngine::Part::computeOperandOutput(int value, bool verbose) {
    auto variable = getVariable().first;

    if (!variable) {
        if (verbose) cout << "Warning: " << name << " has no variable!" << endl;
        return value;
    }

    if (verbose) cout << "computeOperandOutput, name: " << name << " " << variable << endl;

    if (name == "Coil" ) { // writes to variable
        variable->setValue(::toString(value));
        return 0;
    }

    if (name == "RCoil" ) { // writes to variable
        variable->setValue(variable->getStartValue());
        return 0;
    }

    if (value == 0) return 0;

    int var = toFloat(variable->getValue());

    if (name == "Ge" ) { // greater equal check
        auto vars = getVariables();
        if (vars.size() < 2) return 0;
        float v1 = toFloat(vars[0].first->getValue());
        float v2 = toFloat(vars[1].first->getValue());
        return v1 >= v2;
    }

    if (name == "Contact" || name == "PContact" ) {
        if (verbose) cout << " compute Contact, negated: " << negated << ", var: " << var << ", svar: " << variable->getValue() << endl;
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
        if (inputs.size() < 2) return 0;
        float in1 = toFloat(inputs[0]->getValue());
        float in2 = toFloat(inputs[1]->getValue());
        if (outputs.size() < 1) return 0;
        outputs[0]->setValue( ::toString(in1 * in2) );
        return 1;
    }

    if (name == "Move" ) {
        if (inputs.size() < 1) return 0;
        if (outputs.size() < 1) return 0;
        outputs[0]->setValue( inputs[0]->getValue() );
        return 1;
    }

    if (name == "Round") { // TODO
        if (inputs.size() < 1) return 0;
        if (outputs.size() < 1) return 0;
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

void VRLADEngine::read(string tagTablePath, string modulesPath) {
	auto readVariables = [&](string path, string source) {
		XML xml;
		xml.read(path);
		auto root = xml.getRoot();
		for (auto member : root->getChildren("Member", true)) {
			auto variable = VRLADVariable::create();
			variable->setSource(source);
			variable->setName(member->getAttribute("Name"));
			variable->setDataType(member->getAttribute("Datatype"));
            if (member->hasAttribute("Remanence")) variable->setRemanence(member->getAttribute("Remanence"));
            for (auto startValue : member->getChildren("StartValue", true)) {
                variable->setStartValue(startValue->getText());
                variable->setValue(startValue->getText());
            }
			esystem->addVariable(variable->getName(), variable);
		}
	};

	auto getVariables = [&]() {
		// hardware variables, pins, sockets, etc..
		XML treeTagtable;
		treeTagtable.read(tagTablePath); // HMI_Ext_Start_M mit Adresse
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

        for (auto f : openFolder(modulesPath)) {
            readVariables(modulesPath+"/"+f, getFileName(f, false));
        }
	};

	auto setupCompileUnit = [&](XMLElementPtr compileUnitNode, string module, map<string, VRLADVariablePtr>& variables) {
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

        string cuID = module + "." + compileUnitNode->getAttribute("ID");
        auto compileUnit = CompileUnitPtr( new CompileUnit(cuID) );
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

        //cout << "    ----------- cu " << compileUnit->ID << ", N wires: " << rwires.size() << ", check ID in dict: " << compileUnits.count(compileUnit->ID) << endl;
        compileUnits[compileUnit->ID] = compileUnit;

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

	};

	// get compile units
	auto setupCompileUnits = [&](map<string, VRLADVariablePtr>& variables) {
		for (auto f : openFolder(modulesPath)) {
            XML xml;
            xml.read(modulesPath+"/"+f);
            string module = getFileName(f, false);

            for (auto FC : xml.getRoot()->getChildren("SW.Blocks.FC")) {
                for (auto objectList : FC->getChildren("ObjectList")) {
                    for (auto compileUnit : objectList->getChildren("SW.Blocks.CompileUnit")) {
                        setupCompileUnit(compileUnit, module, variables);
                    }
                }
            }
        }
	};

	getVariables();
	auto vars = esystem->getLADVariables();
	setupCompileUnits(vars);
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
	auto V = variables[var];
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

		//if (wire->lastComputationResult != Imax) cout << wire->cu->ID << " toggle wire: " << wire->ID << " -> " << Imax << endl;
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
		/*if (part->lastComputationResult != Imax) {
            cout << part->cu->ID << " toggle part: " << part->ID << " -> " << Imax;
            for (auto v : part->getVariables()) cout << ", v: " << v.first->getName() << "," << v.second;
            cout << endl;
		}*/
		part->lastComputationResult = Imax;
	};

	bool doContinue = true;

	while (doContinue) {
		doContinue = false;
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
                        //cout << " --> state change " << i << ", " << stack[i]->name << ", " << stack[i]->lastComputationResult << endl;
						doContinue = true;
						break;
					}
				}
			}
		}
	}

	//LADvizUpdate();
}

VRTransformPtr VRLADEngine::addVisual() {
    if (ladViz) return ladViz;

	double L = 0.1;
	double F = 0.015;
	double D = 0.15;

	double W = 1;
	double H = 2.4;
	double S = 0.5 ; // 0.2;

	Vec3d p0(-1.5,0,0);

	ladViz = VRTransform::create("ladViz");

	auto lines = VRGeometry::create("lines");
	ladViz->addChild(lines);
	auto labels = VRAnnotationEngine::create("labels");
	labels->setSize(F);
	labels->setBackground(Color4f(1,1,1,0));
	labels->setOutline(5, Color4f(0,0,0,1));
	ladViz->addChild(labels);

	auto m = VRMaterial::create("lm");
	m->setLit(0);
	m->setLineWidth(2);
	m->setPointSize(4);
	lines->setMaterial(m);

	auto bg = VRGeometry::create("bg");
	bg->setPrimitive("Plane 6 3.5 1 1");
	bg->setTransform(Vec3d(1.2,-1.5,-0.02), Vec3d(0,0,-1), Vec3d(0,1,0));
	bg->setMaterial(m);
	ladViz->addChild(bg);

	VRGeoData ldata(lines);

	auto addLine = [&](Pnt3d p1, Pnt3d p2, Color3f c) {
		Vec3d n(0,1,0);
		ldata.pushVert(p1,n,c);
		ldata.pushVert(p2,n,c);
		ldata.pushLine();
	};

	auto addPoint = [&](Pnt3d p, Color3f c) {
		Vec3d n(0,1,0);
		ldata.pushVert(p,n,c);
		ldata.pushPoint();
	};

	auto getOutParts = [&](string cuID, string wID) {
		return getCompileUnitWireOutParts(cuID, wID);
	};

	auto getOutWires = [&](string cuID, string pID) {
		return getCompileUnitPartOutWires(cuID, pID);
	};

	auto getPowerWires = [&](string cuID) {
		return getCompileUnitWires(cuID, true);
	};

	map<string, Pnt3d> partMap;
	vector<string> drawnComponent;
	int lblID = 0;

	function<void(string, string, Pnt3d)> computeNextPartsPosition = [&](string cuID, string part, Pnt3d pos) {
	    auto wires = getOutWires(cuID, part);
		for (int j = 0; j< wires.size(); j++) {
            auto wire2 = wires[j];
		    auto parts = getOutParts(cuID, wire2);
            for (int k = 0; k<parts.size(); k++) {
                auto part2 = parts[k];
				Pnt3d pos2 = pos + Vec3d(L,0,0);
				if (partMap.count(part2)) {
					pos2 = partMap[part2];
					if (pos2[0] <= pos[0]) pos2[0] += L;
				}
				partMap[part2] = pos2;
				computeNextPartsPosition(cuID, part2, pos2);
			}
		}
	};

	auto computePartPositions = [&](string cuID, Pnt3d p0) {
		partMap["-1"] = p0;
		for (auto wire : getPowerWires(cuID) ) {
            auto parts = getOutParts(cuID, wire);
			for (int i = 0; i < parts.size(); i++ ) {
			    auto part = parts[i];
				Pnt3d pos = p0 + Vec3d(L, -i*D-0.05, 0);
				partMap[part] = pos;
				computeNextPartsPosition(cuID, part, pos);
			}
		}
	};

	auto drawConnection = [&](string cuID, string wID, string p1, string p2, int k = 0) {
		// v = wire->lastComputationResult;
		int v = getCompileUnitWireSignal(cuID, wID);
		Color3f c(1,0,0);
		if (v == 1) c = Color3f(0,1,0);

		Pnt3d f;
		if (!partMap.count(p1)) f = partMap["-1"];
		else f = partMap[p1];
		Pnt3d t = partMap[p2];
		Vec3d d = t-f;
		Pnt3d l;
		if (k == 0) l = f+Vec3d(d[0],0,0);
		if (k == 1) l = f+Vec3d(0,d[1],0);
		addLine(f, l, c);
		addLine(l, t, c);
	};

	auto drawPart = [&](string cuID, string pID) {
		if (count(drawnComponent.begin(), drawnComponent.end(), pID)) return;
		drawnComponent.push_back(pID);

		VRLADVariablePtr v = getCompileUnitPartVariable(cuID, pID);
		string n = getCompileUnitPartName(cuID, pID);
		//v = p->getVariable()[0];
		if (v) {
			Pnt3d pos = partMap[pID];
			addPoint(pos, Color3f(0,0,1));
			if (n == "Contact" ) {
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d(-0.01, -0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d( 0.01, 0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,0,0));
			} else if (n == "Coil" ) {
				addLine(pos+Vec3d(-0.006, 0.01, 0), pos+Vec3d(-0.01, 0.003, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(-0.01, 0.003, 0), pos+Vec3d(-0.01, -0.003, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(-0.01, -0.003, 0), pos+Vec3d(-0.006, -0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d( 0.006, 0.01, 0), pos+Vec3d( 0.01, 0.003, 0), Color3f(0,0,0));
				addLine(pos+Vec3d( 0.01, 0.003, 0), pos+Vec3d( 0.01, -0.003, 0), Color3f(0,0,0));
				addLine(pos+Vec3d( 0.01, -0.003, 0), pos+Vec3d( 0.006, -0.01, 0), Color3f(0,0,0));
			} else if (n == "Move" ) {
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d(-0.01, -0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d( 0.01, 0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d( 0.01,  0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(-0.01,-0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,0,0));
			} else if (n == "Calc" ) {
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d(-0.01, -0.01, 0), Color3f(0,1,0));
				addLine(pos+Vec3d( 0.01, 0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,1,0));
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d( 0.01,  0.01, 0), Color3f(0,1,0));
				addLine(pos+Vec3d(-0.01,-0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,1,0));
			} else if (n == "PContact") { // TODO;
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d(-0.01, -0.01, 0), Color3f(1,0,1));
				addLine(pos+Vec3d( 0.01, 0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(1,0,1));
			} else if (n == "RCoil") { // TODO;
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d(-0.01, -0.01, 0), Color3f(0,0,1));
				addLine(pos+Vec3d( 0.01, 0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,0,1));
			}
			else if (n == "SCoil") { // TODO;
				addLine(pos+Vec3d(-0.01, 0.01, 0), pos+Vec3d(-0.01, -0.01, 0), Color3f(0,1,1));
				addLine(pos+Vec3d( 0.01, 0.01, 0), pos+Vec3d( 0.01, -0.01, 0), Color3f(0,1,1));
			} else if (n == "Ge") { // TODO;
				addLine(pos+Vec3d(-0.015, 0.015, 0), pos+Vec3d(-0.015, -0.015, 0), Color3f(0,0,0));
				addLine(pos+Vec3d( 0.015, 0.015, 0), pos+Vec3d( 0.015, -0.015, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(-0.01, 0.015, 0), pos+Vec3d( 0, 0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(-0.01, 0.005, 0), pos+Vec3d( 0, 0.01, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(0, 0.013, 0), pos+Vec3d( 0.01, 0.013, 0), Color3f(0,0,0));
				addLine(pos+Vec3d(0, 0.007, 0), pos+Vec3d( 0.01, 0.007, 0), Color3f(0,0,0));
			} else cout << n << endl;

			auto idParts = splitString(v->getName(), '_');
			for (int i = 0; i<idParts.size(); i++) {
                string l = idParts[i];
                Vec3d p = Vec3d( pos+Vec3d(-0.05,0.015-i*2*F,0.02) );
				labels->set(lblID, p, l);
				lblID += 1;
			}
		}
	};

	function<void(string, string)> drawNextWires = [&](string cuID, string part) {
		drawPart(cuID, part);
		auto wires = getOutWires(cuID, part);
		for (int j = 0; j<wires.size(); j++ ) {
            auto wire = wires[j];
            auto parts = getOutParts(cuID, wire);
			for (int k = 0; k<parts.size(); k++) {
			    auto part2 = parts[k];
				drawConnection(cuID, wire, part, part2);
				drawNextWires(cuID, part2);
			}
		}
	};

	auto drawCompilationUnit = [&](string cuID, Pnt3d p0) {
		computePartPositions(cuID, p0);
		for (auto wire : getPowerWires(cuID) ) {
            auto parts = getOutParts(cuID, wire);
			for (int i = 0; i<parts.size(); i++) {
			    auto part = parts[i];
				drawConnection(cuID, wire, "", part, 1);
				drawNextWires(cuID, part);
			}
		}
	};

	//drawCompilationUnit(compileUnits["2E"], Vec3([-2,0.5,0]));

	auto getHeight = [&]() {
		Vec2d S(1e6,-1e6);
		for (auto& p : partMap) {
            string ID = p.first;
            Pnt3d pos = p.second;
			if (pos[1] < S[0]) S[0] = pos[1];
			if (pos[1] > S[1]) S[1] = pos[1];
		};
		return S[1]-S[0];
	};

	ladViz->setTransform(Vec3d(-22,2,1), Vec3d(-1,0,-1));
	ladViz->setScale(Vec3d(S,S,S));
	for (auto cuID : getCompileUnits()) {
		partMap.clear();
		drawnComponent.clear();
		drawCompilationUnit(cuID, p0);
		double d = getHeight();
		p0 += Vec3d(0,-d-D*0.5,0);
		if (p0[1] < -H) {
			p0[0] += W;
			p0[1] = 0;
		}
	}

	ldata.apply(lines);

    return ladViz;
}

void VRLADEngine::updateVisual() {
	if (!ladViz) return;
	if (!ladViz->isVisible()) return;

	VRGeometryPtr lines = dynamic_pointer_cast<VRGeometry>( ladViz->getChild(0) );
	if (!lines) return;
	VRGeoData ldata(lines);
	size_t i = 0;

	auto addLine = [&](const Color3f& c) {
		ldata.setColor(i  , c);
		ldata.setColor(i+1, c);
		i += 2;
	};

	auto addPoint = [&](const Color3f& c) {
		ldata.setColor(i, c);
		i += 1;
	};

	auto getOutParts = [&](string cuID, string wID) {
		return getCompileUnitWireOutParts(cuID, wID);
	};

	auto getOutWires = [&](string cuID, string pID) {
		return getCompileUnitPartOutWires(cuID, pID);
	};

	auto getPowerWires = [&](string cuID) {
		return getCompileUnitWires(cuID, true);
	};

	vector<string> drawnComponent;

	auto drawConnection = [&](string cuID, string wID) {
		auto v = getCompileUnitWireSignal(cuID, wID);
		Color3f c(1,0,0);
		if (v == 1) c = Color3f(0,1,0);
		addLine(c);
		addLine(c);
	};

	auto drawPart = [&](string cuID, string pID) {
		if (std::find(drawnComponent.begin(), drawnComponent.end(), pID) != drawnComponent.end()) return;
		drawnComponent.push_back(pID);
		auto v = getCompileUnitPartVariable(cuID, pID);
		auto n = getCompileUnitPartName(cuID, pID);
		if (v) {
			addPoint(Color3f(0,0,1));
			if (n == "Contact" ) {
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
			} else if ( n == "Coil" ) {
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
			} else if ( n == "Move" ) {
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
			} else if ( n == "Calc" ) {
				addLine(Color3f(0,1,0));
				addLine(Color3f(0,1,0));
				addLine(Color3f(0,1,0));
				addLine(Color3f(0,1,0));
			} else if ( n == "PContact" ) { // TODO;
				addLine(Color3f(1,0,1));
				addLine(Color3f(1,0,1));
			} else if ( n == "RCoil" ) { // TODO;
				addLine(Color3f(0,0,1));
				addLine(Color3f(0,0,1));
			} else if ( n == "SCoil" ) { // TODO;
				addLine(Color3f(0,1,1));
				addLine(Color3f(0,1,1));
			} else if ( n == "Ge" ) { // TODO;
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
				addLine(Color3f(0,0,0));
			}
		}
	};

	function<void(string, string)> drawNextWires = [&](string cuID, string part) {
		drawPart(cuID, part);
		for (auto wire : getOutWires(cuID, part)) {
			for (auto part2 : getOutParts(cuID, wire)) {
				drawConnection(cuID, wire);
				drawNextWires(cuID, part2);
			}
		}
	};

	auto drawCompilationUnit = [&](string cuID) {
		for (auto wire : getPowerWires(cuID)) {
			for (auto part : getOutParts(cuID, wire)) {
				drawConnection(cuID, wire);
				drawNextWires(cuID, part);
			}
		}
	};

	for (auto cuID : getCompileUnits()) {
		drawnComponent.clear();
		drawCompilationUnit(cuID);
	}
}
