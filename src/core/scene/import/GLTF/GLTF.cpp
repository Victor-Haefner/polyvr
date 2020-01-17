#include "GLTF.h"
#include "core/utils/VRProgress.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"

#include "core/scene/VRScene.h"

#include <string>
#include <iostream>
#include <fstream>
#include <stack>

#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGQuaternion.h>

//#include "core/objects/geometry/OSGGeometry.h"
//#include <OpenSG/OSGGeoProperties.h>
//#include <OpenSG/OSGGeometry.h>

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

using namespace OSG;
using namespace std::placeholders;

struct GLTFSchema {
    int version = 1;

    struct FieldRef {
        string type;
        string def;
    };

    struct NodeRef {
        map<string, FieldRef> fieldRefs;
    };

    map<string, NodeRef> nodeRefs;

    void addNodeRef(string node, vector<string> fields, vector<string> types, vector<string> defaults) {
        nodeRefs[node] = NodeRef();
        for (uint i=0; i<fields.size(); i++) {
            nodeRefs[node].fieldRefs[fields[i]] = FieldRef();
            nodeRefs[node].fieldRefs[fields[i]].type = types[i];
            nodeRefs[node].fieldRefs[fields[i]].def  = defaults[i];
        }
    }

    GLTFSchema(int version = 1) : version(version) {
        if (version == 1) { // TODO
        }

        if (version == 2) {
            addNodeRef("extensionsUsed", {"array"}, {"string"}, {""});
            addNodeRef("extensionsRequired", {"array"}, {"string"}, {""});
            addNodeRef("accessors", {"array"}, {"assessor"}, {"NULL"});
            addNodeRef("animations", {"array"}, {"animation"}, {"NULL"});
            addNodeRef("asset", {"array"}, {"asset"}, {"NULL"});
            addNodeRef("buffers", {"array"}, {"buffer"}, {"NULL"});
            addNodeRef("bufferViews", {"array"}, {"bufferView"}, {"NULL"});
            addNodeRef("cameras", {"array"}, {"camera"}, {"NULL"});
            addNodeRef("images", {"array"}, {"image"}, {"NULL"});
            addNodeRef("materials", {"array"}, {"material"}, {"NULL"});
            addNodeRef("meshes", {"array"}, {"mesh"}, {"NULL"});
            addNodeRef("nodes", {"array"}, {"node"}, {"NULL"});
            addNodeRef("samplers", {"array"}, {"sampler"}, {"NULL"});
            addNodeRef("scene", {"glTFid"}, {"int"}, {"NULL"});
            addNodeRef("scenes", {"array"}, {"scene"}, {"NULL"});
            addNodeRef("skins", {"array"}, {"skin"}, {"NULL"});
            addNodeRef("textures", {"array"}, {"texture"}, {"NULL"});

            addNodeRef("Material", {"name","extensions","extras","pbrMetallicRoughness","normalTexture","emissiveTexture", "emissiveFactor", "alphaMode", "alphaCutoff", "doubleSided" }, {"string","","","","","", "MFnumber", "SFstring", "SFnumber", "SFboolean" }, {"","","","","","", "[ 0.0, 0.0, 0.0 ]", "OPAQUE", "0.5", "false"});
            //alphaMode: OPAQUE, MASK, BLEND
        }
    }

    bool isNode(string& token) { return nodeRefs.count(token); }

    bool isFieldOf(string& node, string& token) {
        if (!isNode(node)) return false;
        return nodeRefs[node].fieldRefs.count(token);
    }

    FieldRef& getField(string& node, string& field) {
        return nodeRefs[node].fieldRefs[field];
    }
};

GLTFSchema gltfschema;


struct GLTFUtils {
    int version = 1;

    bool isGeometryNode(string node) {
        if (node == "Mesh") return true;
        if (node == "Primitive") return true;

        return false;
    }

    bool isPropertyNode(string node) {
        return false;
    }

    bool isNode(string node) {
        if (isGeometryNode(node)) return true;
        if (isPropertyNode(node)) return true;
        return false;
    }

    bool isTransformationNode(string node) {
        if (node == "Transform") return true;
        if (node == "Link") return true;

        return false;
    }

};

struct GLTFNode : GLTFUtils {
    string name;
    string type;
    int nID = -1;
    GLTFNode* parent = 0;
    vector<GLTFNode*> children;
    vector<Pnt3d> positions;
    vector<Vec3d> normals;
    vector<Color3f> colors;
    vector<Vec2d> texCoords;
    vector<int> coordIndex;
    vector<int> normalIndex;
    vector<int> colorIndex;
    vector<int> texCoordIndex;
    VRObjectPtr obj;
    VRMaterialPtr material;
    Matrix4d pose = Matrix4d();
    VRGeoData geoData;
    int matID = -1;

    Vec3d translation = Vec3d(0,0,0);
    Vec4d rotation = Vec4d(0,0,1,0);
    Vec3d scale = Vec3d(1,1,1);
    Matrix4d matTransform  = Matrix4d();


    GLTFNode(string t, string n = "Unnamed") : name(n), type(t) {}
    virtual ~GLTFNode() {
        for (auto c : children) delete c;
    }

    void addChild(GLTFNode* c) {
        c->parent = this;
        children.push_back(c);
    }

    vector<GLTFNode*> getSiblings() {
        vector<GLTFNode*> res;
        if (!parent) return res;
        for (auto c : parent->children) {
            if (c != this) res.push_back(c);
        }
        return res;
    }

    void print(string padding = "") {
        cout << padding << "Node '" << name << "'";
        VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
        if (g) cout << " '" << g->getName() << "'";
        cout << " " << nID;
        cout << " of type " << type;
        if (type == "Mesh" || type == "Primitive") cout << " with matID: " << matID;
        //if (type == "Mesh" && name == "Node") cout << "-------";
        if (type == "Primitive") cout << "----PRIM----";
        cout << endl;
        for (auto c : children) c->print(padding+" ");
    }


    // build OSG ---------------------
    VRObjectPtr makeObject() {
        //cout << "make object '" << name << "' of type " << type << endl;
        if (isGeometryNode(type)) {
            return VRGeometry::create(name);
        }
        if (isPropertyNode(type)) {
            if (type == "PointLight") return VRLight::create(name);
            if (type == "DirectionalLight") return VRLight::create(name);
            if (type == "SpotLight") return VRLight::create(name);
            if (type == "PerspectiveCamera") return VRCamera::create(name, false);
        }

        if (isTransformationNode(type)) return VRTransform::create(name);

        if (type == "Coordinate") return 0;
        if (type == "Normal") return 0;
        if (type == "Color") return 0;
        if (type == "Material") return 0;
        if (type == "Appearance") return 0;

        return VRObject::create(name);
    }

    void applyPose() {
        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        t->setMatrix(pose);
    }

    void applyMaterial() {
        VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
        if (g && material) g->setMaterial(material);
    }

    // transformation data
    void handleTranslation() {
        pose.setTranslate( translation );
    }

    void handleRotation() {
        Quaterniond q( rotation[0], rotation[1], rotation[2], rotation[3] );
        pose.setRotate( q );
    }

    void handleScale() {
        pose.setScale( scale );
    }

    void handleTransform() {
        Vec3d center = Vec3d(0,0,0);
        Vec4d scaleOrientation = Vec4d(0,0,1,0);
        Matrix4d m1; m1.setTranslate(translation+center); m1.setRotate( Quaterniond( rotation[0], rotation[1], rotation[2], rotation[3] ) );
        Matrix4d m2; m2.setRotate( Quaterniond( scaleOrientation[0], scaleOrientation[1], scaleOrientation[2], scaleOrientation[3] ) );
        Matrix4d m3; m3.setScale(scale);
        Matrix4d m4; m4.setTranslate(-center); m4.setRotate( Quaterniond( scaleOrientation[0], scaleOrientation[1], scaleOrientation[2], -scaleOrientation[3] ) );
        Matrix4d M = m1; M.mult(m2); M.mult(m3); M.mult(m4);
        pose = M;
    }

    void buildOSG() {
        if (!obj) {
            obj = makeObject();
            if (obj) {
                if (parent && parent->obj) parent->obj->addChild(obj);
                else cout << "WARNING in GLTFNode::buildOSG, cannot append object to parent!" << endl;
            }
            //applyProperties();
        }

        for (auto c : children) c->buildOSG();
    }

    virtual Matrix4d applyTransformations(Matrix4d m = Matrix4d()) = 0;
    virtual VRMaterialPtr applyMaterials() = 0;
    virtual VRGeoData applyGeometries() = 0;

    void resolveLinks(map<int, GLTFNode*>& references) {
        if (type == "Link") { if (references.count(nID)) obj->addChild(references[nID]->obj->duplicate()); }
        for (auto c : children) c->resolveLinks(references);
    }
};

struct GLTFNNode : GLTFNode{
    GLTFNNode(string type, string name = "Unnamed") : GLTFNode(type, name) { version = 2; }
    ~GLTFNNode() {}

    Matrix4d applyTransformations(Matrix4d m = Matrix4d()) {
        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        if (t) t->setMatrix(pose);

        for (auto c : children) c->applyTransformations(m);
        return m;
    }

    VRMaterialPtr applyMaterials() {
        if (isGeometryNode(type)) {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                if (material) {
                    g->setMaterial(material);
                }
            }
        }

        for (auto c : children) c->applyMaterials();
        return material;
    }

    VRGeoData applyGeometries() {
        if (type == "Mesh" || type == "Primitive") {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                if (geoData.size()>0) geoData.apply(g); //TODO: what if more than one primitive per mesh?
                else cout << g->getName() << " geoData with no data found" << endl;
            }
        }

        for (auto c : children) c->applyGeometries();
        return geoData;
    }
};

class GLTFLoader : public GLTFUtils {
    private:
        string path;
        VRTransformPtr res;
        VRProgressPtr progress;
        bool threaded = false;
        GLTFNode* tree = 0;
        map<int, GLTFNode*> references;
        tinygltf::Model model;
        map<int, GLTFNode*> nodes;
        map<int, GLTFNode*> meshes;
        map<int, GLTFNode*> primitives;
        map<int, GLTFNode*> scenes;
        map<int, int> nodeToMesh;
        map<int, int> primToMesh;
        map<int, vector<int>> meshToNodes;
        map<int, vector<int>> childrenPerNode;
        map<int, vector<int>> childrenPerScene;
        map<int, vector<int>> childrenPerMesh;
        map<int, VRMaterialPtr> materials;
        map<int, VRTexturePtr> textures;
        size_t sceneID = -1;
        size_t nodeID = -1;
        size_t meshID = -1;
        size_t matID = -1;
        size_t texID = -1;
        size_t primID = -1;

        static string GetFilePathExtension(const string &FileName) {
            if (FileName.find_last_of(".") != std::string::npos)
                return FileName.substr(FileName.find_last_of(".") + 1);
            return "";
        }

        bool openFile(ifstream& file) {
            file.open(path);
            if (!file.is_open()) { cout << "ERROR: file '" << path << "' not found!" << endl; return false; }
            return true;
        }

        size_t getFileSize(ifstream& file) {
            file.seekg(0, ios_base::end);
            size_t fileSize = file.tellg();
            file.seekg(0, ios_base::beg);
            return fileSize;
        }

        void handleAsset(const tinygltf::Asset &gltfAsset){
            //gltfAsset.version;
            //gltfAsset.extensions
            if (gltfAsset.extensions.size() > 0) {
                cout << "GLTFLOADER::WARNING IN EXTENSIONS not supported: ";
                for (auto each : gltfAsset.extensions) cout << " " << each.first;
                cout << endl;
            }
        }

        void handleScene(const tinygltf::Scene &gltfScene){
            sceneID++;
            string name = gltfScene.name;
            if (name == "") name = "SceneNode";
            GLTFNode* thisNode = new GLTFNNode("Scene",name);
            scenes[sceneID] = thisNode;
            for (auto each : gltfScene.nodes) childrenPerScene[sceneID].push_back(each);
        }

        void handleNode(const tinygltf::Node &gltfNode){
            nodeID++;
            string type = "Transform";
            string name = "Unnamed";
            Matrix4d mat4;
            mat4.setTranslate(Vec3d(0,0,0));
            Vec3d translation = Vec3d(0,0,0);
            Vec4d rotation = Vec4d(0,0,1,0);
            Vec3d scale = Vec3d(1,1,1);
            name = gltfNode.name;
            vector<bool> transf = {false, false, false, false};

            if (gltfNode.mesh > -1) {
                nodeToMesh[nodeID] = gltfNode.mesh;
                meshToNodes[gltfNode.mesh].push_back(nodeID);
                if (meshToNodes[gltfNode.mesh].size()>1) type = "Link";
            }

            if (gltfNode.translation.size() == 3) {
                auto v = gltfNode.translation;
                translation = Vec3d( v[0], v[1], v[2] );
                transf[0] = true;
            }

            if (gltfNode.rotation.size() == 4) {
                auto v = gltfNode.rotation;
                rotation = Vec4d( v[0], v[1], v[2], v[3] );
                transf[1] = true;
            }

            if (gltfNode.scale.size() == 3) {
                auto v = gltfNode.scale;
                scale = Vec3d( v[0], v[1], v[2] );
                transf[2] = true;
            }

            if (gltfNode.matrix.size() == 16) {
                auto v = gltfNode.matrix;
                mat4 = Matrix4d( Vec4d(v[0], v[1], v[2], v[3]), Vec4d(v[4], v[5], v[6], v[7]), Vec4d(v[8], v[9], v[10], v[11]), Vec4d(v[12], v[13], v[14], v[15]) );
                transf[3] = true;
            }

            if (name == "") name = "Node";
            GLTFNode* thisNode = new GLTFNNode(type,name);
            nodes[nodeID] = thisNode;
            thisNode->nID = nodeID;

            if (gltfNode.children.size() > 0) {
                vector<int> children;
                for (auto each : gltfNode.children) children.push_back(each);
                childrenPerNode[nodeID] = children;
            }

            if (transf[0]) { thisNode->translation = translation; thisNode->handleTranslation(); }
            if (transf[1]) { thisNode->rotation = rotation; thisNode->handleRotation(); }
            if (transf[2]) { thisNode->scale = scale; thisNode->handleScale(); thisNode->handleTransform(); }
            if (transf[3]) { thisNode->matTransform = mat4; thisNode->pose = mat4; };
        }

        void handleMaterial(const tinygltf::Material &gltfMaterial){
            matID++;
            VRMaterialPtr mat = VRMaterial::create(gltfMaterial.name);
            //cout << "   MATE " << gltfMaterial.name << endl;
            bool bsF = false;
            bool mtF = false;
            bool rfF = false;
            bool emF = false;
            bool singleFace = false;
            for (const auto &content : gltfMaterial.values) {
                if (content.first == "baseColorTexture") {
                    int tID = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
                    if (textures.count(tID)) mat->setTexture(textures[tID]);
                }
                if (content.first == "metallicRoughnessTexture") {
                    //int tID = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                }
                if (content.first == "baseColorFactor") bsF = true;
                if (content.first == "metallicFactor") mtF = true;
                if (content.first == "roughnessFactor") rfF = true;
                if (content.first == "emissiveFactor") emF = true;
            }
            for (const auto &content : gltfMaterial.additionalValues) {
                if (content.first == "normalTexture") {
                    //int tID = gltfMaterial.normalTexture.index;
                }
                if (content.first == "emissiveTexture") {
                    //int tID = gltfMaterial.emissiveTexture.index;
                }
                if (content.first == "alphaMode") {
                    //cout << "alhphaMODE " << gltfMaterial.alphaMode << endl;
                }
                if (content.first == "doubleSided") {
                    if (!gltfMaterial.doubleSided) { singleFace = true; cout << "GLTFLOADER::WARNING IN MATERIAL " << gltfMaterial.name << " - SINGLE SIDE - ambient set to black" << endl; }
                }
            }
            Color3f baseColor;
            Color3f diff;
            if (bsF) {
                baseColor = Color3f(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],gltfMaterial.pbrMetallicRoughness.baseColorFactor[2]);
                diff = baseColor;
                mat->setDiffuse(diff);
            }
            Color3f spec;
            Color3f amb;
            double metallicFactor = 0.0;
            if (bsF && mtF) { metallicFactor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
                spec = Color3f(0.04,0.04,0.04)*(1.0-metallicFactor) + baseColor*metallicFactor;
                mat->setSpecular(spec);
                amb = baseColor * (1.0 - metallicFactor);
                if (amb[0]>0.5) amb[0] = 0.4;
                if (amb[1]>0.5) amb[1] = 0.4;
                if (amb[2]>0.5) amb[2] = 0.4;
                mat->setAmbient(amb);
            }

            if (bsF && mtF && rfF) {
                double roughnessFactor = gltfMaterial.pbrMetallicRoughness.roughnessFactor;
                if (singleFace) metallicFactor = 1.0;
                float shiny = 1.0 - roughnessFactor;
                mat->setShininess(shiny);
                //mat->ignoreMeshColors(true);
            }
            materials[matID] = mat;
        }

        void handleTexture(const tinygltf::Texture &gltfTexture){
            texID++;
            VRTexturePtr img = VRTexture::create();
            //Get texture data layout information
            const auto &image = model.images[gltfTexture.source];
            int components = image.component;
            int width = image.width;
            if (width < 0) { cout << "GLTFLOADER::WARNING IN TEXTURE loading failed " << gltfTexture.name << endl; return;}
            int height = image.height;
            int bits = image.bits;
            //cout << texID << " " << gltfTexture.source << " components " << components << " width " << width << " height " << height << " bits " << bits  << endl;

            const auto size = components * width * height * bits; //sizeof(unsigned char);
            char* data = new char[size];
            memcpy(data, image.image.data(), size);
            Vec3i dims = Vec3i(width, height, 1);

            int pf = Image::OSG_RGB_PF;
            if (components == 4) pf = Image::OSG_RGBA_PF;
            int f = Image::OSG_UINT8_IMAGEDATA;
            if (bits == 32) f = Image::OSG_FLOAT32_IMAGEDATA;
            img->getImage()->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)data, f);
            //if (components == 4) setTexture(img, true);
            //if (components == 3) setTexture(img, false);

            textures[texID] = img;
        }

        void handleMesh(const tinygltf::Mesh &gltfMesh){
            meshID++;
            GLTFNode* node;
            string name;
            name = gltfMesh.name;
            if (name == "") name = "Mesh";
            node = new GLTFNNode("Mesh",name);
            meshes[meshID] = node;
            for (auto nodeID : meshToNodes[meshID]) { references[nodeID] = node; }
            nodes[meshToNodes[meshID][0]]->addChild(node);

            long nPos = 0;
            long nUpTo = 0; //nUpToThisPrimitive
            long n = 0;
            VRGeoData gdata = VRGeoData();
            bool firstPrim = true;
            // if (gltfMesh.primitives.size() > 1) cout << "GLTFLOADER::WARNING IN MESH: multiple primitives per mesh" << endl;
            /*
            else if (gltfMesh.primitives.size() > 1) {
                string name;
                name = gltfMesh.name;
                if (name == "") name = "Mesh";
                node = new GLTFNNode("Transform",name);
                meshes[meshID] = node;
                for (auto nodeID : meshToNodes[meshID]) { references[nodeID] = node; }
                nodes[meshToNodes[meshID][0]]->addChild(node);
            }*/
            for (tinygltf::Primitive primitive : gltfMesh.primitives) {
                if (gltfMesh.primitives.size() > 1) {
                    /*
                    primID++;
                    node = new GLTFNNode("Primitive","Primitive");
                    primitives[primID] = node;
                    primToMesh[primID] = meshID;
                    childrenPerMesh[meshID].push_back(primID);*/
                }
                if (!node) return;

                if (!primitive.attributes.count("POSITION")) { cout << "GLTFLOADER::ERROR IN MESH this primitive has no pos" << endl; continue; }

                if (primitive.attributes.count("POSITION")){
                    const tinygltf::Accessor& accessorP = model.accessors[primitive.attributes["POSITION"]];
                    const tinygltf::BufferView& bufferViewP = model.bufferViews[accessorP.bufferView];
                    const tinygltf::Buffer& bufferP = model.buffers[bufferViewP.buffer];
                    const float* positions = reinterpret_cast<const float*>(&bufferP.data[bufferViewP.byteOffset + accessorP.byteOffset]);
                    if (accessorP.componentType == 5126) {
                        for (size_t i = 0; i < accessorP.count; ++i) {
                            // Positions are Vec3 components, so for each vec3 stride, offset for x, y, and z.
                            Vec3d pos;
                            if (bufferViewP.byteStride > 12) { pos = Vec3d( positions[i * (bufferViewP.byteStride/4) + 0], positions[i * (bufferViewP.byteStride/4) + 1], positions[i * (bufferViewP.byteStride/4) + 2] ); }
                            else pos = Vec3d( positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] );
                            gdata.pushVert(pos);
                            nPos++;
                        }
                    }
                }

                if (primitive.attributes.count("NORMAL")) {
                    const tinygltf::Accessor& accessorN = model.accessors[primitive.attributes["NORMAL"]];
                    const tinygltf::BufferView& bufferViewN = model.bufferViews[accessorN.bufferView];
                    const tinygltf::Buffer& bufferN = model.buffers[bufferViewN.buffer];
                    const float* normals   = reinterpret_cast<const float*>(&bufferN.data[bufferViewN.byteOffset + accessorN.byteOffset]);
                    if (accessorN.componentType == 5126) {
                        for (size_t i = 0; i < accessorN.count; ++i) {
                            Vec3d nor;
                            if (bufferViewN.byteStride > 12) { nor = Vec3d( normals[i * (bufferViewN.byteStride/4) + 0], normals[i * (bufferViewN.byteStride/4) + 1], normals[i * (bufferViewN.byteStride/4) + 2] ); }
                            else nor = Vec3d( normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] );
                            gdata.pushNorm(nor);
                        }
                    }
                }

                if (primitive.attributes.count("COLOR_0")){
                    const tinygltf::Accessor& accessorColor = model.accessors[primitive.attributes["COLOR_0"]];
                    const tinygltf::BufferView& bufferViewCO = model.bufferViews[accessorColor.bufferView];
                    const tinygltf::Buffer& bufferCO = model.buffers[bufferViewCO.buffer];
                    if (accessorColor.componentType == 5126) {
                        const float* colors   = reinterpret_cast<const float*>(&bufferCO.data[bufferViewCO.byteOffset + accessorColor.byteOffset]);
                        for (size_t i = 0; i < accessorColor.count; ++i) {
                            if (accessorColor.type == 3){ auto cl = Color3f( colors[i * 3 + 0], colors[i * 3 + 1], colors[i * 3 + 2] ); gdata.pushColor(cl); }
                            if (accessorColor.type == 4){ auto cl = Color4f( colors[i * 4 + 0], colors[i * 4 + 1], colors[i * 4 + 2], colors[i * 4 + 3] ); gdata.pushColor(cl);  }
                        }
                    }
                }

                if (primitive.attributes.count("TEXCOORD_0")) {
                    const tinygltf::Accessor& accessorTexUV = model.accessors[primitive.attributes["TEXCOORD_0"]];
                    const tinygltf::BufferView& bufferViewUV = model.bufferViews[accessorTexUV.bufferView];
                    const tinygltf::Buffer& bufferUV = model.buffers[bufferViewUV.buffer];
                    if (accessorTexUV.componentType == 5126) {
                        const float* UVs   = reinterpret_cast<const float*>(&bufferUV.data[bufferViewUV.byteOffset + accessorTexUV.byteOffset]);
                        for (size_t i = 0; i < accessorTexUV.count; ++i) {
                            Vec2d UV = Vec2d( UVs[i*2 + 0], UVs[i*2 + 1] );
                            if (firstPrim) gdata.pushTexCoord(UV);
                        }
                    }
                }

                if (primitive.attributes.count("TEXCOORD_1")) {
                    //const tinygltf::Accessor& accessorTexUV1 = model.accessors[primitive.attributes["TEXCOORD_1"]];
                }

                if (primitive.indices > -1) {
                    const tinygltf::Accessor& accessorIndices = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferViewIndices = model.bufferViews[accessorIndices.bufferView];
                    const tinygltf::Buffer& bufferInd = model.buffers[bufferViewIndices.buffer];
                    if (primitive.mode == 0) { /*POINTS*/cout << "GLTF-LOADER: not implemented POINTS" << endl; }
                    if (primitive.mode == 1) { /*LINE*/ cout << "GLTF-LOADER: not implemented LINE" << endl; }
                    if (primitive.mode == 2) { /*LINE LOOP*/ cout << "GLTF-LOADER: not implemented LINE LOOP" << endl; }
                    if (primitive.mode == 3) { /*LINE STRIP*/ cout << "GLTF-LOADER: not implemented LINE STRIP" << endl; }
                    if (primitive.mode == 4) { /*TRIANGLES*/
                        if (accessorIndices.componentType == 5121) {
                            const unsigned char* indices   = reinterpret_cast<const unsigned char*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count/3; ++i) gdata.pushTri(nUpTo+indices[i*3+0],nUpTo+indices[i*3+1],nUpTo+indices[i*3+2]);
                        }
                        if (accessorIndices.componentType == 5122) {
                            const unsigned short* indices   = reinterpret_cast<const unsigned short*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count/3; ++i) gdata.pushTri(nUpTo+indices[i*3+0],nUpTo+indices[i*3+1],nUpTo+indices[i*3+2]);
                        }
                        if (accessorIndices.componentType == 5123) {
                            const short* indices   = reinterpret_cast<const short*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count/3; ++i) gdata.pushTri(nUpTo+indices[i*3+0],nUpTo+indices[i*3+1],nUpTo+indices[i*3+2]);
                        }
                        if (accessorIndices.componentType == 5125) {
                            const unsigned int* indices   = reinterpret_cast<const unsigned int*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count/3; ++i) gdata.pushTri(nUpTo+indices[i*3+0],nUpTo+indices[i*3+1],nUpTo+indices[i*3+2]);
                        }
                        if (accessorIndices.componentType != 5121 && accessorIndices.componentType != 5122 && accessorIndices.componentType != 5125 && accessorIndices.componentType != 5123) { cout << "GLTF-LOADER: data type of INDICES unknwon: " << accessorIndices.componentType << endl; }
                    }
                    if (primitive.mode == 5) { /*TRIANGLE STRIP*/ cout << "GLTF-LOADER: not implemented TRIANGLE STRIP" << endl;}
                    if (primitive.mode == 6) { /*TRAINGLE FAN*/ cout << "GLTF-LOADER: not implemented fTRAINGLE FAN" << endl;}
                }   else {
                    if (primitive.mode == 0) { /*POINTS*/cout << "GLTF-LOADER: not implemented POINTS" << endl; }
                    if (primitive.mode == 1) { /*LINE*/ cout << "GLTF-LOADER: not implemented LINE" << endl; }
                    if (primitive.mode == 2) { /*LINE LOOP*/ cout << "GLTF-LOADER: not implemented LINE LOOP" << endl; }
                    if (primitive.mode == 3) { /*LINE STRIP*/ cout << "GLTF-LOADER: not implemented LINE STRIP" << endl; }
                    if (primitive.mode == 4) { /*TRIANGLES*/
                        for (long i = 0; i < n/3; i++) gdata.pushTri(nUpTo+i*3+0,nUpTo+i*3+1,nUpTo+i*3+2);
                    }
                    if (primitive.mode == 5) { /*TRIANGLE STRIP*/ cout << "GLTF-LOADER: not implemented TRIANGLE STRIP" << endl;}
                    if (primitive.mode == 6) { /*TRAINGLE FAN*/ cout << "GLTF-LOADER: not implemented fTRAINGLE FAN" << endl;}
                }
                //cout << meshID << " " << gdata.size() << " --- " << n <<  endl;
                //cout << "prim with v " << n << " : " << primitive.mode <<  endl;
                if (firstPrim) {
                    node->matID = primitive.material;
                    if (materials.count(primitive.material)) node->material = materials[primitive.material];
                    firstPrim = false;
                }
                nUpTo = nPos;
            }
            node->geoData = gdata;
        }

        void handleSkin(const tinygltf::Skin &gltfSkin){
            //cout << gltfSkin.name << endl;
            //for (auto joint : gltfSkin.joints) cout << joint << endl;
            //gltfSkin.inverseBindMatrices
            cout << "GLTFLOADER::WARNING IN SKINS: not supported" << endl;
            cout << " inverseBindMatrices int" << gltfSkin.inverseBindMatrices << endl;
            cout << " joints size int" << gltfSkin.joints.size() << endl;
            cout << " skeleton int" << gltfSkin.skeleton << endl;
            return;
        }

        void handleAnimation(const tinygltf::Animation &gltfAnimation){
            //cout << gltfAnimation.name << endl;
            cout << "GLTFLOADER::WARNING IN ANIMATIONS: not supported" << endl;
            cout << " channels size int" << gltfAnimation.channels.size() << endl;
            cout << " samplers size int" << gltfAnimation.samplers.size() << endl;
            return;
        }

        void connectTree() {
            for (auto eachPair : scenes) {
                auto ID = eachPair.first;
                auto& node = eachPair.second;
                for (auto childID : childrenPerScene[ID]) node->addChild(nodes[childID]);
            }
            for (auto eachPair : nodes) {
                auto ID = eachPair.first;
                auto& node = eachPair.second;
                for (auto childID : childrenPerNode[ID]) node->addChild(nodes[childID]);
            }
            for (auto eachPair : primitives) {
                auto ID = eachPair.first;
                auto& node = eachPair.second;
                for (auto childID : childrenPerMesh[ID]) node->addChild(primitives[childID]);
            }
        }

        bool parsetinygltf() {
            handleAsset(model.asset);
            for (auto each: model.scenes) handleScene(each);
            for (auto each: model.nodes) handleNode(each);
            for (auto each: model.textures) handleTexture(each);
            for (auto each: model.materials) handleMaterial(each);
            for (auto each: model.meshes) handleMesh(each);
            for (auto each: model.skins) handleSkin(each);
            for (auto each: model.animations) handleAnimation(each);
            connectTree();
            return true;
        }

    public:
        GLTFLoader(string p, VRTransformPtr r, VRProgressPtr pr, bool t) : path(p), res(r), progress(pr), threaded(t) {}

        void load() {
            ifstream file;
            if (!openFile(file)) return;
            auto fileSize = getFileSize(file);

            if (progress == 0) progress = VRProgress::create();
            progress->setup("load GLTF " + path, fileSize);
            progress->reset();

            //using namespace tinygltf;
            tinygltf::TinyGLTF loader;
            std::string err;
            std::string warn;

            string ext = GetFilePathExtension(path);
            bool ret = false;

            if ( ext.compare("glb") == 0 ) {
                cout << "try loading bin glb file at " << path << endl;
                ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str()); // for binary glTF(.glb)
            }

            if ( ext.compare("gltf") == 0 ) {
                cout << "try loading ASCII gltf file at " << path << endl;
                ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
            }

            if (!warn.empty()) {
                printf("Warn: %s\n", warn.c_str());
            }

            if (!err.empty()) {
                printf("Err: %s\n", err.c_str());
                debugDump(&model);
                cout << "ERR: not loading GLTF" << endl;
            }

            if (!ret) {
                printf("Failed to parse glTF\n");
                //return -1;
            }
            //debugDump(&model);
            if (err.empty()){
                parsetinygltf();
                version = 2;
                gltfschema = GLTFSchema(version);
                tree = new GLTFNNode("Root", "Root");
                for (auto each:scenes) tree->addChild(each.second);
                tree->obj = res;
                //tree->print();
                tree->buildOSG();
                tree->applyTransformations();
                tree->applyMaterials();
                tree->applyGeometries();
                tree->resolveLinks(references);
                //tree->print();
                progress->finish();
            }
            return;
        }

        void debugDump(tinygltf::Model *model){
            cout << "This glTF model has:\n"
                << model->accessors.size() << " accessors\n"
                << model->animations.size() << " animations\n"
                << model->buffers.size() << " buffers\n"
                << model->bufferViews.size() << " bufferViews\n"
                << model->materials.size() << " materials\n"
                << model->meshes.size() << " meshes\n"
                << model->nodes.size() << " nodes\n"
                << model->textures.size() << " textures\n"
                << model->images.size() << " images\n"
                << model->skins.size() << " skins\n"
                << model->samplers.size() << " samplers\n"
                << model->cameras.size() << " cameras\n"
                << model->scenes.size() << " scenes\n"
                << model->lights.size() << " lights\n"
                << endl;
        }
};

void OSG::loadGLTF(string path, VRTransformPtr res, VRProgressPtr p, bool thread) {
    GLTFLoader gltf(path, res, p, thread);
    gltf.load();
}

template<typename T>
T& addElement(vector<T>& v, int& ID) {
    ID = v.size();
    v.push_back(T());
    return v.back();
}

template<typename T, typename G>
int addBuffer(tinygltf::Model& model, string name, int N, function<G(int)> data) {
    if (N == 0) return -1;
    int bID;
    tinygltf::Buffer& buf = addElement(model.buffers, bID);
    vector<T> vec;
    for (int i = 0; i<N; i++) vec.push_back(T(data(i)));
    buf.name = name;
    unsigned char* d = (unsigned char*)&vec[0];
    buf.data = vector<unsigned char>( d, d + sizeof(T)*vec.size() );
    return bID;
}

int addBufferView(tinygltf::Model& model, string name, int bufID, int byteO, int byteL, int target) {
    int vID;
    tinygltf::BufferView& view = addElement(model.bufferViews, vID);
    view.name = name;
    view.buffer = bufID;
    view.byteOffset = byteO;
    view.byteLength = byteL;
    view.target = target;
    return vID;
}

int addAccessor(tinygltf::Model& model, string name, int viewID, int count, int type, int ctype) {
    int aID;
    tinygltf::Accessor& accessor = addElement(model.accessors, aID);
    accessor.name = name;
    accessor.bufferView = viewID;
    accessor.byteOffset = 0;
    accessor.count = count;
    accessor.componentType = ctype;
    accessor.type = type;
    return aID;
}

void addPrimitive(tinygltf::Model& model, tinygltf::Mesh& mesh, int length, int offset, map<string, int> bufIDs, map<string, int> accN, int mode, int cID) {
    // buffer views
    int indicesViewID   = addBufferView(model, "indicesView"  , bufIDs["indices"]  , sizeof(int)*offset, sizeof(int)*length, TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
    int positionsViewID = addBufferView(model, "positionsView", bufIDs["positions"], 0, sizeof(Vec3f)*accN["positions"], TINYGLTF_TARGET_ARRAY_BUFFER);
    int normalsViewID   = addBufferView(model, "normalsView"  , bufIDs["normals"]  , 0, sizeof(Vec3f)*accN["normals"], TINYGLTF_TARGET_ARRAY_BUFFER);

    // accessors
    int indicesAccID   = addAccessor(model, "indices"  , indicesViewID  , length, TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
    int positionsAccID = addAccessor(model, "positions", positionsViewID, accN["positions"], TINYGLTF_TYPE_VEC3  , TINYGLTF_COMPONENT_TYPE_FLOAT);
    int normalsAccID   = addAccessor(model, "normals"  , normalsViewID  , accN["normals"]  , TINYGLTF_TYPE_VEC3  , TINYGLTF_COMPONENT_TYPE_FLOAT);

    int primID;
    tinygltf::Primitive& primitive = addElement(mesh.primitives, primID);
    primitive.indices = indicesAccID;
    primitive.mode = mode;
    primitive.material = cID;
    primitive.attributes["POSITION"] = positionsAccID;
    primitive.attributes["NORMAL"] = normalsAccID;

    if (bufIDs.count("colors3")) {
        int colors3ViewID = addBufferView(model, "colors3View", bufIDs["colors3"], 0, sizeof(Vec3f)*accN["colors3"], TINYGLTF_TARGET_ARRAY_BUFFER);
        int colors3AccID = addAccessor(model, "colors3", colors3ViewID, accN["colors3"], TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
        primitive.attributes["COLOR_0"] = colors3AccID;
    }

    if (bufIDs.count("colors4")) {
        int colors4ViewID = addBufferView(model, "colors4View", bufIDs["colors4"], 0, sizeof(Vec4f)*accN["colors4"], TINYGLTF_TARGET_ARRAY_BUFFER);
        int colors4AccID = addAccessor(model, "colors4", colors4ViewID, accN["colors4"], TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT);
        primitive.attributes["COLOR_0"] = colors4AccID;
    }
}

void constructGLTF(tinygltf::Model& model, VRObjectPtr obj, int pID = -1) {
    tinygltf::Scene& scene = model.scenes.back();

    // new node
    int nID, mID, cID;
    tinygltf::Node& node = addElement(model.nodes, nID);

    // from object
    node.name = obj->getName();

    // from transform
    auto trans = dynamic_pointer_cast<VRTransform>(obj);
    if (trans) {
        Matrix4d mat = trans->getMatrix();
        double* data = mat.getValues();
        for (int i=0; i<16; i++) node.matrix.push_back(data[i]);
    }

    // scene graph structure
    if (pID < 0) scene.nodes.push_back(nID); // a root node
    else model.nodes[pID].children.push_back(nID);

    // from geometry
    auto geo = dynamic_pointer_cast<VRGeometry>(trans);
    if (geo) {
        // material
        auto mat = geo->getMaterial();
        tinygltf::Material& material = addElement(model.materials, cID);
        material.name = mat->getName();

        Color3f d = mat->getDiffuse();
        //float t = mat->getTransparency();
        material.pbrMetallicRoughness.baseColorFactor = {d[0],d[1],d[2],1};
        material.pbrMetallicRoughness.metallicFactor = 0;
        material.pbrMetallicRoughness.roughnessFactor = 1;

        // mesh
        tinygltf::Mesh& mesh = addElement(model.meshes, mID);
        mesh.name = geo->getName() + "_mesh";
        node.mesh = mID;

        VRGeoData data(geo);
        map<string, int> dataN;
        int Ntypes         = data.getDataSize(0);
        //int Nlengths       = data.getDataSize(1);
        dataN["indices"]   = data.getDataSize(2);
        dataN["positions"] = data.getDataSize(3);
        dataN["normals"]   = data.getDataSize(4);
        dataN["colors3"]   = data.getDataSize(5);
        dataN["colors4"]   = data.getDataSize(6);
        dataN["texcoords"] = data.getDataSize(7);
        dataN["texcoords2"] = data.getDataSize(8);
        dataN["indicesNormals"] = data.getDataSize(9);
        dataN["indicesColors"] = data.getDataSize(10);
        dataN["indicesTexCoords"] = data.getDataSize(11);

        if (dataN["indicesNormals"] > 0 || dataN["indicesColors"] > 0 || dataN["indicesTexCoords"] > 0) {
            cout << "constructGLTF failed due to multiindexed node!" << endl;
            // TODO: break here?
        }

        // buffer
        map<string, int> bufIDs;
        bufIDs["indices"]   = addBuffer<int  , int  >(model, "indicesBuffer"  , dataN["indices"]  , bind(&VRGeoData::getIndex   , &data, _1, PositionsIndex));
        bufIDs["positions"] = addBuffer<Vec3f, Pnt3d>(model, "positionsBuffer", dataN["positions"], bind(&VRGeoData::getPosition, &data, _1));
        bufIDs["normals"]   = addBuffer<Vec3f, Vec3d>(model, "normalsBuffer"  , dataN["normals"]  , bind(&VRGeoData::getNormal  , &data, _1));
        if (dataN["colors3"] > 0)
            bufIDs["colors3"]   = addBuffer<Vec3f, Color3f>(model, "colors3Buffer"  , dataN["colors3"]  , bind(&VRGeoData::getColor3, &data, _1));
        if (dataN["colors4"] > 0)
            bufIDs["colors4"]   = addBuffer<Vec4f, Color4f>(model, "colors4Buffer"  , dataN["colors4"]  , bind(&VRGeoData::getColor , &data, _1));
        if (dataN["colors3"] > 0 || dataN["colors4"] > 0) {
            material.pbrMetallicRoughness.baseColorFactor = {1,1,1,1};
        }

        map<int, int> typeMap;
        typeMap[GL_POINTS] = TINYGLTF_MODE_POINTS;
        typeMap[GL_LINES] = TINYGLTF_MODE_LINE;
        typeMap[GL_LINE_LOOP] = TINYGLTF_MODE_LINE_LOOP;
        typeMap[GL_LINE_STRIP] = TINYGLTF_MODE_LINE_STRIP;
        typeMap[GL_TRIANGLES] = TINYGLTF_MODE_TRIANGLES;
        typeMap[GL_TRIANGLE_STRIP] = TINYGLTF_MODE_TRIANGLE_STRIP;
        typeMap[GL_TRIANGLE_FAN] = TINYGLTF_MODE_TRIANGLE_FAN;

        // add types
        int offset = 0;
        for (int iType = 0; iType < Ntypes; iType++) {
            int type = data.getType(iType);
            int length = data.getLength(iType);

            if (typeMap.count(type)) {
                addPrimitive(model, mesh, length, offset, bufIDs, dataN, typeMap[type], cID);
                offset += length;
            } else {
                if (type == GL_QUADS) {
                    int mode = TINYGLTF_MODE_TRIANGLE_FAN;
                    for (int i = 0; i<length; i+=4) {
                        addPrimitive(model, mesh, 4, offset, bufIDs, dataN, mode, cID);
                        offset += 4;
                    }
                }
            }
        }
    }

    for (auto child : obj->getChildren()) constructGLTF(model, child, nID);
}

void OSG::writeGLTF(VRObjectPtr obj, string path) {
    tinygltf::Model model;
    model.scenes.push_back(tinygltf::Scene());
    model.defaultScene = 0;
    model.asset.version = "2.0";

    constructGLTF(model, obj);
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&model, path, true, true, true, false); // last flag is binary
}









