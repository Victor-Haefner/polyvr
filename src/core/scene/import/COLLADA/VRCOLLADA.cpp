#include "VRCOLLADA.h"
#include "VRCOLLADA_Material.h"
#include "VRCOLLADA_Geometry.h"
#include "VRCOLLADA_Kinematics.h"

#include "core/scene/import/VRImportFwd.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"

#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

#include <OpenSG/OSGColor.h>

#include <iostream>
#include <stack>

using namespace OSG;

namespace OSG {

class VRCOLLADA_Stream : public XMLStreamHandler {
    private:
        struct Node {
            string name;
            string data;
            map<string, XMLAttribute> attributes;
        };

        stack<Node> nodeStack;

        VRObjectPtr root;
        stack<VRObjectPtr> objStack;

        VRCOLLADA_Material materials;
        VRCOLLADA_Geometry geometries;
        //VRCOLLADA_Kinematics kinematics;

        string skipHash(const string& s) { return subString(s, 1, s.size()-1); }

    public:
        VRCOLLADA_Stream(VRObjectPtr root) : root(root) {}
        ~VRCOLLADA_Stream() {}

        static VRCOLLADA_StreamPtr create(VRObjectPtr root) { return VRCOLLADA_StreamPtr( new VRCOLLADA_Stream(root) ); }

        void startDocument() {}
        void endDocument() {}
        void startElement(const string& uri, const string& name, const string& qname, const map<string, XMLAttribute>& attributes);
        void endElement(const string& uri, const string& name, const string& qname);
        void characters(const string& chars);
        void processingInstruction(const string& target, const string& data) {}

        void warning(const string& chars) { cout << "VRCOLLADA_Stream Warning" << endl; }
        void error(const string& chars) { cout << "VRCOLLADA_Stream Error" << endl; }
        void fatalError(const string& chars) { cout << "VRCOLLADA_Stream Fatal Error" << endl; }
        void onException(exception& e) { cout << "VRCOLLADA_Stream Exception" << endl; }
};

}

void VRCOLLADA_Stream::startElement(const string& uri, const string& name, const string& qname, const map<string, XMLAttribute>& attributes) {
    //cout << "startElement " << name << " " << nodeStack.size() << endl;
    Node n;
    n.name = name;
    n.attributes = attributes;
    nodeStack.push(n);

    if (name == "effect") materials.newEffect( n.attributes["id"].val );
    if (name == "geometry") geometries.newGeometry(n.attributes["name"].val, n.attributes["id"].val);
    if (name == "source") geometries.newSource(n.attributes["id"].val);
    if (name == "accessor") geometries.handleAccessor(n.attributes["count"].val, n.attributes["stride"].val);
    if (name == "input") geometries.handleInput(n.attributes["semantic"].val, skipHash(n.attributes["source"].val), n.attributes["offset"].val, n.attributes["set"].val);
    if (name == "triangles") geometries.newPrimitive(name, n.attributes["count"].val, 3);
    if (name == "instance_geometry") geometries.instantiateGeometry(skipHash(n.attributes["url"].val), objStack.top());

    if (name == "node") {
        string Name = n.attributes["name"].val;
        auto parent = objStack.size() > 0 ? objStack.top() : root;
        auto obj = VRTransform::create(Name);
        parent->addChild(obj);
        objStack.push(obj);
    }
}

void VRCOLLADA_Stream::characters(const string& chars) {
    nodeStack.top().data = chars;
}

void VRCOLLADA_Stream::endElement(const string& uri, const string& name, const string& qname) {
    auto node = nodeStack.top();
    nodeStack.pop();

    if (name == "node") objStack.pop();
    if (node.name == "effect") materials.closeEffect();
    if (node.name == "geometry") geometries.closeGeometry();
    if (node.name == "color") materials.setColor(node.attributes["sid"].val, toValue<Color4f>(node.data));
    if (node.name == "float_array") geometries.setSourceData(node.data);
    if (name == "triangle") geometries.closePrimitive();
    if (node.name == "p") geometries.handleIndices(node.data);

    if (node.name == "float") {
        //float f = toValue<float>(node.data);
        //string sid = node.attributes["sid"].val;
        //if (sid == "ior" && currentMaterial) currentMaterial->setRefraction(f);
    }


    //cout << "endElement " << name << " " << nodeStack.size() << endl;
}

void OSG::loadCollada(string path, VRObjectPtr root) {
    auto xml = XML::create();

    auto handler = VRCOLLADA_Stream::create(root);
    xml->stream(path, handler.get());
}
