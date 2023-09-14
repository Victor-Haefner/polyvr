VR.Ontology VR.AMLLoader.getOntology():
	 return ontology
	
VR.Transform VR.AMLLoader.getScene():
	 return scene
	
def VR.AMLLoader.read(string path):
	assets.clear()
	sm = VR.Scene.getCurrent().getSemanticManager()
	ontology = sm.addOntology("AutomationML")
	ontology.setFlag("internal")
	ontology.addConcept("Link", "",:
		:
			"refA", "string"
		,:
			"refB", "string"
		
	)
	AMLDir = getFolderName(path)+"/"
	XML xml
	xml.read(path)
	interfaceLibs = xml.getRoot().getChildren("InterfaceClassLib")
	 // knowledge basesroleLibs = xml.getRoot().getChildren("RoleClassLib")
	 // knowledge basesinterfaceLibs.insert(interfaceLibs.end(), roleLibs.begin(), roleLibs.end())
	for (lib : interfaceLibs):
		for (cl : lib.getChildren()):
			readLibNode(cl)
			
		
	scene = VR.Transform("scene")
	scene.setOrientation([0,-1,0), [0,0,1))
	 // AML has z uphierarchy = xml.getRoot().getChild("InstanceHierarchy")
	 // scene graphreadNode(hierarchy, scene)
	
def VR.AMLLoader.readLibNode(XMLElement node, VR.Concept parent):
	if (!node) return
	string tag = node.getName()
	if (tag == "RoleClass" or tag == "InterfaceClass"):
		string name = node.getAttribute("Name")
		cl = ontology.addConcept(name, parent?parent.getName():"")
		for (c : node.getChildren()) readLibNode(c, cl)
		
	if (tag == "Attribute"):
		string name = node.getAttribute("Name")
		string type = node.getAttribute("AttributeDataType")
		if (contains(type, ":")) type = splitString(type, ':')[1]
		parent.addProperty(name, type)
		
	
string VR.AMLLoader.parseRole(const string& role):
	if (role == "") return ""
	rolePath = splitString(role, '/')
	return rolePath[rolePath.size()-1]
	
VR.Entity VR.AMLLoader.addEntity(const string& role, VR.Object parent, map<string, string> props):
	if (role != ""):
		if (!ontology.getConcept(role) ):
			ontology.addConcept(role, "", props)
			
		e = parent.getEntity()
		if (!e):
			//e = ontology.addEntity("e", role)
			 // TODO: proper name?//parent.setEntity(e)
			
		 else:
			if (!e.is_a(role)) e.addConcept( ontology.getConcept(role) )
			
		return e
		
	return 0
	
def VR.AMLLoader.readNode(XMLElement node, VR.Transform parent):
	if (!node) return
	processElement(node, parent)
	string name = node.getName()
	if (name == "InstanceHierarchy" or name == "InternalElement"):
		child = VR.Transform( node.getAttribute("Name") )
		e = ontology.addEntity( node.getAttribute("Name") )
		child.setEntity(e)
		roles = node.getChildren("RoleRequirements")
		roles2 = node.getChildren("SupportedRoleClass")
		roles.insert(roles.end(), roles2.begin(), roles2.end())
		for (c : roles):
			string role
			if (c.hasAttribute("RefBaseRoleClassPath")) role = c.getAttribute("RefBaseRoleClassPath")
			if (c.hasAttribute("RefRoleClassPath")) role = c.getAttribute("RefRoleClassPath")
			role = parseRole(role)
			addEntity(role, child)
			
		parent.addChild(child)
		for (childNode : node.getChildren()) readNode(childNode, child)
		
	
def VR.AMLLoader.processElement(XMLElement node, VR.Transform parent):
	string name = node.getName()
	// structureif (name == "InstanceHierarchy" or name == "InternalElement"):
		return
		
	// geometries
	if (name == "ExternalInterface" and node.getAttribute("Name") == "Representation" ):
		for (a : node.getChildren("Attribute") ):
			if (a.getAttribute("Name") == "refType" ):
				string ref = a.getChild("Value").getText()
				if (ref == "implicit") parent.hide()
				
			if (a.getAttribute("Name") == "refURI"):
				string uri = a.getChild("Value").getText()
				if ( contains(uri, ".dae") ):
					string scene = ""
					if ( contains(uri, "#") ):
						scene = splitString(uri, '#')[1]
						uri = splitString(uri, '#')[0]
						
					if (!assets.count(uri)):
						map<string, string> options
						options["scene"] = scene
						cout << "AMLDir \"" << AMLDir << "\", uri \"" << uri << "\"" << endl
						obj = VR.Import.get().load( AMLDir+uri, 0, 0, "COLLADA", 0, options, 0)
						if (obj):
							obj.setPersistency(0)
							obj.setIdentity()
							 // rotation is in aml node!assets[uri] = obj
							
						
					if (assets.count(uri) and assets[uri]):
						obj = dynamic_pointer_cast<VR.Transform>(assets[uri].duplicate())
						//parent.setPose( obj.getPose() )
						//obj.setIdentity()
						parent.addChild(obj)
						
					
				
			
		return
		
	// transformation
	if (name == "Attribute" and node.getAttribute("Name") == "Frame" ):
		Vec3d pos
		Vec3d eul
		for (a : node.getChildren("Attribute") ):
			if (a.getAttribute("Name") == "x") pos[0] = toFloat(a.getChild("Value").getText())*1000.0
			if (a.getAttribute("Name") == "y") pos[1] = toFloat(a.getChild("Value").getText())*1000.0
			if (a.getAttribute("Name") == "z") pos[2] = toFloat(a.getChild("Value").getText())*1000.0
			if (a.getAttribute("Name") == "rx") eul[0] = toFloat(a.getChild("Value").getText())*Pi/180.0
			if (a.getAttribute("Name") == "ry") eul[1] = toFloat(a.getChild("Value").getText())*Pi/180.0
			if (a.getAttribute("Name") == "rz") eul[2] = toFloat(a.getChild("Value").getText())*Pi/180.0
			
		parent.setFrom(pos)
		parent.setEuler(eul)
		//print pos, eul
		return
		
	// skip roleif (name == "RoleRequirements" or name == "SupportedRoleClass"):
		return
		
	if (name == "ExternalInterface"):
		string role
		if (node.hasAttribute("RefBaseClassPath")):
			role = node.getAttribute("RefBaseClassPath")
			role = parseRole(role)
			e = addEntity(role, parent,:
				:
					"ID",""
				
			)
			if (node.hasAttribute("ID")) e.set("ID", node.getAttribute("ID"))
			
		return
		
	if (name == "Attribute"):
		string name = node.getAttribute("Name")
		string type = node.getAttribute("AttributeDataType")
		valNode = node.getChild("Value")
		string val = valNode ? valNode.getText() : ""
		e = parent.getEntity()
		if (e) e.add(name, val)
		return
		
	// kinematics and electricsif (name == "InternalLink"):
		string name = node.getAttribute("Name")
		string refA = node.getAttribute("RefPartnerSideA")
		string refB = node.getAttribute("RefPartnerSideB")
		e = ontology.addEntity(name, "Link")
		e.set("refA", refA)
		e.set("refB", refB)
		return
		
	cout << "VR.AMLLoader.processElement, unhandled element: " << name << endl
	
