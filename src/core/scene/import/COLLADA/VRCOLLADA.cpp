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
#include "core/objects/VRKeyFrameAnimation.h"
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
        VRTransformPtr root;
        PosePtr rootPose;
        map<string, VRObjectWeakPtr> objects;
        map<string, VRObjectPtr> library_nodes;
        map<string, VRObjectPtr> library_scenes;
        string currentSection;

        VRSchedulerPtr scheduler;
        stack<VRObjectPtr> objStack;
        VRGeometryPtr lastInstantiatedGeo;

    public:
        VRCOLLADA_Scene() { scheduler = VRScheduler::create(); }

        void setRootOrientation(string upAxis) {
            if (upAxis == "X_UP") rootPose = Pose::create(Vec3d(), Vec3d(0,0, 1), Vec3d(1,0,0));
            if (upAxis == "Y_UP") rootPose = Pose::create(Vec3d(), Vec3d(0,0,-1), Vec3d(0,1,0));
            if (upAxis == "Z_UP") rootPose = Pose::create(Vec3d(), Vec3d(0,-1,0), Vec3d(0,0,1));
            if (root && rootPose) {
                root->setPose(rootPose);
                //root->setIdentity();
            }
        }

        void setRoot(VRTransformPtr r) { root = r; if (root && rootPose) root->setPose(rootPose); }
        void closeNode() { objStack.pop(); }
        VRObjectPtr top() { return objStack.size() > 0 ? objStack.top() : 0; }

        map<string, VRObjectWeakPtr>& getObjects() { return objects; }
        bool hasScene(string name) { return library_scenes.count(name); }

        void setCurrentSection(string s) {
            currentSection = s;
            objStack = stack<VRObjectPtr>();
            lastInstantiatedGeo = 0;
        }

		void finalize() {
		    scheduler->callPostponed(true);

		    auto geos = root->getChildren(true, "Geometry");
		    for (auto g : geos) {
                auto geo = dynamic_pointer_cast<VRTransform>(g);
                auto trans = dynamic_pointer_cast<VRTransform>(g->getParent());
                if (!geo || !trans) continue;
                auto parent = trans->getParent();
                if (!parent) continue;

                geo->setPose(trans->getPose());
                parent->addChild(geo);
                for (auto child : trans->getChildren()) geo->addChild(child);

                for (auto o : objects) {
                    if (auto O = o.second.lock()) {
                        if (O == trans) {
                            objects[o.first] = geo;
                            break;
                        }
                    }
                }

                trans->destroy();
		    }
        }

		void applyMatrix(Matrix4d m) {
            auto t = dynamic_pointer_cast<VRTransform>(top());
            if (t) {
                auto M = t->getMatrix();
                M.mult(m);
                t->setMatrix(M);
            }
		}

        void setMatrix(string data) {
            Matrix4d m = toValue<Matrix4d>(data);
            m.transpose();
            applyMatrix(m);
        }

        void translate(string data) { // TODO: read in specs what these are all about, like this it messes up the AML test scene
            Vec3d v = toValue<Vec3d>(data);
            Matrix4d m;
            m.setTranslate(v);
            applyMatrix(m);
        }

        void rotate(string data) { // TODO: read in specs what these are all about, like this it messes up the AML test scene
            Vec4d v = toValue<Vec4d>(data);
            double a = v[3]/180.0*Pi;
            Quaterniond q(Vec3d(v[0], v[1], v[2]), a);

            Matrix4d m;
            m.setRotate(q);
            applyMatrix(m);
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
            if (name == "") name = id;
            auto obj = VRTransform::create(name);
            objects[id] = obj;

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
            if (parent) {
                parent->remTag("COLLADA-postponed");
                parent->addChild( lastInstantiatedGeo );
            }
        }

        void instantiateNode(string url, string fPath, VRObjectPtr parent = 0) {
            if (!parent) parent = top();

            if (!library_nodes.count(url)) {
                string file = splitString(url, '#')[0];
                string node = splitString(url, '#')[1];

                if (file != "") { // reference another file
                    // TODO, use node to get correct object
                    VRTransformPtr obj = VRTransform::create(file);
                    map<string, string> options;
                    loadCollada( fPath + "/" + file, obj, options );
                    obj->setIdentity(); // dont turn coord system multiple times!
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

        void mergeGeometryNodes() { // merge geometry with parent transform nodes

        }

        void instantiateScene(string url) {
            if (library_scenes.count(url)) {
                for (auto child : library_scenes[url]->getChildren())
                    root->addChild(child);
            }
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
        map<string, string> options;
        stack<Node> nodeStack;

        string currentSection;
        VRCOLLADA_Material materials;
        VRCOLLADA_Geometry geometries;
        VRCOLLADA_Kinematics kinematics;
        VRCOLLADA_Scene scene;

        string skipHash(const string& s) { return (s[0] == '#') ? subString(s, 1, s.size()-1) : s; }

    public:
        VRCOLLADA_Stream(VRTransformPtr root, string fPath, map<string, string> opts) : fPath(fPath), options(opts) {
            scene.setRoot(root);
            materials.setFilePath(fPath);
        }

        ~VRCOLLADA_Stream() {}

        static VRCOLLADA_StreamPtr create(VRTransformPtr root, string fPath, map<string, string> opts) { return VRCOLLADA_StreamPtr( new VRCOLLADA_Stream(root, fPath, opts) ); }

        void startDocument() override {}

        void endDocument() override {
            materials.finalize();
            geometries.finalize();
            scene.finalize();
            kinematics.finalize(scene.getObjects());
        }

        void startElement(const string& uri, const string& name, const string& qname, const map<string, XMLAttribute>& attributes) override;
        void endElement(const string& uri, const string& name, const string& qname) override;
        void characters(const string& chars) override;
        void processingInstruction(const string& target, const string& data) override {}

        void warning(const string& chars) override { cout << "VRCOLLADA_Stream Warning" << endl; }
        void error(const string& chars) override { cout << "VRCOLLADA_Stream Error " << getLine() << "," << getColumn() << ": " << chars << endl; }
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
    if (name == "library_animations") currentSection = name;
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
    if (currentSection == "library_geometries") {
        if (name == "geometry") geometries.newGeometry(n.attributes["name"].val, n.attributes["id"].val);
        if (name == "source") geometries.newSource(n.attributes["id"].val);
        if (name == "accessor") geometries.handleAccessor(n.attributes["count"].val, n.attributes["stride"].val);
        if (name == "input") geometries.handleInput(n.attributes["semantic"].val, skipHash(n.attributes["source"].val), n.attributes["offset"].val, n.attributes["set"].val);
        if (name == "points") geometries.newPrimitive(name, n.attributes["count"].val);
        if (name == "lines") geometries.newPrimitive(name, n.attributes["count"].val);
        if (name == "triangles") geometries.newPrimitive(name, n.attributes["count"].val);
        if (name == "trifans") geometries.newPrimitive(name, n.attributes["count"].val);
        if (name == "tristrips") geometries.newPrimitive(name, n.attributes["count"].val);
        if (name == "polylist") geometries.newPrimitive(name, n.attributes["count"].val);
    }

    // animations
    if (currentSection == "library_animations") {
        if (name == "animation") kinematics.newAnimation( n.attributes["id"].val, n.attributes["name"].val);
        if (name == "source") kinematics.newSource(n.attributes["id"].val);
        if (name == "accessor") kinematics.handleAccessor(n.attributes["count"].val, n.attributes["stride"].val);
        if (name == "sampler") kinematics.newSampler(n.attributes["id"].val);
        if (name == "input") kinematics.handleInput(n.attributes["semantic"].val, skipHash(n.attributes["source"].val));
        if (name == "channel") kinematics.handleChannel(skipHash(n.attributes["source"].val), n.attributes["target"].val);
    }

    // scene graphs
    if (name == "instance_geometry") scene.instantiateGeometry(skipHash(n.attributes["url"].val), &geometries);
    if (name == "instance_material") scene.setMaterial(skipHash(n.attributes["target"].val), &materials);
    if (name == "instance_node") scene.instantiateNode(skipHash(n.attributes["url"].val), fPath);
    if (name == "node" || name == "visual_scene") scene.newNode(n.attributes["id"].val, n.attributes["name"].val);

    // actual scene
    if (name == "instance_visual_scene") {
        string sceneName = skipHash(n.attributes["url"].val);
        if (options.count("scene"))
            if (scene.hasScene(options["scene"]))
                sceneName = options["scene"];
        scene.instantiateScene(sceneName);
    }
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
    if (node.name == "lambert") materials.setRendering(node.name);
    if (node.name == "phong") materials.setRendering(node.name);
    if (node.name == "constant") materials.setRendering(node.name);
    if (node.name == "texture") materials.setTexture(node.attributes["texture"].val);
    if (node.name == "p") geometries.handleIndices(node.data);
    if (node.name == "points") geometries.closePrimitive();
    if (node.name == "lines") geometries.closePrimitive();
    if (node.name == "triangles") geometries.closePrimitive();
    if (node.name == "trifans") geometries.closePrimitive();
    if (node.name == "tristrips") geometries.closePrimitive();
    if (node.name == "polylist") geometries.closePrimitive();
    if (node.name == "vcount") geometries.handleVCount(node.data);

    if (currentSection == "library_geometries") {
        if (node.name == "float_array") geometries.setSourceData(node.data);
    }

    if (currentSection == "library_animations") {
        if (node.name == "float_array") kinematics.setSourceData(node.data);
        if (node.name == "Name_array") kinematics.setSourceStrData(node.data);
    }

    if (node.name == "float") {
        float f = toValue<float>(node.data);
        string sid = node.attributes["sid"].val;
        if (sid == "shininess") materials.setShininess(f);
    }

    if (node.name == "up_axis") scene.setRootOrientation(node.data);
    if (node.name == "node" || node.name == "visual_scene") scene.closeNode();
    if (node.name == "matrix") scene.setMatrix(node.data);
    if (node.name == "translate") scene.translate(node.data);
    if (node.name == "rotate") scene.rotate(node.data);

    // animations
    if (node.name == "animation") kinematics.endAnimation();
}

void OSG::loadCollada(string path, VRTransformPtr root, map<string, string> options) {
    auto xml = XML::create();
    string fPath = getFolderName(path);
    auto handler = VRCOLLADA_Stream::create(root, fPath, options);
    xml->stream(path, handler.get());
}

string create_timestamp() {
    time_t _tm = time(0);
    struct tm * curtime = localtime ( &_tm );
    ostringstream ss;
    ss << put_time(curtime, "%Y-%m-%dT%H:%M:%S"); // 2021-12-06T13:51:03
    return string(ss.str());
}

void OSG::writeCollada(VRObjectPtr root, string path, map<string, string> options) {
    bool exportChildren = bool(root->getBaseName() == "proxy");

    string version = "1.5.0"; // "1.4.1"
    if (options.count("version")) version = options["version"];
    ofstream stream(path);

    function<bool(VRObjectPtr)> isVisible = [&](VRObjectPtr o) -> bool {
        if (!o->isVisible()) return false;
        if (o == root) return true;
        if (!isVisible(o->getParent())) return false;
        return true;
    };

    function<void(VRObjectPtr, int, bool)> writeSceneGraph = [&](VRObjectPtr node, int indent, bool onlyChildren) -> void {
        if (!node->isVisible()) return;
        string identStr = "";
        for (int i=0; i< indent; i++) identStr += "\t";
        string name = node->getName();
        VRTransformPtr trans = dynamic_pointer_cast<VRTransform>(node);
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(node);
        VRMaterialPtr mat;
        if (geo) mat = geo->getMaterial();
        string matName;
        if (mat) matName = mat->getName();

        if (!onlyChildren) {
            stream << identStr << "<node id=\"" << name << "\" name=\"" << name << "\" type=\"NODE\">" << endl;

            if (trans) {
                bool hasAnim = false;

                if (!hasAnim) {
                    Matrix4d m = trans->getMatrix();
                    m.transpose();
                    stream << identStr << "\t<matrix sid=\"transform\">" << toString(m) << "</matrix>" << endl;
                }

                for (auto a : trans->getAnimations()) {
                    auto kfAnim = dynamic_pointer_cast<VRKeyFrameAnimation>(a);
                    if (!kfAnim) continue;
                    hasAnim = true;

                    map<string, bool> properties;

                    for (auto& s : kfAnim->getSamplers()) {
                        auto& sampler = s.second;
                        string property = sampler.property;

                        if (contains(property,"rotationX") && !properties.count("rotationX")) {
                            Vec3d e = trans->getEuler();
                            stream << identStr << "\t<rotate sid=\"rotationX\">1 0 0 " << e[0] << "</rotate>" << endl;
                            properties["rotationX"] = true;
                        }

                        if (contains(property,"rotationY") && !properties.count("rotationY")) {
                            Vec3d e = trans->getEuler();
                            stream << identStr << "\t<rotate sid=\"rotationY\">0 1 0 " << e[1] << "</rotate>" << endl;
                            properties["rotationY"] = true;
                        }

                        if (contains(property,"rotationZ") && !properties.count("rotationZ")) {
                            Vec3d e = trans->getEuler();
                            stream << identStr << "\t<rotate sid=\"rotationZ\">0 0 1 " << e[2] << "</rotate>" << endl;
                            properties["rotationZ"] = true;
                        }

                        if (contains(property,"location") && !properties.count("location")) {
                            Vec3d p = trans->getFrom();
                            stream << identStr << "\t<translate sid=\"location\">" << toString(p) << "</translate>" << endl;
                            properties["location"] = true;
                        }
                    }
                }
            }

            if (geo) {
                stream << identStr << "\t<instance_geometry url=\"#" << name << "_mesh\" name=\"" << name << "\">" << endl;
                stream << identStr << "\t\t<bind_material>" << endl;
                stream << identStr << "\t\t\t<technique_common>" << endl;
                stream << identStr << "\t\t\t\t<instance_material symbol=\"" << matName << "\" target=\"#" << matName << "\"/>" << endl;
                stream << identStr << "\t\t\t</technique_common>" << endl;
                stream << identStr << "\t\t</bind_material>" << endl;
                stream << identStr << "\t</instance_geometry>" << endl;
            }
        } else indent--;

		if (node->getChildrenCount() > 0) {
            for (auto child : node->getChildren()) {
                writeSceneGraph(child, indent+1, false);
            }
		}
		if (!onlyChildren) stream << identStr << "</node>" << endl;
    };

    stream << "﻿<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl;
    if (version == "1.4.1") stream << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"" << version << "\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" << endl;
    else                    stream << "<COLLADA xmlns=\"http://www.collada.org/2008/03/COLLADASchema\" version=\"" << version << "\">" << endl;

    string timestamp = create_timestamp();
    stream << "﻿\t<asset>" << endl;
    stream << "﻿\t\t<contributor>" << endl;
    stream << "﻿\t\t\t<author>PolyVR User</author>" << endl;
	stream << "﻿\t\t\t<author_website>https://github.com/Victor-Haefner/polyvr</author_website>" << endl;
    stream << "﻿\t\t\t<authoring_tool>PolyVR</authoring_tool>" << endl;
    stream << "﻿\t\t</contributor>" << endl;
    stream << "﻿\t\t<created>" << timestamp << "</created>" << endl;
    stream << "﻿\t\t<modified>" << timestamp << "</modified>" << endl;
    stream << "﻿\t\t<unit name=\"meter\" meter=\"1\"/>" << endl;
    stream << "﻿\t\t<up_axis>Y_UP</up_axis>" << endl;
    stream << "﻿\t</asset>" << endl;

    auto getGeometries = [](VRObjectPtr root) {
        auto objs = root->getChildren(true, "", true);

        vector<VRGeometryPtr> geos;
        for (auto obj : objs) {
            VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
            if (!geo) continue;
            if (geo->getType() == "AnnotationEngine") continue;
            geos.push_back(geo);
        }
        return geos;
    };

    auto getTransforms = [](VRObjectPtr root) {
        auto objs = root->getChildren(true, "", true);

        vector<VRTransformPtr> trans;
        for (auto obj : objs) {
            VRTransformPtr tr = dynamic_pointer_cast<VRTransform>(obj);
            if (!tr) continue;
            trans.push_back(tr);
        }
        return trans;
    };

    auto geos = getGeometries(root);
    auto trans = getTransforms(root);

    map<string, VRMaterialPtr> materials;
    for (auto geo : geos) {
        if (!geo) continue;
        auto mat = geo->getMaterial();
        materials[mat->getName()] = mat;
    }

    stream << "﻿\t<library_effects>" << endl;
    for (auto mat : materials) {
        string name = mat.second->getName();
        Color3f d = mat.second->getDiffuse();
        float t = mat.second->getTransparency();
        string rendering = mat.second->isLit() ? "lambert" : "constant"; // there is also phong

        stream << "﻿\t\t<effect id=\"" << name << "_effect\">" << endl;
        stream << "﻿\t\t\t<profile_COMMON>" << endl;
        stream << "﻿\t\t\t\t<technique sid=\"common\">" << endl;
        stream << "﻿\t\t\t\t\t<" << rendering << ">" << endl;
        stream << "﻿\t\t\t\t\t\t<emission>" << endl;
        stream << "﻿\t\t\t\t\t\t\t<color sid=\"emission\">0 0 0 1</color>" << endl;
        stream << "﻿\t\t\t\t\t\t</emission>" << endl;
        stream << "﻿\t\t\t\t\t\t<diffuse>" << endl;
        stream << "﻿\t\t\t\t\t\t\t<color sid=\"diffuse\">" << d[0] << " " << d[1] << " " << d[2] << " " << t << "</color>" << endl;
        stream << "﻿\t\t\t\t\t\t</diffuse>" << endl;
        stream << "﻿\t\t\t\t\t\t<index_of_refraction>" << endl;
        stream << "﻿\t\t\t\t\t\t\t<float sid=\"ior\">1.45</float>" << endl;
        stream << "﻿\t\t\t\t\t\t</index_of_refraction>" << endl;
        stream << "﻿\t\t\t\t\t</" << rendering << ">" << endl;
        stream << "﻿\t\t\t\t</technique>" << endl;
        stream << "﻿\t\t\t</profile_COMMON>" << endl;
        stream << "﻿\t\t</effect>" << endl;
    }
    stream << "﻿\t</library_effects>" << endl;

    stream << "\t<library_images/>" << endl;

    stream << "﻿\t<library_materials>" << endl;
    for (auto mat : materials) {
        string name = mat.second->getName();
        stream << "﻿\t\t<material id=\"" << name << "\" name=\"" << name << "\">" << endl;
        stream << "﻿\t\t\t<instance_effect url=\"#" << name << "_effect\"/>" << endl;
        stream << "﻿\t\t</material>" << endl;
    }
    stream << "﻿\t</library_materials>" << endl;


    stream << "\t<library_geometries>" << endl;
    for (auto geo : geos) {
        if (!isVisible(geo)) continue;
        string name = geo->getName();
        VRGeoData data(geo);
        auto mat = geo->getMaterial();

        stream << "\t\t<geometry id=\"" << name << "_mesh\" name=\"" << name << "\">" << endl;
        stream << "\t\t\t<mesh>" << endl;

        stream << "\t\t\t\t<source id=\"" << name << "_positions\">" << endl;
        stream << "\t\t\t\t\t<float_array count=\"" << data.size()*3 << "\" id=\"" << name << "_positions_array\">";
        for (int i=0; i<data.size(); i++) {
            if (i > 0) stream << " ";
            Pnt3d p = data.getPosition(i);
            stream << p[0] << " " << p[1] << " " << p[2];
        }
        stream << "</float_array>" << endl;
		stream << "\t\t\t\t\t<technique_common>" << endl;
		stream << "\t\t\t\t\t\t<accessor source=\"#" << name << "_positions_array\" count=\"" << data.size() << "\" stride=\"3\">" << endl;
		stream << "\t\t\t\t\t\t\t<param name=\"X\" type=\"float\"/>" << endl;
		stream << "\t\t\t\t\t\t\t<param name=\"Y\" type=\"float\"/>" << endl;
		stream << "\t\t\t\t\t\t\t<param name=\"Z\" type=\"float\"/>" << endl;
		stream << "\t\t\t\t\t\t</accessor>" << endl;
		stream << "\t\t\t\t\t</technique_common>" << endl;
        stream << "\t\t\t\t</source>" << endl;

        bool doNormals = bool(data.sizeNormals() > 0);
        if (doNormals) {
            stream << "\t\t\t\t<source id=\"" << name << "_normals\">" << endl;
            stream << "\t\t\t\t\t<float_array count=\"" << data.sizeNormals()*3 << "\" id=\"" << name << "_normals_array\">";
            for (int i=0; i<data.sizeNormals(); i++) {
                if (i > 0) stream << " ";
                Vec3d n = data.getNormal(i);
                stream << n[0] << " " << n[1] << " " << n[2];
            }
            stream << "</float_array>" << endl;
            stream << "\t\t\t\t\t<technique_common>" << endl;
            stream << "\t\t\t\t\t\t<accessor source=\"#" << name << "_normals_array\" count=\"" << data.sizeNormals() << "\" stride=\"3\">" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"X\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"Y\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"Z\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t</accessor>" << endl;
            stream << "\t\t\t\t\t</technique_common>" << endl;
            stream << "\t\t\t\t</source>" << endl;
        }

        bool doColor3s = bool(data.sizeColor3s() > 0);
        bool doColor4s = bool(data.sizeColor4s() > 0);
        bool doColors = bool(doColor3s || doColor4s);
        if (doColors) {
            int Ncols = max(data.sizeColor3s(), data.sizeColor4s());
            int Nchannels = doColor3s ? 3 : 4;
            stream << "\t\t\t\t<source id=\"" << name << "_colors\">" << endl;
            stream << "\t\t\t\t\t<float_array count=\"" << Ncols*Nchannels << "\" id=\"" << name << "_colors_array\">";
            for (int i=0; i<Ncols; i++) {
                if (i > 0) stream << " ";
                if (doColor3s) {
                    Color3f n = data.getColor3(i);
                    stream << n[0] << " " << n[1] << " " << n[2];
                }
                if (doColor4s) {
                    Color4f n = data.getColor(i);
                    stream << n[0] << " " << n[1] << " " << n[2] << " " << n[3];
                }
            }
            stream << "</float_array>" << endl;
            stream << "\t\t\t\t\t<technique_common>" << endl;
            stream << "\t\t\t\t\t\t<accessor source=\"#" << name << "_normals_array\" count=\"" << Ncols << "\" stride=\"" << Nchannels << "\">" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"R\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"G\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"B\" type=\"float\"/>" << endl;
            if (doColor4s) stream << "\t\t\t\t\t\t\t<param name=\"A\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t</accessor>" << endl;
            stream << "\t\t\t\t\t</technique_common>" << endl;
            stream << "\t\t\t\t</source>" << endl;
        }

        bool doTexCoords = bool(data.sizeTexCoords() > 0);
        if (doTexCoords) {
            stream << "\t\t\t\t<source id=\"" << name << "_texcoords\">" << endl;
            stream << "\t\t\t\t\t<float_array count=\"" << data.sizeTexCoords()*2 << "\" id=\"" << name << "_texcoords_array\">";
            for (int i=0; i<data.sizeTexCoords(); i++) {
                if (i > 0) stream << " ";
                Vec2d n = data.getTexCoord(i);
                stream << n[0] << " " << n[1];
            }
            stream << "</float_array>" << endl;
            stream << "\t\t\t\t\t<technique_common>" << endl;
            stream << "\t\t\t\t\t\t<accessor source=\"#" << name << "_normals_array\" count=\"" << data.sizeTexCoords() << "\" stride=\"2\">" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"U\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t\t<param name=\"V\" type=\"float\"/>" << endl;
            stream << "\t\t\t\t\t\t</accessor>" << endl;
            stream << "\t\t\t\t\t</technique_common>" << endl;
            stream << "\t\t\t\t</source>" << endl;
        }

        stream << "\t\t\t\t<vertices id=\"" << name << "_vertices\">" << endl;
		stream << "\t\t\t\t\t<input semantic=\"POSITION\" source=\"#" << name << "_positions\" />" << endl;
		stream << "\t\t\t\t</vertices>" << endl;

		size_t N0 = 0;
        for (int i=0; i<data.getNTypes(); i++) {
            int type = data.getType(i);
            int length = data.getLength(i);

            int faceCount = length/3;
            string typeName = "triangles";
            if (type == GL_POINTS) { typeName = "points"; faceCount = length; }
            if (type == GL_LINES) { typeName = "lines"; faceCount = length/2; }
            if (type == GL_TRIANGLE_FAN) { typeName = "trifans"; faceCount = 1; }
            if (type == GL_TRIANGLE_STRIP) { typeName = "tristrips"; faceCount = 1; }
            if (type == GL_QUADS) { typeName = "triangles"; faceCount = length/4 * 2; } // times two because we convert to triangles
            if (type == GL_QUAD_STRIP) { typeName = "quadstrips"; faceCount = 1; } // probably not supported..

            int offset = 0;
            stream << "\t\t\t\t<" << typeName << " count=\"" << faceCount << "\" material=\"" << mat->getName() << "\">" << endl;
            stream << "\t\t\t\t\t<input offset=\"" << offset++ << "\" semantic=\"VERTEX\" source=\"#" << name << "_vertices\" />" << endl;
            if (doNormals) stream << "\t\t\t\t\t<input offset=\"" << offset++ << "\" semantic=\"NORMAL\" source=\"#" << name << "_normals\" />" << endl;
            if (doColors)  stream << "\t\t\t\t\t<input offset=\"" << offset++ << "\" semantic=\"COLOR\" source=\"#" << name << "_colors\" />" << endl;
            if (doTexCoords)  stream << "\t\t\t\t\t<input offset=\"" << offset++ << "\" semantic=\"TEXCOORD\" source=\"#" << name << "_texcoords\" set=\"0\"/>" << endl;
            stream << "\t\t\t\t\t<p>";
            if (type == GL_QUADS) { // convert to triangles on the fly
                for (int i=0; i<length/4; i++) {
                    if (i > 0) stream << " ";
                    stream << data.getIndex(N0+i*4+0) << " ";
                    if (doNormals) stream << data.getIndex(N0+i*4+0, NormalsIndex) << " ";
                    if (doColors)  stream << data.getIndex(N0+i*4+0, ColorsIndex) << " ";
                    if (doTexCoords)  stream << data.getIndex(N0+i*4+0, TexCoordsIndex) << " ";
                    stream << data.getIndex(N0+i*4+1) << " ";
                    if (doNormals) stream << data.getIndex(N0+i*4+1, NormalsIndex) << " ";
                    if (doColors)  stream << data.getIndex(N0+i*4+1, ColorsIndex) << " ";
                    if (doTexCoords)  stream << data.getIndex(N0+i*4+1, TexCoordsIndex) << " ";
                    stream << data.getIndex(N0+i*4+2) << " ";
                    if (doNormals) stream << data.getIndex(N0+i*4+2, NormalsIndex) << " ";
                    if (doColors)  stream << data.getIndex(N0+i*4+2, ColorsIndex) << " ";
                    if (doTexCoords)  stream << data.getIndex(N0+i*4+2, TexCoordsIndex) << " ";
                    stream << data.getIndex(N0+i*4+0) << " ";
                    if (doNormals) stream << data.getIndex(N0+i*4+0, NormalsIndex) << " ";
                    if (doColors)  stream << data.getIndex(N0+i*4+0, ColorsIndex) << " ";
                    if (doTexCoords)  stream << data.getIndex(N0+i*4+0, TexCoordsIndex) << " ";
                    stream << data.getIndex(N0+i*4+2) << " ";
                    if (doNormals) stream << data.getIndex(N0+i*4+2, NormalsIndex) << " ";
                    if (doColors)  stream << data.getIndex(N0+i*4+2, ColorsIndex) << " ";
                    if (doTexCoords)  stream << data.getIndex(N0+i*4+2, TexCoordsIndex) << " ";
                    stream << data.getIndex(N0+i*4+3);
                    if (doNormals) stream << " " << data.getIndex(N0+i*4+3, NormalsIndex);
                    if (doColors)  stream << " " << data.getIndex(N0+i*4+3, ColorsIndex);
                    if (doTexCoords)  stream << " " << data.getIndex(N0+i*4+3, TexCoordsIndex);
                }
            } else {
                for (int i=0; i<length; i++) {
                    if (i > 0) stream << " ";
                    stream << data.getIndex(N0+i);
                    if (doNormals) stream << " " << data.getIndex(N0+i, NormalsIndex);
                    if (doColors) stream << " " << data.getIndex(N0+i, ColorsIndex);
                    if (doTexCoords) stream << " " << data.getIndex(N0+i, TexCoordsIndex);
                }
            }
            stream << "</p>" << endl;
            stream << "\t\t\t\t</" << typeName << ">" << endl;
            N0 += length;
        }

        stream << "\t\t\t</mesh>" << endl;
        stream << "\t\t</geometry>" << endl;
    }
    stream << "\t</library_geometries>" << endl;

    stream << "\t<library_animations>" << endl;
    for (auto t : trans) {
        for (auto a : t->getAnimations()) {
            auto kfAnim = dynamic_pointer_cast<VRKeyFrameAnimation>(a);
            if (!kfAnim) continue;

            auto sources = kfAnim->getSources();
            auto samplers = kfAnim->getSamplers();

            string name = kfAnim->getName();
            stream << "\t\t<animation id=\"action_container_" << name << "\" name=\"" << name << "\">" << endl;
            for (auto& s : samplers) {
                auto& sampler = s.second;
                auto tObj = sampler.target.lock();
                if (!tObj) continue;
                string tName = tObj->getName();
                stream << "\t\t\t<animation id=\"anim_" << s.first << "\" name=\"" << name << "\">" << endl;

                string paramIn = "TIME";
                string paramOut = "TRANSFORM"; // for property transform
                if (sampler.property == "location.X") paramOut = "X";
                if (sampler.property == "location.Y") paramOut = "Y";
                if (sampler.property == "location.Z") paramOut = "Z";
                if (contains(sampler.property, "ANGLE")) paramOut = "ANGLE";

                for (auto& so : sampler.sources) {
                    string sourceID = so.second;
                    auto& source = sources[sourceID];
                    stream << "\t\t\t\t<source id=\"" << sourceID << "\">" << endl;
                    string type = "float";

                    string param = "INTERPOLATION";
                    if (so.first == "INPUT") param = paramIn;
                    if (so.first == "OUTPUT") param = paramOut;

                    if (source.data.size() > 0) {
                        stream << "\t\t\t\t\t<float_array id=\"" << sourceID << "_array\" count=\"" << source.data.size() << "\">";
                        for (int i=0; i<source.data.size(); i++) {
                            if (i > 0) stream << " ";
                            stream << source.data[i];
                        }
                        stream << "</float_array>" << endl;
                    }

                    if (source.strData.size() > 0) {
                        type = "name";
                        stream << "\t\t\t\t\t<Name_array id=\"" << sourceID << "_array\" count=\"" << source.strData.size() << "\">";
                        for (int i=0; i<source.strData.size(); i++) {
                            if (i > 0) stream << " ";
                            stream << source.strData[i];
                        }
                        stream << "</Name_array>" << endl;
                    }

                    stream << "\t\t\t\t\t<technique_common>" << endl;
                    stream << "\t\t\t\t\t\t<accessor source=\"#" << sourceID << "_array\" count=\"" << source.count << "\" stride=\"" << source.stride << "\">" << endl;
                    stream << "\t\t\t\t\t\t\t<param name=\"" << param << "\" type=\"" << type << "\"/>" << endl;
                    stream << "\t\t\t\t\t\t</accessor>" << endl;
                    stream << "\t\t\t\t\t</technique_common>" << endl;
                    stream << "\t\t\t\t</source>" << endl;
                }

                stream << "\t\t\t\t<sampler id=\"" << s.first << "\">" << endl;
                for (auto& so : sampler.sources) {
                    string sourceID = so.second;
                    auto& source = sources[sourceID];
                    stream << "\t\t\t\t\t<input semantic=\"" << so.first << "\" source=\"#" << sourceID << "\"/>" << endl;
                }
                stream << "\t\t\t\t</sampler>" << endl;
                stream << "\t\t\t\t<channel source=\"#" << s.first << "\" target=\"" << tName << "/" << sampler.property << "\"/>" << endl;
                stream << "\t\t\t</animation>" << endl;
            }
            stream << "\t\t</animation>" << endl;
        }
    }
    stream << "\t</library_animations>" << endl;

    // the scenes, actually only a single one..
	stream << "\t<library_visual_scenes>" << endl;
	stream << "\t\t<visual_scene id=\"the_visual_scene\">" << endl;
	writeSceneGraph(root, 3, exportChildren);
	stream << "\t\t</visual_scene>" << endl;
	stream << "\t</library_visual_scenes>" << endl;

    // the scene
    stream << "\t<scene>" << endl;
    stream << "\t\t<instance_visual_scene url=\"#the_visual_scene\" />" << endl;
    stream << "\t</scene>" << endl;

    stream << "﻿</COLLADA>" << endl;
}


