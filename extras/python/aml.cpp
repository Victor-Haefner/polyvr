VROntologyPtr VRAMLLoader::getOntology() { return ontology; }
VRTransformPtr VRAMLLoader::getScene() { return scene; }

void VRAMLLoader::read(string path) {
    assets.clear();
    auto sm = VRScene::getCurrent()->getSemanticManager();
    ontology = sm->addOntology("AutomationML");
    ontology->setFlag("internal");

    ontology->addConcept("Link", "", {{"refA", "string"},{"refB", "string"}});

    AMLDir = getFolderName(path)+"/";
    XML xml;
    xml.read(path);

    auto interfaceLibs = xml.getRoot()->getChildren("InterfaceClassLib"); // knowledge bases
    auto roleLibs = xml.getRoot()->getChildren("RoleClassLib"); // knowledge bases
    interfaceLibs.insert(interfaceLibs.end(), roleLibs.begin(), roleLibs.end());

    for (auto lib : interfaceLibs) {
        for (auto cl : lib->getChildren()) {
            readLibNode(cl);
        }
    }


    scene = VRTransform::create("scene");
    scene->setOrientation(Vec3d(0,-1,0), Vec3d(0,0,1)); // AML has z up
    auto hierarchy = xml.getRoot()->getChild("InstanceHierarchy"); // scene graph
	readNode(hierarchy, scene);
}

void VRAMLLoader::readLibNode(XMLElementPtr node, VRConceptPtr parent) {
    if (!node) return;
    string tag = node->getName();

    if (tag == "RoleClass" || tag == "InterfaceClass") {
        string name = node->getAttribute("Name");
        auto cl = ontology->addConcept(name, parent?parent->getName():"");
        for (auto c : node->getChildren()) readLibNode(c, cl);
    }

    if (tag == "Attribute") {
        string name = node->getAttribute("Name");
        string type = node->getAttribute("AttributeDataType");
        if (contains(type, ":")) type = splitString(type, ':')[1];
        parent->addProperty(name, type);
    }
}

string VRAMLLoader::parseRole(const string& role) {
    if (role == "") return "";
    auto rolePath = splitString(role, '/');
    return rolePath[rolePath.size()-1];
}

VREntityPtr VRAMLLoader::addEntity(const string& role, VRObjectPtr parent, map<string, string> props) {
    if (role != "") {
        if (!ontology->getConcept(role) ) {
            ontology->addConcept(role, "", props);
        }

        auto e = parent->getEntity();
        if (!e) {
            //e = ontology->addEntity("e", role); // TODO: proper name?
            //parent->setEntity(e);
        } else {
            if (!e->is_a(role)) e->addConcept( ontology->getConcept(role) );
        }
        return e;
    }
    return 0;
}

void VRAMLLoader::readNode(XMLElementPtr node, VRTransformPtr parent) {
    if (!node) return;

    processElement(node, parent);

    string name = node->getName();
    if (name == "InstanceHierarchy" || name == "InternalElement") {
        auto child = VRTransform::create( node->getAttribute("Name") );
        auto e = ontology->addEntity( node->getAttribute("Name") );
        child->setEntity(e);

        auto roles = node->getChildren("RoleRequirements");
        auto roles2 = node->getChildren("SupportedRoleClass");
        roles.insert(roles.end(), roles2.begin(), roles2.end());
        for (auto c : roles) {
            string role;
            if (c->hasAttribute("RefBaseRoleClassPath")) role = c->getAttribute("RefBaseRoleClassPath");
            if (c->hasAttribute("RefRoleClassPath")) role = c->getAttribute("RefRoleClassPath");
            role = parseRole(role);
            addEntity(role, child);
        }

        parent->addChild(child);
        for (auto childNode : node->getChildren()) readNode(childNode, child);
    }
}

void VRAMLLoader::processElement(XMLElementPtr node, VRTransformPtr parent) {
    string name = node->getName();

    // structure
    if (name == "InstanceHierarchy" || name == "InternalElement") {
        return;
    }

    // geometries;
    if (name == "ExternalInterface" && node->getAttribute("Name") == "Representation" ) {
        for (auto a : node->getChildren("Attribute") ) {
            if (a->getAttribute("Name") == "refType" ) {
                string ref = a->getChild("Value")->getText();
                if (ref == "implicit") parent->hide();
            }

            if (a->getAttribute("Name") == "refURI") {
                string uri = a->getChild("Value")->getText();
                if ( contains(uri, ".dae") ) {
                    string scene = "";
                    if ( contains(uri, "#") ) {
                        scene = splitString(uri, '#')[1];
                        uri = splitString(uri, '#')[0];
                    }

                    if (!assets.count(uri)) {
                        map<string, string> options;
                        options["scene"] = scene;
                        cout << "AMLDir \"" << AMLDir << "\", uri \"" << uri << "\"" << endl;
                        auto obj = VRImport::get()->load( AMLDir+uri, 0, 0, "COLLADA", 0, options, 0);
                        if (obj) {
                            obj->setPersistency(0);
                            obj->setIdentity(); // rotation is in aml node!
                            assets[uri] = obj;
                        }
                    }

                    if (assets.count(uri) && assets[uri]) {
                        auto obj = dynamic_pointer_cast<VRTransform>(assets[uri]->duplicate());
                        //parent->setPose( obj->getPose() );
                        //obj->setIdentity();
                        parent->addChild(obj);
                    }
                }
            }
        }
        return;
    }

    // transformation;
    if (name == "Attribute" && node->getAttribute("Name") == "Frame" ) {
        Vec3d pos;
        Vec3d eul;
        for (auto a : node->getChildren("Attribute") ) {
            if (a->getAttribute("Name") == "x") pos[0] = toFloat(a->getChild("Value")->getText())*1000.0;
            if (a->getAttribute("Name") == "y") pos[1] = toFloat(a->getChild("Value")->getText())*1000.0;
            if (a->getAttribute("Name") == "z") pos[2] = toFloat(a->getChild("Value")->getText())*1000.0;
            if (a->getAttribute("Name") == "rx") eul[0] = toFloat(a->getChild("Value")->getText())*Pi/180.0;
            if (a->getAttribute("Name") == "ry") eul[1] = toFloat(a->getChild("Value")->getText())*Pi/180.0;
            if (a->getAttribute("Name") == "rz") eul[2] = toFloat(a->getChild("Value")->getText())*Pi/180.0;
        }
        parent->setFrom(pos);
        parent->setEuler(eul);
        //print pos, eul;
        return;
    }

    // skip role
    if (name == "RoleRequirements" || name == "SupportedRoleClass") {
        return;
    }

    if (name == "ExternalInterface") {
        string role;
        if (node->hasAttribute("RefBaseClassPath")) {
            auto role = node->getAttribute("RefBaseClassPath");
            role = parseRole(role);
            auto e = addEntity(role, parent, {{"ID",""}});
            if (node->hasAttribute("ID")) e->set("ID", node->getAttribute("ID"));
        }
        return;
    }

    if (name == "Attribute") {
        string name = node->getAttribute("Name");
        string type = node->getAttribute("AttributeDataType");
        auto valNode = node->getChild("Value");
        string val = valNode ? valNode->getText() : "";
        auto e = parent->getEntity();
        if (e) e->add(name, val);
        return;
    }

    // kinematics and electrics
    if (name == "InternalLink") {
        string name = node->getAttribute("Name");
        string refA = node->getAttribute("RefPartnerSideA");
        string refB = node->getAttribute("RefPartnerSideB");

        auto e = ontology->addEntity(name, "Link");
        e->set("refA", refA);
        e->set("refB", refB);
        return;
    }

    cout << "VRAMLLoader::processElement, unhandled element: " << name << endl;
}

