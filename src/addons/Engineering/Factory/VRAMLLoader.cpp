#include "VRAMLLoader.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/import/VRImport.h"
#include "core/utils/system/VRSystem.h"

#include <iostream>
#include <OpenSG/OSGVector.h>

using namespace OSG;

VRAMLLoader::VRAMLLoader() {}
VRAMLLoader::~VRAMLLoader() {}

VRAMLLoaderPtr VRAMLLoader::create()  { return VRAMLLoaderPtr(new VRAMLLoader()); }

VROntologyPtr VRAMLLoader::getOntology() { return ontology; }
VRTransformPtr VRAMLLoader::getScene() { return scene; }

void VRAMLLoader::read(string path) {
    assets.clear();
    ontology = VROntology::create("global");
    AMLDir = getFolderName(path)+"/";

    XML xml;
    xml.read(path);

    scene = VRTransform::create("scene");
    //scene->setOrientation(Vec3d(0,-1,0), Vec3d(0,0,1)); // TODO
    auto hierarchy = xml.getRoot()->getChild("InstanceHierarchy"); // scene graph
	readNode(hierarchy, scene);
}

void VRAMLLoader::readNode(XMLElementPtr node, VRTransformPtr parent) {
    if (!node) return;

    processElement(node, parent);

    string name = node->getName();
    if (name == "InstanceHierarchy" || name == "InternalElement") {
        auto child = VRTransform::create( node->getAttribute("Name") );
        parent->addChild(child);
        for (auto childNode : node->getChildren()) readNode(childNode, child);
    }
}

void VRAMLLoader::processElement(XMLElementPtr node, VRTransformPtr parent) {
    string name = node->getName();
    // geometries;
    if (name == "ExternalInterface" && node->getAttribute("Name") == "Representation" ) {
        for (auto a : node->getChildren("Attribute") ) {
            if (a->getAttribute("Name") == "refType" ) {
                string ref = a->getChild("Value")->getText();
                if (ref == "implicit") parent->hide();
            }

            if (a->getAttribute("Name") == "refURI" ) {
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
                        assets[uri] = VRImport::get()->load( AMLDir+uri, 0, 0, "COLLADA", 0, options, 0);
                        if (assets[uri]) assets[uri]->setPersistency(0);
                    }
                    //if len(assets) == 56 ) {
                    if (assets[uri]) parent->addChild(assets[uri]->duplicate());
                }
            }
        }
    }

    // transformation;
    if (name == "Attribute" && node->getAttribute("Name") == "Frame" ) {
        Vec3d pos;
        Vec3d eul;
        for (auto a : node->getChildren("Attribute") ) {
            if (a->getAttribute("Name") == "x") pos[0] = toFloat(a->getChild("Value")->getText())*1000;
            if (a->getAttribute("Name") == "y") pos[1] = toFloat(a->getChild("Value")->getText())*1000;
            if (a->getAttribute("Name") == "z") pos[2] = toFloat(a->getChild("Value")->getText())*1000;
            if (a->getAttribute("Name") == "rx") eul[0] = toFloat(a->getChild("Value")->getText())*Pi/180.0;
            if (a->getAttribute("Name") == "ry") eul[1] = toFloat(a->getChild("Value")->getText())*Pi/180.0;
            if (a->getAttribute("Name") == "rz") eul[2] = toFloat(a->getChild("Value")->getText())*Pi/180.0;
        }
        parent->setFrom(pos);
        parent->setEuler(eul);
        //print pos, eul;
    }

    // role;
    if (name == "RoleRequirements" ) {
        string role = node->getAttribute("RefBaseRoleClassPath");
        auto rolePath = splitString(role, '/');
        role = rolePath[rolePath.size()-1];
        if (!ontology->getConcept(role) ) ontology->addConcept(role);
        auto e = ontology->addEntity("e", role);
        parent->setEntity(e);
    }

    // kinematics;
    if (name == "InternalLink" && node->getAttribute("Name") == "") { // name starts with "Attachment_";

        //RefPartnerSideA // "...= [id1]:[Attachment_...1]";
        //RefPartnerSideB // "...= [id2]:[Attachment_...2]";
        //print "kin";
    }
}


string VRAMLLoader::addAsset(VRObjectPtr obj) {
    string uuid = genUUID();
    assets[uuid] = obj;
    return uuid;
}

void VRAMLLoader::writeHeader(ofstream& stream, string fileName) {
    string head =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<CAEXFile FileName=\""+fileName+"\" SchemaVersion=\"2.15\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"CAEX_ClassModel_V2.15.xsd\">\n"
    "\t<Version>2.0</Version>\n"
    "\t<AdditionalInformation AutomationMLVersion=\"2.0\" />\n"
    "\t<AdditionalInformation>\n"
    "\t\t<WriterHeader>\n"
    "\t\t\t<WriterName>PolyVR</WriterName>\n"
    "\t\t\t<WriterID>PolyVR</WriterID>\n"
    "\t\t\t<WriterVendor>PolyVR</WriterVendor>\n"
    "\t\t\t<WriterVendorURL>https://github.com/Victor-Haefner/polyvr</WriterVendorURL>\n"
    "\t\t\t<WriterVersion>1.0</WriterVersion>\n"
    "\t\t\t<WriterRelease>1.0</WriterRelease>\n"
    "\t\t\t<LastWritingDateTime>2021-12-16T12:00:00.0000000+01:00</LastWritingDateTime>\n"
    "\t\t\t<WriterProjectTitle>PolyVR Data Export</WriterProjectTitle>\n"
    "\t\t\t<WriterProjectID>PolyVR Data Export</WriterProjectID>\n"
    "\t\t</WriterHeader>\n"
    "\t</AdditionalInformation>\n";

    stream << head;
}

void VRAMLLoader::writeFooter(ofstream& stream) {
    stream << "</CAEXFile>\n";
}

void VRAMLLoader::writeScene(ofstream& stream, string daeFolder) {
    makedir( daeFolder );
	stream << "\t<InstanceHierarchy ID=\"" << genUUID() << "\" Name=\"InstanceHierarchy\">\n";

	for (auto obj : assets) {
        string oName = obj.second->getName();
        std::replace( oName.begin(), oName.end(), '/', '_');
        string fPath = daeFolder+"/"+oName+".dae";
        string uuidEl = genUUID();
        string uuidIn = genUUID();
        obj.second->exportToFile(fPath, {});
        stream << "\t\t<InternalElement ID=\"" << uuidEl << "\" Name=\"" << oName << "\">\n";
        stream << "\t\t\t<ExternalInterface Name=\"Representation\" RefBaseClassPath=\"AutomationMLInterfaceClassLib/AutomationMLBaseInterface/ExternalDataConnector/COLLADAInterface\" ID=\"" << uuidIn << "\">\n";
        stream << "\t\t\t\t<Attribute Name=\"refURI\" AttributeDataType=\"xs:anyURI\">\n";
        stream << "\t\t\t\t\t<Value>./dae/" << oName << ".dae</Value>\n";
        stream << "\t\t\t\t</Attribute>\n";
        stream << "\t\t\t\t<Attribute Name=\"refType\">\n";
        stream << "\t\t\t\t\t<Value>explicit</Value>\n";
        stream << "\t\t\t\t</Attribute>\n";
        stream << "\t\t\t</ExternalInterface>\n";
        stream << "\t\t\t<RoleRequirements RefBaseRoleClassPath=\"AutomationMLBaseRoleClassLib/AutomationMLBaseRole/Resource\" />\n";
        stream << "\t\t</InternalElement>\n";
	}

	stream << "\t</InstanceHierarchy>\n";
}

void VRAMLLoader::writeOntology(ofstream& stream) {
    stream << "\t<InterfaceClassLib Name=\"AutomationMLInterfaceClassLib\">\n";
    stream << "\t\t<Description>Standard Automation Markup Language Interface Class Library</Description>\n";
    stream << "\t\t<Version>2.2.2</Version>\n";
	stream << "\t</InterfaceClassLib>\n";
}

void VRAMLLoader::write(string path) {
    string amlFolder = getFolderName(path);
    makedir( amlFolder );
    ofstream stream(path);
    writeHeader(stream, getFileName(path) );
    writeScene(stream, amlFolder+"/dae");
    writeOntology(stream);
    writeFooter(stream);
}


