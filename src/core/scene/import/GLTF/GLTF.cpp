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
#include "core/objects/VRAnimation.h"
#include "core/math/path.h"

#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

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

//DEV OPTION FOR SHADER IMPLEMENTATION - uncomment for dev with pbr
//#define HANDLE_PBR_MATERIAL

#include "tiny_gltf.h"

using namespace OSG;

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
        for (unsigned int i=0; i<fields.size(); i++) {
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

    string animationInterpolationTra;
    string animationInterpolationSca;
    string animationInterpolationRot;
    vector<float> animationTimestampsTra;
    vector<float> animationTimestampsSca;
    vector<float> animationTimestampsRot;
    vector<Vec3d> animationTranslations;
    vector<Vec3d> animationScales;
    vector<Vec4d> animationRotations;
    int lastFrameTra = 0;
    int lastFrameRot = 0;
    int lastFrameSca = 0;
    float animationMaxDuration = 0.0;

    int matID = -1;

    Vec3d translation = Vec3d(0,0,0);
    Vec4d rotation = Vec4d(0,0,1,0);
    Vec3d scale = Vec3d(1,1,1);
    Matrix4d matTransform  = Matrix4d();
    Vec3d translationStart = Vec3d(0,0,0);
    Vec4d rotationStart = Vec4d(0,0,1,0);
    Vec3d scaleStart = Vec3d(1,1,1);
    Matrix4d matTransformStart  = Matrix4d();

    Vec3d stepTranslation = Vec3d(0,0,0);
    Vec4d stepRotation = Vec4d(0,0,0,0);
    Vec3d stepScale = Vec3d(1,1,1);

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
        if (g) cout << " obj name: '" << g->getName() << "'";
        cout << " ID: " << nID;
        cout << " of type '" << type << "'";
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
        Vec4d scaleOrientation = Vec4d(1,0,0,0);
        Matrix4d m1; m1.setTranslate(translation+center); m1.setRotate( Quaterniond( rotation[0], rotation[1], rotation[2], rotation[3] ) );
        Matrix4d m2; m2.setRotate( Quaterniond( scaleOrientation[0], scaleOrientation[1], scaleOrientation[2] , scaleOrientation[3] ) );
        Matrix4d m3; m3.setScale(scale);
        Matrix4d m4; m4.setTranslate(-center); m4.setRotate( Quaterniond( scaleOrientation[0], scaleOrientation[1], scaleOrientation[2] , -scaleOrientation[3] ) );
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

    virtual void applyTransformations() = 0;
    virtual void applyMaterials() = 0;
    virtual void applyGeometries() = 0;
    virtual void applyAnimations() = 0;
    virtual bool applyAnimationFrame(float maxDuration, float tIn) = 0;
    virtual void initAnimations() = 0;
    virtual float primeAnimations(float macDuration = 0.0) = 0;

    void resolveLinks(map<int, GLTFNode*>& references) {
        if (type == "Link") { if (references.count(nID)) obj->addChild(references[nID]->obj->duplicate()); /*cout << "resolved a Link" << endl;*/ }
        for (auto c : children) c->resolveLinks(references);
    }
};

struct GLTFNNode : GLTFNode{
    GLTFNNode(string type, string name = "Unnamed") : GLTFNode(type, name) { version = 2; }
    ~GLTFNNode() {}

    void applyTransformations() {
        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        if (t) t->setMatrix(pose);

        for (auto c : children) c->applyTransformations();
    }

    void applyMaterials() {
        if (isGeometryNode(type)) {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                if (material) {
                    g->setMaterial(material);
                }
            }
        }

        for (auto c : children) c->applyMaterials();
    }

    void applyGeometries() {
        if (type == "Mesh" || type == "Primitive") {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                if (geoData.size()>0) geoData.apply(g); //TODO: what if more than one primitive per mesh?
                else cout << g->getName() << " geoData with no data found" << endl;
            }
        }

        for (auto c : children) c->applyGeometries();
    }

    void applyAnimations() {
        /*auto slerp3d = [&](Vec3d v0, Vec3d v1, double t) { return v0 + (v1-v0)*t; };

        auto animTranslationLinearCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsTra.size() > 0){
                if (t*animationMaxDuration > animationTimestampsTra[0]) {
                    if (t*animationMaxDuration > animationTimestampsTra[lastFrameTra+1]) lastFrameTra++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsTra[1]) lastFrameTra = 0;
                        float lastT = animationTimestampsTra[lastFrameTra];
                        float nextT = animationTimestampsTra[lastFrameTra+1];
                        Vec3d lastVec = animationTranslations[lastFrameTra];
                        Vec3d nextVec = animationTranslations[lastFrameTra+1];
                        float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                        translation = slerp3d(lastVec,nextVec,interP);
                    }
                } else {
                    float lastT = 0.0;
                    float nextT = animationTimestampsTra[0];
                    Vec3d lastVec = translationStart;
                    Vec3d nextVec = animationTranslations[0];
                    float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                    translation = slerp3d(lastVec,nextVec,interP);
                }
                handleTranslation();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animTranslationStepCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsTra.size() > 0){
                if (t*animationMaxDuration > animationTimestampsTra[0]) {
                    if (t*animationMaxDuration > animationTimestampsTra[lastFrameTra+1]) lastFrameTra++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsTra[1]) lastFrameTra = 0;
                        Vec3d lastVec = animationTranslations[lastFrameTra];
                        translation = lastVec;
                    }
                } else translation = translationStart;
                handleTranslation();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animTranslationCubicCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsTra.size() > 0){
                if (t*animationMaxDuration > animationTimestampsTra[0]) {
                    if (t*animationMaxDuration > animationTimestampsTra[lastFrameTra+1]) lastFrameTra++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsTra[1]) lastFrameTra = 0;
                        float lastT = animationTimestampsTra[lastFrameTra];
                        float nextT = animationTimestampsTra[lastFrameTra+1];
                        float deltaT = nextT - lastT;

                        Vec3d lastPos = animationTranslations[3*lastFrameTra +1];
                        Vec3d nextPos = animationTranslations[3*(lastFrameTra+1) +1];

                        Vec3d inTan  = animationTranslations[3*lastFrameTra +0];
                        Vec3d outTan = animationTranslations[3*lastFrameTra +2]*deltaT;

                        float iP = (t*animationMaxDuration - lastT)/(deltaT);
                        float iPP = iP*iP;
                        float iPPP = iPP*iP;

                        float s2 = -2.0*iPPP + 3.0*iPP;
                        float s3 = iPPP - iPP;
                        float s0 = 1.0-s2;
                        float s1 = s3 - iPP + iP;

                        translation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                    }
                } else {
                    float lastT = 0.0;
                    float nextT = animationTimestampsTra[0];
                    float deltaT = nextT - lastT;

                    Vec3d lastPos = translationStart;
                    Vec3d nextPos = animationTranslations[1];

                    Vec3d inTan  = Vec3d(0,0,0);
                    Vec3d outTan = animationTranslations[2]*deltaT;

                    float iP = (t*animationMaxDuration - lastT)/(deltaT);
                    float iPP = iP*iP;
                    float iPPP = iPP*iP;

                    float s2 = -2.0*iPPP + 3.0*iPP;
                    float s3 = iPPP - iPP;
                    float s0 = 1.0-s2;
                    float s1 = s3 - iPP + iP;

                    translation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                }
                handleTranslation();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animScaleLinearCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsSca.size() > 0){
                if (t*animationMaxDuration > animationTimestampsSca[0]) {
                    if (t*animationMaxDuration > animationTimestampsSca[lastFrameSca+1]) lastFrameSca++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsSca[1]) lastFrameSca = 0;
                        float lastT = animationTimestampsSca[lastFrameSca];
                        float nextT = animationTimestampsSca[lastFrameSca+1];
                        Vec3d lastVec = animationScales[lastFrameSca];
                        Vec3d nextVec = animationScales[lastFrameSca+1];
                        float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                        scale = slerp3d(lastVec,nextVec,interP);
                    }
                } else {
                    float lastT = 0.0;
                    float nextT = animationTimestampsSca[0];
                    Vec3d lastVec = scaleStart;
                    Vec3d nextVec = animationScales[0];
                    float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                    scale = slerp3d(lastVec,nextVec,interP);
                }
                handleScale();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animScaleStepCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsSca.size() > 0){
                if (t*animationMaxDuration > animationTimestampsSca[0]) {
                    if (t*animationMaxDuration > animationTimestampsSca[lastFrameSca+1]) lastFrameSca++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsSca[1]) lastFrameSca = 0;
                        Vec3d lastVec = animationScales[lastFrameSca];
                        scale = lastVec;
                    }
                } else scale = scaleStart;
                handleScale();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animScaleCubicCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsSca.size() > 0){
                if (t*animationMaxDuration > animationTimestampsSca[0]) {
                    if (t*animationMaxDuration > animationTimestampsSca[lastFrameSca+1]) lastFrameSca++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsSca[1]) lastFrameSca = 0;
                        float lastT = animationTimestampsSca[lastFrameSca];
                        float nextT = animationTimestampsSca[lastFrameSca+1];
                        float deltaT = nextT - lastT;

                        Vec3d lastPos = animationScales[3*lastFrameSca +1];
                        Vec3d nextPos = animationScales[3*(lastFrameSca+1) +1];

                        Vec3d inTan  = animationScales[3*lastFrameSca +0];
                        Vec3d outTan = animationScales[3*lastFrameSca +2]*deltaT;

                        float iP = (t*animationMaxDuration - lastT)/(deltaT);
                        float iPP = iP*iP;
                        float iPPP = iPP*iP;

                        float s2 = -2.0*iPPP + 3.0*iPP;
                        float s3 = iPPP - iPP;
                        float s0 = 1.0-s2;
                        float s1 = s3 - iPP + iP;

                        scale = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                    }
                } else {
                    float lastT = 0.0;
                    float nextT = animationTimestampsSca[0];
                    float deltaT = nextT - lastT;

                    Vec3d lastPos = scaleStart;
                    Vec3d nextPos = animationScales[1];

                    Vec3d inTan  = Vec3d(0,0,0);
                    Vec3d outTan = animationScales[2]*deltaT;

                    float iP = (t*animationMaxDuration - lastT)/(deltaT);
                    float iPP = iP*iP;
                    float iPPP = iPP*iP;

                    float s2 = -2.0*iPPP + 3.0*iPP;
                    float s3 = iPPP - iPP;
                    float s0 = 1.0-s2;
                    float s1 = s3 - iPP + iP;

                    scale = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                }
                handleScale();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animRotationLinearCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            auto slerpRot = [&](Vec4d v0, Vec4d v1, double t) {
                // Only unit quaternions are valid rotations.
                // Normalize to avoid undefined behavior.
                v0.normalize();
                v1.normalize();

                // Compute the cosine of the angle between the two vectors.
                double dot = v0.dot(v1);// dot_product(v0, v1);

                // If the dot product is negative, slerp won't take
                // the shorter path. Note that v1 and -v1 are equivalent when
                // the negation is applied to all four components. Fix by
                // reversing one quaternion.
                if (dot < 0.0f) {
                    v1 = -v1;
                    dot = -dot;
                }

                const double DOT_THRESHOLD = 0.9995;
                if (dot > DOT_THRESHOLD) {
                    // If the inputs are too close for comfort, linearly interpolate
                    // and normalize the result.

                    Vec4d result = v0 + t*(v1 - v0);
                    result.normalize();
                    return result;
                }

                // Since dot is in range [0, DOT_THRESHOLD], acos is safe
                double theta_0 = acos(dot);        // theta_0 = angle between input vectors
                double theta = theta_0*t;          // theta = angle between v0 and result
                double sin_theta = sin(theta);     // compute this value only once
                double sin_theta_0 = sin(theta_0); // compute this value only once

                double s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
                double s1 = sin_theta / sin_theta_0;

                return (s0 * v0) + (s1 * v1);
            };

            if (animationTimestampsRot.size() > 0){
                if (t*animationMaxDuration > animationTimestampsRot[0]) {
                    if (t*animationMaxDuration > animationTimestampsRot[lastFrameRot+1]) lastFrameRot++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsRot[1]) lastFrameRot = 0;
                        float lastT = animationTimestampsRot[lastFrameRot];
                        float nextT = animationTimestampsRot[lastFrameRot+1];
                        Vec4d lastVec = animationRotations[lastFrameRot];
                        Vec4d nextVec = animationRotations[lastFrameRot+1];
                        float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                        rotation = slerpRot(lastVec,nextVec,interP);
                    }
                } else {
                    float lastT = 0.0;
                    float nextT = animationTimestampsRot[0];
                    Vec4d lastVec = rotationStart;
                    Vec4d nextVec = animationRotations[0];
                    float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                    rotation = slerpRot(lastVec,nextVec,interP);
                }
                handleRotation();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animRotationStepCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsRot.size() > 0){
                if (t*animationMaxDuration > animationTimestampsRot[lastFrameRot+1]) lastFrameRot++;
                if (t*animationMaxDuration > animationTimestampsRot[0]) {
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsRot[1]) lastFrameRot = 0;
                        Vec4d lastVec = animationRotations[lastFrameRot];
                        rotation = lastVec;
                    }
                } else {
                    rotation = rotationStart;
                }
                handleRotation();
                //o->setMatrix(pose);
            }
            return;
        };

        auto animRotationCubicCB = [&](OSG::VRTransformPtr o, float duration, float t) {
            if (animationTimestampsRot.size() > 0){
                if (t*animationMaxDuration > animationTimestampsRot[0]) {
                    if (t*animationMaxDuration > animationTimestampsRot[lastFrameRot+1]) lastFrameRot++;
                    if (t*animationMaxDuration >= duration) {
                        return;
                    } else {
                        if (t*animationMaxDuration < animationTimestampsRot[1]) lastFrameRot = 0;
                        float lastT = animationTimestampsRot[lastFrameRot];
                        float nextT = animationTimestampsRot[lastFrameRot+1];
                        float deltaT = nextT - lastT;

                        Vec4d lastPos = animationRotations[3*lastFrameRot +1];
                        Vec4d nextPos = animationRotations[3*(lastFrameRot+1) +1];
                        lastPos.normalize();
                        nextPos.normalize();

                        Vec4d inTan  = animationRotations[3*lastFrameRot +0];
                        Vec4d outTan = animationRotations[3*lastFrameRot +2]*deltaT;

                        float iP = (t*animationMaxDuration - lastT)/(deltaT);
                        float iPP = iP*iP;
                        float iPPP = iPP*iP;

                        float s2 = -2.0*iPPP + 3.0*iPP;
                        float s3 = iPPP - iPP;
                        float s0 = 1.0-s2;
                        float s1 = s3 - iPP + iP;

                        rotation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                    }
                } else {
                    float lastT = 0.0;
                    float nextT = animationTimestampsRot[0];
                    float deltaT = nextT - lastT;

                    Vec4d lastPos = rotationStart;
                    Vec4d nextPos = animationRotations[1];
                    lastPos.normalize();
                    nextPos.normalize();

                    Vec4d inTan  = Vec4d(0,0,0,0);
                    Vec4d outTan = animationRotations[2]*deltaT;

                    float iP = (t*animationMaxDuration - lastT)/(deltaT);
                    float iPP = iP*iP;
                    float iPPP = iPP*iP;

                    float s2 = -2.0*iPPP + 3.0*iPP;
                    float s3 = iPPP - iPP;
                    float s0 = 1.0-s2;
                    float s1 = s3 - iPP + iP;

                    rotation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                }
                handleTransform();//handleRotation();
                //o->setMatrix(pose);
            }
            return;
        };

        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        //for (auto c : children) c->applyAnimations();
        //return t;*/
    }

    bool applyAnimationFrame(float maxDuration, float tIn) {
        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        if (t) {
            auto slerp3d = [&](Vec3d v0, Vec3d v1, double t) { return v0 + (v1-v0)*t; };

            vector<bool> transf = {false, false, false, false};

            auto animTranslationLinearCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsTra.size() > 0){
                    transf[0] = true;
                    float duration = animationTimestampsTra[animationTimestampsTra.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsTra[0]) {
                        if (t*animationMaxDuration > animationTimestampsTra[lastFrameTra+1]) lastFrameTra++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsTra[1]) lastFrameTra = 0;
                            float lastT = animationTimestampsTra[lastFrameTra];
                            float nextT = animationTimestampsTra[lastFrameTra+1];
                            Vec3d lastVec = animationTranslations[lastFrameTra];
                            Vec3d nextVec = animationTranslations[lastFrameTra+1];
                            float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                            translation = slerp3d(lastVec,nextVec,interP);
                        }
                    } else {
                        float lastT = 0.0;
                        float nextT = animationTimestampsTra[0];
                        Vec3d lastVec = translationStart;
                        Vec3d nextVec = animationTranslations[0];
                        float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                        translation = slerp3d(lastVec,nextVec,interP);
                    }
                }
                return;
            };

            auto animTranslationStepCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsTra.size() > 0){
                    transf[0] = true;
                    float duration = animationTimestampsTra[animationTimestampsTra.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsTra[0]) {
                        if (t*animationMaxDuration > animationTimestampsTra[lastFrameTra+1]) lastFrameTra++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsTra[1]) lastFrameTra = 0;
                            Vec3d lastVec = animationTranslations[lastFrameTra];
                            translation = lastVec;
                        }
                    } else translation = translationStart;
                }
                return;
            };

            auto animTranslationCubicCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsTra.size() > 0){
                    transf[0] = true;
                    float duration = animationTimestampsTra[animationTimestampsTra.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsTra[0]) {
                        if (t*animationMaxDuration > animationTimestampsTra[lastFrameTra+1]) lastFrameTra++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsTra[1]) lastFrameTra = 0;
                            float lastT = animationTimestampsTra[lastFrameTra];
                            float nextT = animationTimestampsTra[lastFrameTra+1];
                            float deltaT = nextT - lastT;

                            Vec3d lastPos = animationTranslations[3*lastFrameTra +1];
                            Vec3d nextPos = animationTranslations[3*(lastFrameTra+1) +1];

                            Vec3d inTan  = animationTranslations[3*lastFrameTra +0];
                            Vec3d outTan = animationTranslations[3*lastFrameTra +2]*deltaT;

                            float iP = (t*animationMaxDuration - lastT)/(deltaT);
                            float iPP = iP*iP;
                            float iPPP = iPP*iP;

                            float s2 = -2.0*iPPP + 3.0*iPP;
                            float s3 = iPPP - iPP;
                            float s0 = 1.0-s2;
                            float s1 = s3 - iPP + iP;

                            translation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                        }
                    } else {
                        float lastT = 0.0;
                        float nextT = animationTimestampsTra[0];
                        float deltaT = nextT - lastT;

                        Vec3d lastPos = translationStart;
                        Vec3d nextPos = animationTranslations[1];

                        Vec3d inTan  = Vec3d(0,0,0);
                        Vec3d outTan = animationTranslations[2]*deltaT;

                        float iP = (t*animationMaxDuration - lastT)/(deltaT);
                        float iPP = iP*iP;
                        float iPPP = iPP*iP;

                        float s2 = -2.0*iPPP + 3.0*iPP;
                        float s3 = iPPP - iPP;
                        float s0 = 1.0-s2;
                        float s1 = s3 - iPP + iP;

                        translation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                    }
                }
                return;
            };

            auto animScaleLinearCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsSca.size() > 0){
                    transf[2] = true;
                    float duration = animationTimestampsSca[animationTimestampsSca.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsSca[0]) {
                        if (t*animationMaxDuration > animationTimestampsSca[lastFrameSca+1]) lastFrameSca++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsSca[1]) lastFrameSca = 0;
                            float lastT = animationTimestampsSca[lastFrameSca];
                            float nextT = animationTimestampsSca[lastFrameSca+1];
                            Vec3d lastVec = animationScales[lastFrameSca];
                            Vec3d nextVec = animationScales[lastFrameSca+1];
                            float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                            scale = slerp3d(lastVec,nextVec,interP);
                        }
                    } else {
                        float lastT = 0.0;
                        float nextT = animationTimestampsSca[0];
                        Vec3d lastVec = scaleStart;
                        Vec3d nextVec = animationScales[0];
                        float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                        scale = slerp3d(lastVec,nextVec,interP);
                    }
                }
                return;
            };

            auto animScaleStepCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsSca.size() > 0){
                    transf[2] = true;
                    float duration = animationTimestampsSca[animationTimestampsSca.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsSca[0]) {
                        if (t*animationMaxDuration > animationTimestampsSca[lastFrameSca+1]) lastFrameSca++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsSca[1]) lastFrameSca = 0;
                            Vec3d lastVec = animationScales[lastFrameSca];
                            scale = lastVec;
                        }
                    } else scale = scaleStart;
                }
                return;
            };

            auto animScaleCubicCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsSca.size() > 0){
                    transf[2] = true;
                    float duration = animationTimestampsSca[animationTimestampsSca.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsSca[0]) {
                        if (t*animationMaxDuration > animationTimestampsSca[lastFrameSca+1]) lastFrameSca++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsSca[1]) lastFrameSca = 0;
                            float lastT = animationTimestampsSca[lastFrameSca];
                            float nextT = animationTimestampsSca[lastFrameSca+1];
                            float deltaT = nextT - lastT;

                            Vec3d lastPos = animationScales[3*lastFrameSca +1];
                            Vec3d nextPos = animationScales[3*(lastFrameSca+1) +1];

                            Vec3d inTan  = animationScales[3*lastFrameSca +0];
                            Vec3d outTan = animationScales[3*lastFrameSca +2]*deltaT;

                            float iP = (t*animationMaxDuration - lastT)/(deltaT);
                            float iPP = iP*iP;
                            float iPPP = iPP*iP;

                            float s2 = -2.0*iPPP + 3.0*iPP;
                            float s3 = iPPP - iPP;
                            float s0 = 1.0-s2;
                            float s1 = s3 - iPP + iP;

                            scale = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                        }
                    } else {
                        float lastT = 0.0;
                        float nextT = animationTimestampsSca[0];
                        float deltaT = nextT - lastT;

                        Vec3d lastPos = scaleStart;
                        Vec3d nextPos = animationScales[1];

                        Vec3d inTan  = Vec3d(0,0,0);
                        Vec3d outTan = animationScales[2]*deltaT;

                        float iP = (t*animationMaxDuration - lastT)/(deltaT);
                        float iPP = iP*iP;
                        float iPPP = iPP*iP;

                        float s2 = -2.0*iPPP + 3.0*iPP;
                        float s3 = iPPP - iPP;
                        float s0 = 1.0-s2;
                        float s1 = s3 - iPP + iP;

                        scale = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                    }
                }
                return;
            };

            auto animRotationLinearCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                auto slerpRot = [&](Vec4d v0, Vec4d v1, double t) {
                    // Only unit quaternions are valid rotations.
                    // Normalize to avoid undefined behavior.
                    v0.normalize();
                    v1.normalize();

                    // Compute the cosine of the angle between the two vectors.
                    double dot = v0.dot(v1);// dot_product(v0, v1);

                    // If the dot product is negative, slerp won't take
                    // the shorter path. Note that v1 and -v1 are equivalent when
                    // the negation is applied to all four components. Fix by
                    // reversing one quaternion.
                    if (dot < 0.0f) {
                        v1 = -v1;
                        dot = -dot;
                    }

                    const double DOT_THRESHOLD = 0.9995;
                    if (dot > DOT_THRESHOLD) {
                        // If the inputs are too close for comfort, linearly interpolate
                        // and normalize the result.

                        Vec4d result = v0 + t*(v1 - v0);
                        result.normalize();
                        return result;
                    }

                    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
                    double theta_0 = acos(dot);        // theta_0 = angle between input vectors
                    double theta = theta_0*t;          // theta = angle between v0 and result
                    double sin_theta = sin(theta);     // compute this value only once
                    double sin_theta_0 = sin(theta_0); // compute this value only once

                    double s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
                    double s1 = sin_theta / sin_theta_0;

                    return (s0 * v0) + (s1 * v1);
                };

                if (animationTimestampsRot.size() > 0){
                    transf[1] = true;
                    float duration = animationTimestampsRot[animationTimestampsRot.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsRot[0]) {
                        if (t*animationMaxDuration > animationTimestampsRot[lastFrameRot+1]) lastFrameRot++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsRot[1]) lastFrameRot = 0;
                            float lastT = animationTimestampsRot[lastFrameRot];
                            float nextT = animationTimestampsRot[lastFrameRot+1];
                            Vec4d lastVec = animationRotations[lastFrameRot];
                            Vec4d nextVec = animationRotations[lastFrameRot+1];
                            float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                            rotation = slerpRot(lastVec,nextVec,interP);
                        }
                    } else {
                        float lastT = 0.0;
                        float nextT = animationTimestampsRot[0];
                        Vec4d lastVec = rotationStart;
                        Vec4d nextVec = animationRotations[0];
                        float interP = (t*animationMaxDuration - lastT)/(nextT-lastT);
                        rotation = slerpRot(lastVec,nextVec,interP);
                    }
                }
                return;
            };

            auto animRotationStepCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsRot.size() > 0){
                    transf[1] = true;
                    float duration = animationTimestampsRot[animationTimestampsRot.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsRot[lastFrameRot+1]) lastFrameRot++;
                    if (t*animationMaxDuration > animationTimestampsRot[0]) {
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsRot[1]) lastFrameRot = 0;
                            Vec4d lastVec = animationRotations[lastFrameRot];
                            rotation = lastVec;
                        }
                    } else {
                        rotation = rotationStart;
                    }
                }
                return;
            };

            auto animRotationCubicCB = [&](OSG::VRTransformPtr o, float animationMaxDuration, float t) {
                if (animationTimestampsRot.size() > 0){
                    transf[1] = true;
                    float duration = animationTimestampsRot[animationTimestampsRot.size() - 1];
                    if (t*animationMaxDuration > animationTimestampsRot[0]) {
                        if (t*animationMaxDuration > animationTimestampsRot[lastFrameRot+1]) lastFrameRot++;
                        if (t*animationMaxDuration >= duration) {
                            return;
                        } else {
                            if (t*animationMaxDuration < animationTimestampsRot[1]) lastFrameRot = 0;
                            float lastT = animationTimestampsRot[lastFrameRot];
                            float nextT = animationTimestampsRot[lastFrameRot+1];
                            float deltaT = nextT - lastT;

                            Vec4d lastPos = animationRotations[3*lastFrameRot +1];
                            Vec4d nextPos = animationRotations[3*(lastFrameRot+1) +1];
                            lastPos.normalize();
                            nextPos.normalize();

                            Vec4d inTan  = animationRotations[3*lastFrameRot +0];
                            Vec4d outTan = animationRotations[3*lastFrameRot +2]*deltaT;

                            float iP = (t*animationMaxDuration - lastT)/(deltaT);
                            float iPP = iP*iP;
                            float iPPP = iPP*iP;

                            float s2 = -2.0*iPPP + 3.0*iPP;
                            float s3 = iPPP - iPP;
                            float s0 = 1.0-s2;
                            float s1 = s3 - iPP + iP;

                            rotation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                        }
                    } else {
                        float lastT = 0.0;
                        float nextT = animationTimestampsRot[0];
                        float deltaT = nextT - lastT;

                        Vec4d lastPos = rotationStart;
                        Vec4d nextPos = animationRotations[1];
                        lastPos.normalize();
                        nextPos.normalize();

                        Vec4d inTan  = Vec4d(0,0,0,0);
                        Vec4d outTan = animationRotations[2]*deltaT;

                        float iP = (t*animationMaxDuration - lastT)/(deltaT);
                        float iPP = iP*iP;
                        float iPPP = iPP*iP;

                        float s2 = -2.0*iPPP + 3.0*iPP;
                        float s3 = iPPP - iPP;
                        float s0 = 1.0-s2;
                        float s1 = s3 - iPP + iP;

                        rotation = lastPos*s0 + outTan*s1 + nextPos*s2 + inTan*s3;
                    }
                }
                return;
            };

            if (animationInterpolationTra == "STEP") animTranslationStepCB( t, maxDuration, tIn );
            if (animationInterpolationTra == "CUBICSPLINE") animTranslationCubicCB( t, maxDuration, tIn );
            if (animationInterpolationTra == "LINEAR") animTranslationLinearCB( t, maxDuration, tIn );

            if (animationInterpolationRot == "STEP") animRotationStepCB( t, maxDuration, tIn );
            if (animationInterpolationRot == "CUBICSPLINE") animRotationCubicCB( t, maxDuration, tIn );
            if (animationInterpolationRot == "LINEAR") animRotationLinearCB( t, maxDuration, tIn );

            if (animationInterpolationSca == "STEP") animScaleStepCB( t, maxDuration, tIn );
            if (animationInterpolationSca == "CUBICSPLINE") animScaleCubicCB( t, maxDuration, tIn );
            if (animationInterpolationSca == "LINEAR") animScaleLinearCB( t, maxDuration, tIn );

            //handleTransform();
            //pose = Matrix4d(); handleTransform();
            //if (transf[0] || transf[1] || transf[2]) { pose = Matrix4d(); handleTransform(); }
            if (transf[0]) { handleTranslation(); }
            if (transf[1]) { handleRotation(); }
            if (transf[2]) { handleTransform(); } /*else {
                if (transf[0]) handleTranslation();
                if (transf[1]) handleRotation();
            }*/
            //if (transf[3]) { pose = mat4; }
            //t->setMatrix(pose);
        }

        for (auto c : children) c->applyAnimationFrame(maxDuration, tIn);
        return 0;
    }

    float primeAnimations(float maxDuration = 0.0){
        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        if (t) {
            translationStart = translation;
            rotationStart = rotation;
            scaleStart = scale;
            matTransformStart  = matTransform;

            if (animationTranslations.size() > 0) {
                float duration = animationTimestampsTra[animationTimestampsTra.size() - 1];
                if (duration > maxDuration) maxDuration = duration;
            }

            if (animationScales.size() > 0) {
                float duration = animationTimestampsSca[animationTimestampsSca.size() - 1];
                if (duration > maxDuration) maxDuration = duration;
            }

            if (animationRotations.size() > 0) {
                float duration = animationTimestampsRot[animationTimestampsRot.size() - 1];
                if (duration > maxDuration) maxDuration = duration;
            }
        }
        for (auto c : children) {
            float res = c->primeAnimations();
            if (res > maxDuration) maxDuration = res;
        }
        return maxDuration;
    }

    void initAnimations() {
        auto animCB = [&](float maxDuration, float t) {
            applyAnimationFrame(maxDuration, t);
            applyTransformations();
            return;
        };

        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        if (t) {
            float maxDuration = primeAnimations();
            VRAnimCbPtr fkt = VRAnimCb::create("anim", bind(animCB, maxDuration, placeholders::_1) );
            VRAnimationPtr animptr = VRScene::getCurrent()->addAnimation<float>(maxDuration, 0.f, fkt, 0.f, 1.f, true, true);
            t->addAnimation(animptr);
        }
    }
};

class GLTFLoader : public GLTFUtils {
    private:
        string path;
        VRTransformPtr res;
        VRProgressPtr progress;
        bool threaded = false;
        bool disableMaterials = false;
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
        vector<Vec3d> tangentsVec;
        vector<Vec2d> tangentsUVs;
        int texSizeX = 0;
        int texSizeY = 0;
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
        }

        void handleExtension(string extension){
            bool extensionNotSupported = true;
            if (extension == "KHR_materials_pbrSpecularGlossines") disableMaterials = true;
            if (extension == "KHR_materials_unlit") extensionNotSupported = false;
            if (extensionNotSupported) cout << "GLTFLOADER::WARNING IN EXTENSIONS USED: '" << extension << "' not supported yet" << endl;
        }

        void handleScene(const tinygltf::Scene &gltfScene){
            sceneID++;
            string name = gltfScene.name;
            if (name == "") name = ""; //empty name not prevented
            GLTFNode* thisNode = new GLTFNNode("Scene",name);
            thisNode->nID = sceneID;
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
            Vec4d rotation = Vec4d(0,0,0,1);
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

            if (name == "") name = ""; //empty name not prevented
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
            if (transf[2]) { thisNode->translation = translation; thisNode->rotation = rotation; thisNode->scale = scale; thisNode->handleTransform(); }
            if (transf[3]) { thisNode->matTransform = mat4; thisNode->pose = mat4; }
        }

        void handleMaterial(const tinygltf::Material &gltfMaterial){
            #ifdef HANDLE_PBR_MATERIAL
                handlePBRMaterial(gltfMaterial);
                return;
            #endif
            matID++;
            VRMaterialPtr mat = VRMaterial::create(gltfMaterial.name);
            if (gltfMaterial.extensions.count("KHR_materials_pbrSpecularGlossines")) {
                cout << "GLTFLOADER::WARNING IN MATERIALS: extension KHR pbrSpecGloss" << endl;
            }
            if (gltfMaterial.extensions.count("KHR_materials_unlit")) mat->setLit(false);
            mat->setPointSize(5);
            //cout << "   MATE " << gltfMaterial.name << endl;
            bool bsF = false;
            bool mtF = false;
            bool rfF = false;
            //bool emF = false;
            bool alphaBlend = false;
            //bool alphaMask = false;
            //bool alphaOpaque = false;
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
                //if (content.first == "emissiveFactor") emF = true;
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
                    if (gltfMaterial.alphaMode == "BLEND") alphaBlend = true;
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
                //if (alphaBlend) mat->setTransparency(gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]);
            }
            Color3f spec;
            Color3f amb;
            double metallicFactor = 0.0;
            if (bsF && mtF) {
                metallicFactor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
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
            if (!alphaBlend) mat->clearTransparency();
            if (!disableMaterials) {
                materials[matID] = mat;
            }
        }

        void handlePBRMaterial(const tinygltf::Material &gltfMaterial){
            matID++;
            VRMaterialPtr mat = VRMaterial::create(gltfMaterial.name);
            mat->setPointSize(5);
            //cout << "   MATE " << gltfMaterial.name << endl;
            string defines = "#version 400 compatibility\n";
            bool usePBR = false;

            bool bsT = false;
            bool mrT = false;
            bool noT = false;
            bool emT = false;

            bool bsF = false;
            bool mtF = false;
            bool rfF = false;
            bool emF = false;

            bool alMask = false;
            //bool alOpaque = false;
            //bool alBlend = false;
            //float alphaCutoff = 0.5;

            bool doS = false;

            if (gltfMaterial.values.count("baseColorTexture")) {
                defines += "#define HAS_BASE_COLOR_MAP 1\n";
                bsT = true;
            }
            if (gltfMaterial.values.count("metallicRoughnessTexture")) {
                defines += "#define HAS_METALLIC_ROUGHNESS_MAP 1\n";
                mrT = true;
            }
            if (gltfMaterial.values.count("baseColorFactor")) bsF = true;
            if (gltfMaterial.values.count("metallicFactor")) mtF = true;
            if (gltfMaterial.values.count("roughnessFactor")) rfF = true;
            if (gltfMaterial.values.count("emissiveFactor")) emF = true;

            if (gltfMaterial.additionalValues.count("normalTexture")) {
                defines += "#define HAS_NORMAL_MAP 1\n";
                noT = true;
            }
            if (gltfMaterial.additionalValues.count("emissiveTexture")) {
                defines += "#define HAS_EMISSIVE_MAP 1\n";
                emT = true;
            }
            if (gltfMaterial.additionalValues.count("alphaMode")) {
                if (gltfMaterial.alphaMode == "MASK") {
                    defines += "#define ALPHAMODE_MASK 1\n";
                    alMask = true;
                }
                if (gltfMaterial.alphaMode == "OPAQUE") {
                    defines += "#define ALPHAMODE_OPAQUE 1\n";
                    //alOpaque = true;
                }
                if (gltfMaterial.alphaMode == "BLEND") {
                    defines += "#define ALPHAMODE_BLEND 1\n";
                    //alBlend = true;
                }
            }
            if (gltfMaterial.additionalValues.count("doubleSided")) {
                if (gltfMaterial.doubleSided) doS = true;
            }

            if (bsT || bsF) {
                defines += "#define MATERIAL_METALLICROUGHNESS 1\n";
                usePBR = true;
            }
            //defines += "#define USE_PUNCTUAL 1\n";

            auto readFile = [&](string path) {
                ifstream file(path.c_str());
                string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();
                return str;
            };

            string wdir = VRSceneManager::get()->getOriginalWorkdir();
            string fpPath = wdir+"/shader/PBR/PBR.fp.glsl";
            //string fdpPath = wdir+"/shader/PBR/PBR.fdp";
            string vpPath = wdir+"/shader/PBR/PBR.vp.glsl";
            string fpShader = readFile(wdir+"/shader/PBR/PBR.fp.glsl");
            string vpShader = readFile(wdir+"/shader/PBR/PBR.vp.glsl");
            mat->setFragmentShader(defines+fpShader,fpPath, false);
            mat->setVertexShader(vpShader,vpPath);

            auto setTexture = [&](int pos, string sampler, int tID){
                if (textures.count(tID)) {
                    mat->setShaderParameter(sampler,pos);
                    mat->setTexture(textures[tID], true, pos);
                }
            };

            if (bsT) {
                setTexture(0, "u_BaseColorSampler", gltfMaterial.pbrMetallicRoughness.baseColorTexture.index);
                cout << gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord << endl;
                mat->setShaderParameter("u_BaseColorUVSet", gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord);
            }
            if (mrT) {
                setTexture(1, "u_MetallicRoughnessSampler", gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
                mat->setShaderParameter("u_MetallicRoughnessUVSet", gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
            }
            if (noT) {
                setTexture(2, "u_NormalSampler", gltfMaterial.normalTexture.index);
                cout << gltfMaterial.normalTexture.texCoord << endl;
                mat->setShaderParameter("u_NormalUVSet", gltfMaterial.normalTexture.texCoord);
            }
            if (emT) {
                setTexture(3, "u_EmissiveSampler", gltfMaterial.emissiveTexture.index);
                mat->setShaderParameter("u_EmissiveUVSet", gltfMaterial.emissiveTexture.texCoord);
            }

            Color4f baseColor = Color4f(1,1,1,1);
            float metallicFactor = 1.0;
            float roughnessFactor = 1.0;
            Color3f emissiveFactor = Color3f(0,0,0);

            if (bsF) {
                baseColor = Color4f(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],1.0);
                if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() > 3) baseColor[3] = gltfMaterial.pbrMetallicRoughness.baseColorFactor[3];
            }
            if (mtF) { metallicFactor = float(gltfMaterial.pbrMetallicRoughness.metallicFactor); }
            if (rfF) { roughnessFactor = float(gltfMaterial.pbrMetallicRoughness.roughnessFactor); }
            if (emF) emissiveFactor = Color3f(gltfMaterial.emissiveFactor[0],gltfMaterial.emissiveFactor[1],gltfMaterial.emissiveFactor[2]);
            if (usePBR) {
                mat->setShaderParameter("u_BaseColorFactor", baseColor);
                mat->setShaderParameter("u_MetallicFactor", metallicFactor);
                mat->setShaderParameter("u_RoughnessFactor", roughnessFactor);
                mat->setShaderParameter("u_EmissiveFactor", emissiveFactor);
            }
            if (alMask) mat->setShaderParameter("u_AlphaCutoff", float(gltfMaterial.alphaCutoff));
            mat->setShaderParameter("u_Exposure", float(1.0));
            if (doS) ;

            cout << "material with:\n";
            cout << "   baseColorTexture: " << bsT << "\n";
            cout << "   metallicRoughnessTexture: " << mrT << "\n";
            cout << "   normalTexture: " << noT << "\n";
            cout << "   emissiveTexture: " << emT << "\n";
            cout << "    baseColor: " << bsF << " " << baseColor << "\n";
            cout << "    roughnessFactor: " << rfF << " " << roughnessFactor << "\n";
            cout << "    metallicFactor: " << mtF << " " << metallicFactor << "\n";
            cout << "    emissiveFactor: " << emF << " " << emissiveFactor<< "\n";
            cout << endl;
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

            //const auto size = components * width * height * bits; //sizeof(unsigned char);
            //unsigned char* data = new unsigned char[size];
            //char* data = new char[size];
            //memcpy(data, image.image.data(), size);
            Vec3i dims = Vec3i(width, height, 1);

            int pf = Image::OSG_RGB_PF;
            if (components == 4) pf = Image::OSG_RGBA_PF;
            int f = Image::OSG_UINT8_IMAGEDATA;
            if (bits == 32) f = Image::OSG_FLOAT32_IMAGEDATA;
            //img->getImage()->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)data, f);
            img->getImage()->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)image.image.data(), f);
            //if (components == 4) setTexture(img, true);
            //if (components == 3) setTexture(img, false);
            if (texSizeX == 0) {
                texSizeX = width;
                texSizeY = height;
            }
            textures[texID] = img;
        }

        void handleMesh(const tinygltf::Mesh &gltfMesh){
            meshID++;
            GLTFNode* node;
            string name;
            name = gltfMesh.name;
            if (name == "") name = ""; //empty name not prevented
            node = new GLTFNNode("Mesh",name);
            meshes[meshID] = node;
            for (auto nodeIDt : meshToNodes[meshID]) { references[nodeIDt] = node; }
            nodes[meshToNodes[meshID][0]]->addChild(node);
            node->nID = meshID;

            long nPos = 0;
            long nUpTo = 0; //nUpToThisPrimitive
            long n = 0;
            VRGeoData gdata = VRGeoData();
            bool firstPrim = true;
            bool pointsOnly = false;
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

#ifdef HANDLE_PBR_MATERIAL
                if (primitive.attributes.count("TANGENT")) {
                    const tinygltf::Accessor& accessorT = model.accessors[primitive.attributes["TANGENT"]];
                    const tinygltf::BufferView& bufferViewT = model.bufferViews[accessorT.bufferView];
                    const tinygltf::Buffer& bufferT = model.buffers[bufferViewT.buffer];
                    const float* tangents   = reinterpret_cast<const float*>(&bufferT.data[bufferViewT.byteOffset + accessorT.byteOffset]);
                    if (accessorT.componentType == 5126) {
                        for (size_t i = 0; i < accessorT.count; ++i) {
                            Vec3d tan;
                            if (bufferViewT.byteStride > 12) { tan = Vec3d( tangents[i * (bufferViewT.byteStride/4) + 0], tangents[i * (bufferViewT.byteStride/4) + 1], tangents[i * (bufferViewT.byteStride/4) + 2] ); }
                            else tan = Vec3d( tangents[i * 3 + 0], tangents[i * 3 + 1], tangents[i * 3 + 2] );
                            //gdata.pushTangent(tan);
                            tangentsVec.push_back(tan);
                        }
                    }
                }
#endif // HANDLE_PBR_MATERIAL

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
                            tangentsUVs.push_back(UV);
                        }
                    }
                }

                if (primitive.attributes.count("TEXCOORD_1")) {
                    //const tinygltf::Accessor& accessorTexUV1 = model.accessors[primitive.attributes["TEXCOORD_1"]];
                }

                if (primitive.attributes.count("JOINTS_0")) {
                    const tinygltf::Accessor& accessor= model.accessors[primitive.attributes["JOINTS_0"]];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    vector<Vec4i> joints;
                    if (accessor.type == 4) {
                        if (accessor.componentType == 5123) {
                            const short* jointsData   = reinterpret_cast<const short*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t i = 0; i < accessor.count; i++) joints.push_back( Vec4i( jointsData[i*4 + 0], jointsData[i*4 + 1], jointsData[i*4 + 2], jointsData[i*4 + 3] ) );
                        }
                    }
                    cout << " vertex_joints type: " << accessor.componentType << " accessor type: " << accessor.type << " accessor count: " << accessor.count << endl;//  << " joints:  " << toString(joints) << endl;
                }

                if (primitive.attributes.count("WEIGHTS_0")) {
                    const tinygltf::Accessor& accessor = model.accessors[primitive.attributes["WEIGHTS_0"]];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    vector<Vec4d> weights;
                    if (accessor.type == 4) {
                        if (accessor.componentType == 5126) {
                            const float* weightsData   = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t i = 0; i < accessor.count; i++) weights.push_back( Vec4d( weightsData[i*4 + 0],  weightsData[i*4 + 1],  weightsData[i*4 + 2],  weightsData[i*4 + 3] ) );
                        }
                    }
                    cout << " vertex_weights type: " << accessor.componentType << " accessor type: " << accessor.type << " accessor count: " << accessor.count << endl;// << " weights: " << toString(weights) << endl;
                }

                if (primitive.indices > -1) {
                    const tinygltf::Accessor& accessorIndices = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferViewIndices = model.bufferViews[accessorIndices.bufferView];
                    const tinygltf::Buffer& bufferInd = model.buffers[bufferViewIndices.buffer];
                    if (primitive.mode == 0) { /*POINTS*/
                        pointsOnly = true;
                        if (accessorIndices.componentType == 5121) {
                            const unsigned char* indices   = reinterpret_cast<const unsigned char*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count; ++i) gdata.pushPoint(nUpTo+indices[i]);
                        }
                        if (accessorIndices.componentType == 5122) {
                            const unsigned short* indices   = reinterpret_cast<const unsigned short*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count; ++i) gdata.pushPoint(nUpTo+indices[i]);
                        }
                        if (accessorIndices.componentType == 5123) {
                            const short* indices   = reinterpret_cast<const short*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count; ++i) gdata.pushPoint(nUpTo+indices[i]);
                        }
                        if (accessorIndices.componentType == 5125) {
                            const unsigned int* indices   = reinterpret_cast<const unsigned int*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count; ++i) gdata.pushPoint(nUpTo+indices[i]);
                        }
                        if (accessorIndices.componentType != 5121 && accessorIndices.componentType != 5122 && accessorIndices.componentType != 5125 && accessorIndices.componentType != 5123) { cout << "GLTF-LOADER: data type of POINT INDICES unknwon: " << accessorIndices.componentType << endl; }
                    }
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
                        if (accessorIndices.componentType != 5121 && accessorIndices.componentType != 5122 && accessorIndices.componentType != 5125 && accessorIndices.componentType != 5123) { cout << "GLTF-LOADER: data type of TRIANGLE INDICES unknwon: " << accessorIndices.componentType << endl; }
                    }
                    if (primitive.mode == 5) { /*TRIANGLE STRIP*/
                        if (accessorIndices.componentType == 5121) {
                            const unsigned char* indices   = reinterpret_cast<const unsigned char*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count-2; ++i) {
                                if (i%2) {
                                    gdata.pushTri(nUpTo+indices[i+1],nUpTo+indices[i+0],nUpTo+indices[i+2]);
                                } else {
                                    gdata.pushTri(nUpTo+indices[i+0],nUpTo+indices[i+1],nUpTo+indices[i+2]);
                                }
                            }
                        }
                        if (accessorIndices.componentType == 5122) {
                            const unsigned short* indices   = reinterpret_cast<const unsigned short*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count-2; ++i)  {
                                if (i%2) {
                                    gdata.pushTri(nUpTo+indices[i+1],nUpTo+indices[i+0],nUpTo+indices[i+2]);
                                } else {
                                    gdata.pushTri(nUpTo+indices[i+0],nUpTo+indices[i+1],nUpTo+indices[i+2]);
                                }
                            }
                        }
                        if (accessorIndices.componentType == 5123) {
                            const short* indices   = reinterpret_cast<const short*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count-2; ++i)  {
                                if (i%2) {
                                    gdata.pushTri(nUpTo+indices[i+1],nUpTo+indices[i+0],nUpTo+indices[i+2]);
                                } else {
                                    gdata.pushTri(nUpTo+indices[i+0],nUpTo+indices[i+1],nUpTo+indices[i+2]);
                                }
                            }
                        }
                        if (accessorIndices.componentType == 5125) {
                            const unsigned int* indices   = reinterpret_cast<const unsigned int*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                            for (size_t i = 0; i < accessorIndices.count-2; ++i)  {
                                if (i%2) {
                                    gdata.pushTri(nUpTo+indices[i+1],nUpTo+indices[i+0],nUpTo+indices[i+2]);
                                } else {
                                    gdata.pushTri(nUpTo+indices[i+0],nUpTo+indices[i+1],nUpTo+indices[i+2]);
                                }
                            }
                        }
                        if (accessorIndices.componentType != 5121 && accessorIndices.componentType != 5122 && accessorIndices.componentType != 5125 && accessorIndices.componentType != 5123) { cout << "GLTF-LOADER: data type of TRIANGLE SRIP INDICES unknwon: " << accessorIndices.componentType << endl; }
                    }
                    if (primitive.mode == 6) { /*TRAINGLE FAN*/ cout << "GLTF-LOADER: not implemented fTRAINGLE FAN" << endl;}
                }   else {
                    if (primitive.mode == 0) { /*POINTS*/
                        pointsOnly = true;
                        for (long i = nUpTo; i < nPos; i++) gdata.pushPoint(i);
                    }
                    if (primitive.mode == 1) { /*LINE*/ cout << "GLTF-LOADER: not implemented LINE" << endl; }
                    if (primitive.mode == 2) { /*LINE LOOP*/ cout << "GLTF-LOADER: not implemented LINE LOOP" << endl; }
                    if (primitive.mode == 3) { /*LINE STRIP*/ cout << "GLTF-LOADER: not implemented LINE STRIP" << endl; }
                    if (primitive.mode == 4) { /*TRIANGLES*/
                        for (long i = 0; i < n/3; i++) gdata.pushTri(nUpTo+i*3+0,nUpTo+i*3+1,nUpTo+i*3+2);
                    }
                    if (primitive.mode == 5) { /*TRIANGLE STRIP*/ cout << "GLTF-LOADER: not implemented TRIANGLE STRIP" << endl; }
                    if (primitive.mode == 6) { /*TRAINGLE FAN*/ cout << "GLTF-LOADER: not implemented TRAINGLE FAN" << endl;}
                }
                //cout << meshID << " " << gdata.size() << " --- " << n <<  endl;
                //cout << "prim with v " << n << " : " << primitive.mode <<  endl;


#ifdef HANDLE_PBR_MATERIAL
                //MAKE TEXTURE FOR TANGENTS
                VRTexturePtr img = VRTexture::create();
                Vec3i dims = Vec3i(texSizeX, texSizeY, 1);
                const auto size = 3 * dims[0] * dims[1] * 8;

                int pf = Image::OSG_RGB_PF;
                int f = Image::OSG_UINT8_IMAGEDATA;
                unsigned char* data = new unsigned char[size];
                img->getImage()->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)data, f);

                for ( int i = 0; i < tangentsVec.size(); i++ ) {
                    int x = int( tangentsUVs[i][0]*float(dims[0]) );
                    int y = int( tangentsUVs[i][1]*float(dims[1]) );
                    float r = tangentsVec[i][0];
                    float g = tangentsVec[i][1];
                    float b = tangentsVec[i][2];
                    float a = 1;
                    img->setPixel(Vec3i(x,y,1), Color4f(r,g,b,a));
                    //cout << i << "::" << x << " " << y << "--" << tangentsUVs[i][0]*float(dims[0]) << endl;
                }
#endif // HANDLE_PBR_MATERIAL

                if (firstPrim) {
                    node->matID = primitive.material;
                    if (materials.count(primitive.material)) {
                        node->material = materials[primitive.material];
                        if (pointsOnly) materials[primitive.material]->setLit(false);
#ifdef HANDLE_PBR_MATERIAL
                        if (tangentsVec.size() > 0) {
                            materials[primitive.material]->setShaderParameter("u_TangentSampler",4);
                            materials[primitive.material]->setTexture(img, true, 4);
                        }
#endif // HANDLE_PBR_MATERIAL
                    }
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
            //cout << " inverseBindMatrices int " << gltfSkin.inverseBindMatrices << endl;
            cout << " joints size: " << gltfSkin.joints.size() << endl;
            cout << "  joints vec: " << toString(gltfSkin.joints) << endl;
            cout << " skeleton int " << gltfSkin.skeleton << endl;
            if (gltfSkin.inverseBindMatrices > -1) {
                vector<Matrix4d> invBMs;
                const tinygltf::Accessor& accessor= model.accessors[gltfSkin.inverseBindMatrices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                if (accessor.type == 36) {
                    if (accessor.componentType == 5126) {
                        const float* invBindData   = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                        for (size_t i = 0; i < accessor.count; i++) { invBMs.push_back( Matrix4d(
                            invBindData[i*16 +  0], invBindData[i*16 +  1], invBindData[i*16 +  2], invBindData[i*16 +  3],
                            invBindData[i*16 +  4], invBindData[i*16 +  5], invBindData[i*16 +  6], invBindData[i*16 +  7],
                            invBindData[i*16 +  8], invBindData[i*16 +  9], invBindData[i*16 + 10], invBindData[i*16 + 11],
                            invBindData[i*16 + 12], invBindData[i*16 + 13], invBindData[i*16 + 14], invBindData[i*16 + 15] )
                            );
                        }
                    }
                }
                cout << " invBM type: " << accessor.componentType << " accessor type: " << accessor.type << " accessor count: " << accessor.count << endl;// << " invBM:   " << toString(invBMs) << endl;
            }
            return;
        }

        void handleAnimation(const tinygltf::Animation &gltfAnimation){
            //cout << gltfAnimation.name << endl;
            cout << "GLTFLOADER::WARNING IN ANIMATIONS: implementation not finished yet" << endl;
            cout << " channelSizeN: " << gltfAnimation.channels.size() << endl;
            //cout << " samplerSizeN: " << gltfAnimation.samplers.size() << endl;
            vector<Vec3d> translation;
            vector<Vec4d> rotation;
            vector<Vec3d> scale;
            vector<float> weights;
            //int nodeID = -1;
            vector<bool> transf = {false, false, false};
            vector<int> tempAnimNodeIDs;
            float maxDuration = 0;

            for (tinygltf::AnimationChannel each: gltfAnimation.channels) {
                //if (nodeID >= 0 && nodeID != each.target_node) cout << "GLTFLOADER::WARNING IN ANIMATIONS: more than one node in animation channels" << endl;
                nodeID = each.target_node;
                tinygltf::AnimationSampler anim = gltfAnimation.samplers[each.sampler];
                vector<float> keyFrameTs; //vector of timestamps for each keyframe

                const tinygltf::Accessor& accessorIn = model.accessors[anim.input];
                const tinygltf::BufferView& bufferViewIn = model.bufferViews[accessorIn.bufferView];
                const tinygltf::Buffer& bufferIn = model.buffers[bufferViewIn.buffer];
                const float* dataIn = reinterpret_cast<const float*>(&bufferIn.data[bufferViewIn.byteOffset + accessorIn.byteOffset]);
                if (accessorIn.componentType == 5126) {
                    for (size_t i = 0; i < accessorIn.count; ++i) {
                        keyFrameTs.push_back(dataIn[i]);
                        //cout << dataIn[i] << endl;
                    }
                }
                if (keyFrameTs[keyFrameTs.size()-1] > maxDuration) maxDuration = keyFrameTs[keyFrameTs.size()-1];
                const tinygltf::Accessor& accessorOut = model.accessors[anim.output];
                const tinygltf::BufferView& bufferViewOut = model.bufferViews[accessorOut.bufferView];
                const tinygltf::Buffer& bufferOut = model.buffers[bufferViewOut.buffer];
                const float* dataOut = reinterpret_cast<const float*>(&bufferOut.data[bufferViewOut.byteOffset + accessorOut.byteOffset]);
                cout << " accOut-type: " << accessorOut.type << " accOut-component-type: " << accessorOut.componentType << endl;
                if (accessorOut.componentType == 5126) {
                    for (size_t i = 0; i < accessorOut.count; ++i) {
                        if (accessorOut.type == 3){
                            Vec3d vec = Vec3d( dataOut[i * 3 + 0], dataOut[i * 3 + 1], dataOut[i * 3 + 2] );
                            if (each.target_path == "translation") translation.push_back(vec);
                            if (each.target_path == "scale") scale.push_back(vec);
                            //cout << vec << endl;
                        }
                        if (accessorOut.type == 4){
                            Vec4d vec = Vec4d( dataOut[i * 4 + 0], dataOut[i * 4 + 1], dataOut[i * 4 + 2], dataOut[i * 4 + 3] );
                            if (each.target_path == "rotation") rotation.push_back(vec);
                            //cout << vec << endl;
                        }
                        if (each.target_path == "weights") {
                            weights.push_back( dataOut[i] );
                        }
                    }
                }
                cout << " sampler: " << each.sampler  << " node: " << each.target_node << "  input: " << anim.input << " inN: " << accessorIn.count << " output: " << anim.output << " outN: " << accessorOut.count << " path: " << each.target_path << " interpolation: " << anim.interpolation << endl;
                //cout << "  input: " << anim.input << " inN: " << accessorIn.count << " output: " << anim.output << " outN: " << accessorOut.count << " interpolation: " << anim.interpolation << endl;

                if ( nodes.count(each.target_node) ) {
                    auto& node = nodes[each.target_node];

                    if (each.target_path == "translation") {
                        node->animationInterpolationTra = anim.interpolation;
                        node->animationTimestampsTra = keyFrameTs;
                        node->animationTranslations = translation;
                    }
                    if (each.target_path == "rotation") {
                        node->animationInterpolationRot = anim.interpolation;
                        node->animationTimestampsRot = keyFrameTs;
                        node->animationRotations = rotation;
                    }
                    if (each.target_path == "scale") {
                        node->animationInterpolationSca = anim.interpolation;
                        node->animationTimestampsSca = keyFrameTs;
                        node->animationScales = scale;
                    }
                    if (each.target_path == "weights") {
                        cout << "GLTFLOADER::WARNING IN ANIMATIONS: target path 'weights' found but not yet implemented" << endl;
                    }
                    tempAnimNodeIDs.push_back(each.target_node);
                }
            }
            for (auto each: tempAnimNodeIDs) {
                if ( nodes.count(each) ) {
                    auto& node = nodes[each];
                    node->animationMaxDuration = maxDuration;
                }
            }
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
            for (auto each: model.extensionsUsed) handleExtension(each);
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

        void setupAnimations() {
            auto animCB = [&](float duration, float t) {
                //tree->applyAnimationFrame(t);
                cout << endl;
                return;
            };

            VRTransformPtr t = dynamic_pointer_cast<VRTransform>(tree->obj);
            if (t) {
                float duration = 5.0;
                VRAnimCbPtr fkt = VRAnimCb::create("anim", bind(animCB,duration, placeholders::_1) );
                VRAnimationPtr animptr = VRScene::getCurrent()->addAnimation<float>(duration, 0.f, fkt, 0.f, 1.f, true, true);
                t->addAnimation(animptr);
            } else cout << "no Tree found" << endl;
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
                tree->buildOSG();
                tree->applyTransformations();
                tree->applyMaterials();
                tree->applyGeometries();
                tree->resolveLinks(references);
                //tree->print();
                tree->initAnimations();
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
        bufIDs["indices"]   = addBuffer<int  , int  >(model, "indicesBuffer"  , dataN["indices"]  , bind(&VRGeoData::getIndex   , &data, placeholders::_1, PositionsIndex));
        bufIDs["positions"] = addBuffer<Vec3f, Pnt3d>(model, "positionsBuffer", dataN["positions"], bind(&VRGeoData::getPosition, &data, placeholders::_1));
        bufIDs["normals"]   = addBuffer<Vec3f, Vec3d>(model, "normalsBuffer"  , dataN["normals"]  , bind(&VRGeoData::getNormal  , &data, placeholders::_1));
        if (dataN["colors3"] > 0)
            bufIDs["colors3"]   = addBuffer<Vec3f, Color3f>(model, "colors3Buffer"  , dataN["colors3"]  , bind(&VRGeoData::getColor3, &data, placeholders::_1));
        if (dataN["colors4"] > 0)
            bufIDs["colors4"]   = addBuffer<Vec4f, Color4f>(model, "colors4Buffer"  , dataN["colors4"]  , bind(&VRGeoData::getColor , &data, placeholders::_1));
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









