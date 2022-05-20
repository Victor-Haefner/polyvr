#include "VRLADEngine.h"

using namespace OSG;

VRLADEngine::VRLADEngine() {}
VRLADEngine::~VRLADEngine() {}

VRLADEnginePtr VRLADEngine::create() { return VRLADEnginePtr( new VRLADEngine() ); }
VRLADEnginePtr VRLADEngine::ptr() { return static_pointer_cast<VRLADEngine>(shared_from_this()); }

void VRLADEngine::read() {
	/*import VR;
	import xml->etree->ElementTree as ET;

	namespace = "{http://www->siemens->com/automation/Openness/SW/NetworkSource/FlgNet/v1}";
	namespace2 = "{http://www->siemens->com/automation/Openness/SW/Interface/v2}";
	folder = "MA_Thesis/03_TIA_Portal/Gumball_Line_180117_V14->ap14/Extruder_Machine/";

	verbose = False ; //set True for tick-prints;
	; //verbose = True;

	void getTag(node) ) {
//		if (not hasattr(node, "tag")) return "noTag";
		tag = node->tag;
		if (tag[0] == "{") tag = tag->split("}")[1];
		return tag;
	};

	void printNode(node, depth = -1, padding = "") ) {
		print padding, getTag(node), node->attrib;
		if (depth == 0) return;
		for (child : node ) {
			printNode(child, depth-1, padding+" ")		;
		}
	};

	class Variable ) {
		void __init__(self) ) {
			name = "";
			logicalAddress = "";
			dataType = "";
			value = 0;
			startValue = 0;
			source = "";
			remanence = "";
		};

		void __repr__(self) ) {
			return "Name:  "+str(name)+"  Address: "+str(logicalAddress)+"  Type: "+str(dataType)+"  StartValue: "+str(startValue)+"  value: "+str(value);
		}
	};


	class CompileUnit ) {
		void __init__(ID) ) {
			ID = ID;
			parts = {};
			wires = {};
			accesses = {};
			variables = {};
			poweredWireIDs = [];
		};

		void __repr__(self) ) {
			s = "CompileUnit: "+str(ID);
			s += "\n";
			for (ID, c : parts) s += ", "+str(c);
			s += "\n";
			for (ID, c : wires) s += ", "+str(c);
			s += "\n";
			for (ID, c : accesses) s += ", "+str(c);
			return s;
		}
	};

	class Part ) {
		void __init__(ID, cu, name) ) {
			ID = ID;
			cu = cu;
			name = name;
			inputs = [];
			outputs = [];
			operands = [];
			negated = False;
			lastComputationResult = 0;
		};

		void getOperands(self) ) {
			res = [];
			for (o : operands ) {
				if (o in cu->wires ) {
					isIn = o in inputs;
					res->append( (cu->wires[o],isIn) );
				}
			};
			return res;
		};

		void getVariables(self) ) {
			ops = getOperands();
			res = [];
			for (op,isIn : ops ) {
				if (op && op->accessID in cu->accesses ) {
					opAccess = cu->accesses[op->accessID];
					if (opAccess->variable in cu->variables ) {
						res->append( (cu->variables[opAccess->variable], isIn) );
					};
					elif opAccess->constant: ;
						v = LADVariable();
						v->setValue( opAccess->components[0] );
						res->append( (v, isIn) );
					}
				}
			};
			return res;
		};

		void getVariable(i = -1) ) {
			vs = getVariables();
			if (vs == []) return (None,None);
			return vs[i];
		};

		void isOperand(self) ) {
			return name in ["Contact", "PContact", "Coil", "RCoil", "Ge", "Lt"];
		};

		void isBlock(self) ) {
			return name in ["Calc", "Move", "Round"];
		};

		void computeOperandOutput(value = 0) ) {
			variable = getVariable()[0];
			if (not variable ) {
				; //print "Warning:", name, "has no variable!";
				return value;
			};

			if (name == "Coil" ) {
				; //TODO: resolve this workaround!;
				if (variable->getName() == "Prc_Ext_Ok") return 0;

				if (not variable->getValue() == value ) {
					if (not variable->getName() in ["SS_CMD_Hopper", "Sft_Ext_Block"] ) {
						print "LAD set var:", variable->getValue(), "->", value, variable->getName();
					}
				};
				variable->setValue(value);
				return 0;
			};

			if (name == "RCoil" ) {
				if (not variable->getValue() == variable->getStartValue() ) {
					print "LAD reset", variable->getValue(), "->", variable->getStartValue(), variable->getName();
				};
				variable->setValue(variable->getStartValue());
				return 0;
			};

			if (value == 0) return 0;

			var = float(variable->getValue());
			if (name == "Contact" || name == "PContact" ) {
				if (negated && var == 0) return value ;
				if (negated && var == 1) return 0;
				return var*value;
			};
			if (verbose) print "Warning! computeOutput not handled!";
			return 0;
		};

		void computeBlockOutput(value = 0) ) {
			if (value == 0) return 0;

			inputs = [];
			outputs = [];
			for (v,isIn : getVariables() ) {
				if (isIn) inputs->append(v);
				else: outputs->append(v);
			};

			if (name == "Calc" ) {
				; // berechnet anhand der input "operanden" und schreibt in output "operand";
				; // get function from data!;

				; // hard coded test!;
				outputs[0]->setValue( str(float(inputs[0]->getValue()) * float(inputs[1]->getValue())) );
				return 1;
			};

			if (name == "Move" ) {
				outputs[0]->setValue( inputs[0]->getValue() );
				return 1;
			};

			if (name == "Round") ; // TODO;
				outputs[0]->setValue( inputs[0]->getValue() );
				return 1;
			}
		};

		void __repr__(self) ) {
			s = "Part: "+str(ID)+" "+name+" Op:"+str(operands)+" Neg:"+str(negated)+" [";
			for (c : inputs) s += " "+c;
			s += " -> ";
			for (c : outputs) s += " "+c;
			s += "]";
			return s;
		}
	};

	class Access ) {
		void __init__(ID, cu) ) {
			ID = ID;
			cu = cu;
			components = [];
			variable = None;
			constant = False;
		};

		void __repr__(self) ) {
			s = "Access: "+str(ID);
			for (c : components) s += ", "+c;
			return s;
		}
	};

	class Wire ) {
		void __init__(ID, cu) ) {
			name = "wire";
			ID = ID;
			cu = cu;
			inputs = [];
			outputs = [];
			accessID = None;
			operand = None;
			powerrail = False;
			lastComputationResult = 0;
		};

		void addInput(ID) ) {
			inputs->append(ID);
			if (ID in cu->parts) cu->parts[ID]->outputs->append(ID);
		};

		void addOutput(ID) ) {
			outputs->append(ID);
			if (ID in cu->parts) cu->parts[ID]->inputs->append(ID);
		};

		void addOperand(ID) ) {
			operand = ID;
			if (ID in cu->parts) cu->parts[ID]->operands->append( ID );
		};

		void __repr__(self) ) {
			s = "Wire: ID "+str(ID)+" Ac"+str(accessID)+" Op"+str(operand)+" PR"+str(powerrail)+" [";
			for (c : inputs) s += " "+c;
			s += " -> ";
			for (c : outputs) s += " "+c;
			s += "] ";
			return s;
		}
	};


	; // get variables;
	void getVariables():		;
		; // hardware variables, pins, sockets, etc->->;
		treeTagtable = ET->parse(folder+"PLC-Variablen/Default tag table->xml") ; //HMI_Ext_Start_M mit Adresse;
		rootTagTable = treeTagtable->getroot();
		for (tag : rootTagTable->iter("SW->Tags->PlcTag") ) {
			variable = LADVariable();
			variable->setSource("hardware");
			for (name : tag->iter("Name") ) {
				variable->setName(name->text);
			};
			for (logicalAddress : tag->iter("LogicalAddress") ) {
				variable->setLogicalAddress(logicalAddress->text);
			};
			for (dataType : tag->iter("DataTypeName") ) {
				variable->setDataType(dataType->text);
			};
			esystem->addVariable(variable->getName(), variable);
		};

		; // program internal variables;
		treeTagtable = ET->parse(folder+"Programmbausteine/003_Process_Data->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") ) {
			variable = LADVariable();
			variable->setSource("internal");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			esystem->addVariable(variable->getName(), variable);
		};

		; // hmi variables;
		treeTagtable = ET->parse(folder+"Programmbausteine/005_HMI_Data->xml")	; //HMI_Ext_Start mit Datatype;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") ) {
			variable = LADVariable();
			variable->setSource("hmi");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			if ("Remanence" in member->attrib) variable->setRemanence(member->attrib["Remanence"]);
			for (startValue : member->iter(namespace2+"StartValue") ) {
				variable->setStartValue(startValue->text);
				variable->setValue(startValue->text);
			};
			esystem->addVariable(variable->getName(), variable);
		};

		; // Alarms	;
		treeTagtable = ET->parse(folder+"Programmbausteine/004_Alarms_Data->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") ) {
			variable = LADVariable();
			variable->setSource("alarm");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			esystem->addVariable(variable->getName(), variable);
		};

		; //VFD Control Block;
		"""treeTagtable = ET->parse(folder+"Programmbausteine/003_VFD_Control_G120C_MM->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") ) {
			variable = LADVariable();
			variable->setSource("VFD");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			esystem->addVariable(variable->getName(), variable)""";
		};

		; //VFD PAW;
		treeTagtable = ET->parse(folder+"Programmbausteine/003_VFD_PAW->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") ) {
			variable = LADVariable();
			variable->setSource("vfd");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			if ("Remanence" in member->attrib) variable->setRemanence(member->attrib["Remanence"]);
			for (startValue : member->iter(namespace2+"StartValue") ) {
				variable->setStartValue(startValue->text);
			};
			esystem->addVariable(variable->getName(), variable);
		};

		; //VFD PEW;
		treeTagtable = ET->parse(folder+"Programmbausteine/003_VFD_PEW->xml")	;
		rootTagTable = treeTagtable->getroot();
		for (member : rootTagTable->iter(namespace2+"Member") ) {
			variable = LADVariable();
			variable->setSource("vfd");
			variable->setName(member->attrib["Name"]);
			variable->setDataType(member->attrib["Datatype"]);
			if ("Remanence" in member->attrib) variable->setRemanence(member->attrib["Remanence"]);
			for (startValue : member->iter(namespace2+"StartValue") ) {
				variable->setStartValue(startValue->text);
			};
			esystem->addVariable(variable->getName(), variable);
		}
	};


	; // get compile units;
	void getCompileUnits() ) {
		res = [];
		tree = ET->parse(folder+"Programmbausteine/003_Process->xml")  ; // HMI_Ext_Start + HMI_Ext_Start_M mit powered wires;
		for (FC : tree->getroot()->findall("SW->Blocks->FC") ) {
			for (objectList : FC->findall("ObjectList") ) {
				for (compileUnit : objectList->findall("SW->Blocks->CompileUnit") ) {
					res->append(compileUnit);
				}
			}
		};
		return res;
	};

	; // get parts && wires of a compile unit;
	void getPartsAndWires(compileUnit) ) {
		rparts = [];
		rwires = [];
		raccesses = [];
		for (attributeList : compileUnit->findall("AttributeList") ) {
			for (networkSource : attributeList->findall("NetworkSource") ) {
				; // StatementList ?;
				for (flgNet : networkSource->findall(namespace+"FlgNet") ) {
					for (parts : flgNet->findall(namespace+"Parts")) ;
						for (access : parts->findall(namespace+"Access")) raccesses->append(access);
						for (part : parts->findall(namespace+"Part")) rparts->append(part);
					};
					for (wires : flgNet->findall(namespace+"Wires") ) {
						for (wire : wires->findall(namespace+"Wire")) rwires->append(wire);
					}
				}
			}
		};
		return rparts, rwires, raccesses;
	};

	; // get all compile units;
	void setupCompileUnits(variables) ) {
		compileUnits = {};
		for (compileUnitNode : getCompileUnits() ) {
			parts, wires, accesses = getPartsAndWires(compileUnitNode);
			compileUnit = CompileUnit(compileUnitNode->attrib["ID"]);
			compileUnit->variables = variables;

			for (partNode : parts ) {
				part = Part(partNode->attrib["UId"], compileUnit, partNode->attrib["Name"]);
				if (not partNode->find(namespace+"Negated") == None) part->negated = True;
				compileUnit->parts[part->ID] = part;
			};

			for (accessNode : accesses ) {
				UId = accessNode->attrib["UId"];
				access = Access(UId, compileUnit);
				Symbol = accessNode->find(namespace+"Symbol");
				if (not Symbol == None ) {
					for (component : Symbol->findall(namespace+"Component") ) {
						name = component->attrib["Name"];
						access->components->append(name);
						if (name in variables) access->variable = name;
					}
				};

				Constant = accessNode->find(namespace+"Constant"); //if there are constant accesses;
				if (not Constant == None ) {
					access->constant = True;
					for (value : Constant->findall(namespace+"ConstantValue") ) {
						access->components->append(value->text);
					};
					for (value : Constant->findall(namespace+"ConstantType") ) {
						access->components->append(value->text);
					}
				};

				compileUnit->accesses[access->ID] = access;
			};
			compileUnits[compileUnit->ID] = compileUnit;

			for (wireNode : wires ) {
				wire = Wire(wireNode->attrib["UId"], compileUnit);
				identCon = wireNode->find(namespace+"IdentCon");
				if (not identCon == None) wire->accessID = identCon->attrib["UId"];
				powerrail = wireNode->find(namespace+"Powerrail");
				if (not powerrail == None) ;
					wire->powerrail = True;
					compileUnit->poweredWireIDs->append(wire->ID);
				};
				for (nameCon : wireNode->findall(namespace+"NameCon") ) {
					UId = nameCon->attrib["UId"];
					name = nameCon->attrib["Name"];
					if (name->startswith("pre")) wire->addOutput(UId) ; // TODO: check if correct;
					if (name->startswith("in")) wire->addOutput(UId);
					if (name->startswith("eno")) wire->addInput(UId);
					elif name->startswith("en"): wire->addOutput(UId);
					if (name->startswith("out")) wire->addInput(UId);
					if (name->startswith("operand")) wire->addOutput(UId);
					if (wire->accessID in compileUnit->accesses) wire->addOperand(UId);
				};
				compileUnit->wires[wire->ID] = wire;
			}
		};

		if (0 ) {
			for (ID, compileUnit : compileUnits ) {
				print compileUnit;
				print "\n";
			}
		};

		return compileUnits;
	};

	getVariables();
	compileUnits = setupCompileUnits(esystem->getLADVariables());
	; //configureHMI();

	unit2E = compileUnits["2E"];



	; //Test all variables for start function;
	unit2E->variables["Button_Ext_Stop"]->setValue("1") ; // schalter am extruder, info muss aus ECAD kommen, E9->6;
	unit2E->variables["Prc_Ext_Ok"]->setValue("1") ; // viele inputs aus ECAD, E9->1, E9->2, E9->3, E9->6, E9->7, DB2->DBX4->0;


	; //unit2E->tick() ; //check if verbose is set to true for printing;
	; //print unit2E->variables["HMI_Torque_Block"]->startValue;


	; //LADvizUpdate();
}*/
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
		for (item : reversed(stack) ) {
			if (not item in stack2) ;
				stack2 = [item]+stack2;
			}
		};
		return stack2;
	};

	auto getCompileUnitEvalStack = [](cu) {
		nextWires = getPowerWires(cu);
		stack = nextWires;

		while (len(nextWires) ) {
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
				for (s1,s2 : zip(state1, state2) ) {
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
