#include "VRCOLLADA.h"
#include "VRCOLLADA_Material.h"
#include "VRCOLLADA_Geometry.h"
#include "VRCOLLADA_Kinematics.h"

#include "core/scene/import/VRImportFwd.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/utils/VRScheduler.h"
#include "core/math/pose.h"

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
        VRObjectPtr root;
        map<string, VRObjectPtr> objects;
        map<string, VRObjectPtr> library_nodes;
        map<string, VRObjectPtr> library_scenes;
        string currentSection;

        VRSchedulerPtr scheduler;
        stack<VRObjectPtr> objStack;
        VRGeometryPtr lastInstantiatedGeo;

    public:
        VRCOLLADA_Scene() { scheduler = VRScheduler::create(); }

        void setRoot(VRObjectPtr r) { root = r; }
        void closeNode() { objStack.pop(); }
        VRObjectPtr top() { return objStack.size() > 0 ? objStack.top() : 0; }

        void setCurrentSection(string s) {
            currentSection = s;
            objStack = stack<VRObjectPtr>();
            lastInstantiatedGeo = 0;
        }

		void finalize() { scheduler->callPostponed(true); }

        void setMatrix(string data) {
            Matrix4d m = toValue<Matrix4d>(data);
            m.transpose();
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) {
                auto M = t->getMatrix();
                M.mult(m);
                t->setMatrix(M);
            }
        }

        void translate(string data) { // TODO: read in specs what these are all about, like this it messes up the AML test scene
            /*Vec3d v = toValue<Vec3d>(data);
            Matrix4d m;
            m.setTranslate(v);
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) {
                auto M = t->getMatrix();
                M.mult(m);
                t->setMatrix(M);
            }*/
        }

        void rotate(string data) { // TODO: read in specs what these are all about, like this it messes up the AML test scene
            /*Vec4d v = toValue<Vec4d>(data);
            Matrix4d m;
            m.setRotate(Quaterniond(Vec3d(v[0], v[1], v[2]), v[3]/180.0*Pi));
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) {
                auto M = t->getMatrix();
                M.mult(m);
                t->setMatrix(M);
            }*/
        }

        void setMaterial(string mid, VRCOLLADA_Material* materials, VRGeometryPtr geo = 0) {
            if (!geo) geo = lastInstantiatedGeo;
            auto mat = materials->getMaterial(mid);
            if (!mat) {
                if (geo) geo->addTag("COLLADA-postponed");
                scheduler->postpone( bind(&VRCOLLADA_Scene::setMaterial, this, mid, materials, geo) );
                return;
            }
            if (geo) geo->remTag("COLLADA-postponed");
            if (geo) geo->setMaterial(mat);
        }

        void newNode(string id, string name) {
            auto obj = VRTransform::create(name);

            auto parent = top();
            if (parent) parent->addChild(obj);
            else {
                if (currentSection == "library_nodes") library_nodes[id] = obj;
                if (currentSection == "library_visual_scenes") library_scenes[id] = obj;
            }

            objStack.push(obj);
        }

        void instantiateGeometry(string gid, VRCOLLADA_Geometry* geometries, VRObjectPtr parent = 0) {
            if (!parent) parent = top();
            auto geo = geometries->getGeometry(gid);
            if (!geo) {
                if (parent) parent->addTag("COLLADA-postponed");
                scheduler->postpone( bind(&VRCOLLADA_Scene::instantiateGeometry, this, gid, geometries, parent) );
                return;
            }
            lastInstantiatedGeo = dynamic_pointer_cast<VRGeometry>( geo->duplicate() );
            if (parent) parent->remTag("COLLADA-postponed");
            if (parent) parent->addChild( lastInstantiatedGeo );
        }

        void instantiateNode(string url, string fPath, VRObjectPtr parent = 0) {
            if (!parent) parent = top();

            if (!library_nodes.count(url)) {
                string file = splitString(url, '#')[0];
                string node = splitString(url, '#')[1];

                if (file != "") { // reference another file
                    // TODO, use node to get correct object
                    VRTransformPtr obj = VRTransform::create(file);
                    loadCollada( fPath + "/" + file, obj );
                    library_nodes[url] = obj;
                } else {} // should already be in library_nodes
            }

            if (parent && library_nodes.count(url)) {
                auto prototype = library_nodes[url];
                if (prototype->hasTag("COLLADA-postponed")) {
                    scheduler->postpone( bind(&VRCOLLADA_Scene::instantiateNode, this, url, fPath, parent) );
                } else parent->addChild( prototype->duplicate() );
            }
        }

        void instantiateScene(string url) {
            if (library_scenes.count(url)) root->addChild(library_scenes[url]);
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

        string currentSection;
        VRCOLLADA_Material materials;
        VRCOLLADA_Geometry geometries;
        VRCOLLADA_Scene scene;
        //VRCOLLADA_Kinematics kinematics;

        string skipHash(const string& s) { return (s[0] == '#') ? subString(s, 1, s.size()-1) : s; }

    public:
        VRCOLLADA_Stream(VRObjectPtr root, string fPath) : fPath(fPath) {
            scene.setRoot(root);
            materials.setFilePath(fPath);
        }

        ~VRCOLLADA_Stream() {}

        static VRCOLLADA_StreamPtr create(VRObjectPtr root, string fPath) { return VRCOLLADA_StreamPtr( new VRCOLLADA_Stream(root, fPath) ); }

        void startDocument() override {}

        void endDocument() override {
            materials.finalize();
            geometries.finalize();
            scene.finalize();
        }

        void startElement(const string& uri, const string& name, const string& qname, const map<string, XMLAttribute>& attributes) override;
        void endElement(const string& uri, const string& name, const string& qname) override;
        void characters(const string& chars) override;
        void processingInstruction(const string& target, const string& data) override {}

        void warning(const string& chars) override { cout << "VRCOLLADA_Stream Warning" << endl; }
        void error(const string& chars) override { cout << "VRCOLLADA_Stream Error" << endl; }
        void fatalError(const string& chars) override { cout << "VRCOLLADA_Stream Fatal Error" << endl; }
        void onException(exception& e) override { cout << "VRCOLLADA_Stream Exception" << endl; }
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

    // enter section
    if (name == "asset") currentSection = name;
    if (name == "library_geometries") currentSection = name;
    if (name == "library_materials" || name == "library_effects" || name == "library_images") currentSection = name;
    if (name == "library_joints" || name == "library_kinematics_models" || name == "library_articulated_systems" || name == "library_kinematics_scenes") currentSection = name;
    if (name == "scene" || name == "library_visual_scenes" || name == "library_nodes") {
        currentSection = name;
        scene.setCurrentSection(name);
    }

    // materials and textures
    if (name == "surface") materials.addSurface(parent.attributes["sid"].val);
    if (name == "sampler2D") materials.addSampler(parent.attributes["sid"].val);
    if (name == "effect") materials.newEffect( n.attributes["id"].val );
    if (name == "material") materials.newMaterial( n.attributes["id"].val, n.attributes["name"].val );
    if (name == "instance_effect") materials.setMaterialEffect( skipHash(n.attributes["url"].val) );

    // geometric data
    if (name == "geometry") geometries.newGeometry(n.attributes["name"].val, n.attributes["id"].val);
    if (name == "source") geometries.newSource(n.attributes["id"].val);
    if (name == "accessor") geometries.handleAccessor(n.attributes["count"].val, n.attributes["stride"].val);
    if (name == "input") geometries.handleInput(n.attributes["semantic"].val, skipHash(n.attributes["source"].val), n.attributes["offset"].val, n.attributes["set"].val);
    if (name == "triangles") geometries.newPrimitive(name, n.attributes["count"].val);
    if (name == "trifans") geometries.newPrimitive(name, n.attributes["count"].val);
    if (name == "tristrips") geometries.newPrimitive(name, n.attributes["count"].val);
    if (name == "polylist") geometries.newPrimitive(name, n.attributes["count"].val);

    // scene graphs
    if (name == "instance_geometry") scene.instantiateGeometry(skipHash(n.attributes["url"].val), &geometries);
    if (name == "instance_material") scene.setMaterial(skipHash(n.attributes["target"].val), &materials);
    if (name == "instance_node") scene.instantiateNode(skipHash(n.attributes["url"].val), fPath);
    if (name == "node" || name == "visual_scene") scene.newNode(n.attributes["id"].val, n.attributes["name"].val);

    // actual scene
    if (name == "instance_visual_scene") scene.instantiateScene(skipHash(n.attributes["url"].val));
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
