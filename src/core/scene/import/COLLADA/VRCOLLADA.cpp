#include "VRCOLLADA.h"
#include "VRCOLLADA_Material.h"
#include "VRCOLLADA_Geometry.h"
#include "VRCOLLADA_Kinematics.h"

#include "core/scene/import/VRImportFwd.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/utils/VRScheduler.h"

#include "core/objects/object/VRObject.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

#include <OpenSG/OSGColor.h>

#include <iostream>
#include <stack>

using namespace OSG;

namespace OSG {

class VRCOLLADA_Scene {
    private:
        VRSchedulerPtr scheduler;
        VRObjectPtr root;
        map<string, VRObjectPtr> objects;
        map<string, VRObjectPtr> subscenes;
        stack<VRObjectPtr> objStack;
        VRGeometryPtr lastInstantiatedGeo;

    public:
        VRCOLLADA_Scene() { scheduler = VRScheduler::create(); }

        void setRoot(VRObjectPtr r) { root = r; }
        void closeNode() { objStack.pop(); }
        VRObjectPtr top() { return objStack.size() > 0 ? objStack.top() : 0; }

		void finalize() {
            scheduler->callPostponed(true);
		}

        void setMatrix(string data) {
            Matrix4d m;
            toValue(data, m);
            m.transpose();
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) t->setMatrix(m);
        }

        void translate(string data) {
            Vec3d v;
            toValue(data, v);
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) t->translate(v);
        }

        void rotate(string data) {
            Vec4d v;
            toValue(data, v);
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) t->rotate(v[3]/180.0*Pi, Vec3d(v[0], v[1], v[2]));
        }

        void setMaterial(string mid, VRCOLLADA_Material* materials, VRGeometryPtr geo = 0) {
            if (!geo) geo = lastInstantiatedGeo;
            auto mat = materials->getMaterial(mid);
            if (!mat) {
                scheduler->postpone( bind(&VRCOLLADA_Scene::setMaterial, this, mid, materials, geo) );
                return;
            }
            if (geo) geo->setMaterial(mat);
        }

        void newNode(string name) {
            auto parent = objStack.size() > 0 ? objStack.top() : root;
            auto obj = VRTransform::create(name);
            if (parent) parent->addChild(obj);
            objStack.push(obj);
        }

        void instantiateGeometry(string gid, VRCOLLADA_Geometry* geometries, VRObjectPtr parent = 0) {
            if (!parent) parent = top();
            auto geo = geometries->getGeometry(gid);
            if (!geo) {
                scheduler->postpone( bind(&VRCOLLADA_Scene::instantiateGeometry, this, gid, geometries, parent) );
                return;
            }
            lastInstantiatedGeo = dynamic_pointer_cast<VRGeometry>( geo->duplicate() );
            if (parent) parent->addChild( lastInstantiatedGeo );
        }

        void loadSubscene(string url, string fPath, VRObjectPtr parent = 0) {
            if (!parent) parent = top();

            if (!subscenes.count(url)) {
                string file = splitString(url, '#')[0];
                if (file == "") return; // TODO: implement nodes library!
                //string scene = splitString(url, '#')[1]; // TODO
                VRTransformPtr obj = VRTransform::create(file);
                loadCollada( fPath + "/" + file, obj );
                subscenes[url] = obj;
            }

            if (parent) parent->addChild( subscenes[url]->duplicate() );
        }
};

class VRCOLLADA_Stream : public XMLStreamHandler {
    private:
        struct Node {
            string name;
            string data;
            map<string, XMLAttribute> attributes;
        };

        string fPath;
        stack<Node> nodeStack;

        VRCOLLADA_Material materials;
        VRCOLLADA_Geometry geometries;
        VRCOLLADA_Scene scene;
        //VRCOLLADA_Kinematics kinematics;

        string skipHash(const string& s) { return subString(s, 1, s.size()-1); }

    public:
        VRCOLLADA_Stream(VRObjectPtr root, string fPath) : fPath(fPath) {
            scene.setRoot(root);
            materials.setFilePath(fPath);
        }

        ~VRCOLLADA_Stream() {}

        static VRCOLLADA_StreamPtr create(VRObjectPtr root, string fPath) { return VRCOLLADA_StreamPtr( new VRCOLLADA_Stream(root, fPath) ); }

        void startDocument() {}

        void endDocument() {
            materials.finalize();
            geometries.finalize();
            scene.finalize();
        }

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
    Node parent;
    if (nodeStack.size() > 0) parent = nodeStack.top();

    Node n;
    n.name = name;
    n.attributes = attributes;
    nodeStack.push(n);

    if (name == "surface") materials.addSurface(parent.attributes["sid"].val);
    if (name == "sampler2D") materials.addSampler(parent.attributes["sid"].val);
    if (name == "effect") materials.newEffect( n.attributes["id"].val );
    if (name == "material") materials.newMaterial( n.attributes["id"].val, n.attributes["name"].val );
    if (name == "instance_effect") materials.setMaterialEffect( skipHash(n.attributes["url"].val) );

    if (name == "geometry") geometries.newGeometry(n.attributes["name"].val, n.attributes["id"].val);
    if (name == "source") geometries.newSource(n.attributes["id"].val);
    if (name == "accessor") geometries.handleAccessor(n.attributes["count"].val, n.attributes["stride"].val);
    if (name == "input") geometries.handleInput(n.attributes["semantic"].val, skipHash(n.attributes["source"].val), n.attributes["offset"].val, n.attributes["set"].val);
    if (name == "triangles") geometries.newPrimitive(name, n.attributes["count"].val);
    if (name == "trifans") geometries.newPrimitive(name, n.attributes["count"].val);
    if (name == "tristrips") geometries.newPrimitive(name, n.attributes["count"].val);
    if (name == "polylist") geometries.newPrimitive(name, n.attributes["count"].val);

    if (name == "instance_geometry") scene.instantiateGeometry(skipHash(n.attributes["url"].val), &geometries);
    if (name == "instance_material") scene.setMaterial(skipHash(n.attributes["target"].val), &materials);
    if (name == "instance_node") scene.loadSubscene(n.attributes["url"].val, fPath);
    if (name == "node") scene.newNode(n.attributes["name"].val);
}

void VRCOLLADA_Stream::characters(const string& chars) {
    nodeStack.top().data += chars;
}

void VRCOLLADA_Stream::endElement(const string& uri, const string& name, const string& qname) {
    auto node = nodeStack.top();
    nodeStack.pop();
    Node parent;
    if (nodeStack.size() > 0) parent = nodeStack.top();

    if (node.name == "init_from" && parent.name == "surface") materials.setSurfaceSource(node.data);
    if (node.name == "source" && parent.name == "sampler2D") materials.setSamplerSource(node.data);

    if (node.name == "effect") materials.closeEffect();
    if (node.name == "geometry") geometries.closeGeometry();
    if (node.name == "init_from" && parent.name == "image") materials.loadImage(parent.attributes["id"].val, node.data);
    if (node.name == "color") {
        string type;
        if (node.attributes.count("sid")) type = node.attributes["sid"].val;
        else type = parent.name;
        materials.setColor(type, toValue<Color4f>(node.data));
    }
    if (node.name == "texture") materials.setTexture(node.attributes["texture"].val);
    if (node.name == "float_array") geometries.setSourceData(node.data);
    if (node.name == "p") geometries.handleIndices(node.data);
    if (node.name == "triangles") geometries.closePrimitive();
    if (node.name == "trifans") geometries.closePrimitive();
    if (node.name == "tristrips") geometries.closePrimitive();
    if (node.name == "polylist") geometries.closePrimitive();
    if (node.name == "vcount") geometries.handleVCount(node.data);

    if (node.name == "float") {
        float f = toValue<float>(node.data);
        string sid = node.attributes["sid"].val;
        if (sid == "shininess") materials.setShininess(f);
    }

    if (node.name == "node") scene.closeNode();
    if (node.name == "matrix") scene.setMatrix(node.data);
    if (node.name == "translate") scene.translate(node.data);
    if (node.name == "rotate") scene.rotate(node.data);
}

void OSG::loadCollada(string path, VRObjectPtr root) {
    auto xml = XML::create();
    string fPath = getFolderName(path);
    auto handler = VRCOLLADA_Stream::create(root, fPath);
    xml->stream(path, handler.get());
}
