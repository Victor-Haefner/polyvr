#include "GLTF.h"
#include "core/utils/VRProgress.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"

#include "core/scene/VRScene.h"

#include <string>
#include <iostream>
#include <fstream>
#include <stack>

#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGQuaternion.h>

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

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
        for (int i=0; i<fields.size(); i++) {
            nodeRefs[node].fieldRefs[fields[i]] = FieldRef();
            nodeRefs[node].fieldRefs[fields[i]].type = types[i];
            nodeRefs[node].fieldRefs[fields[i]].def  = defaults[i];
        }
    }

    GLTFSchema(int version = 1) : version(version) {
        if (version == 1) { // TODO
            addNodeRef("AsciiText", {}, {}, {});
            addNodeRef("Cone", {}, {}, {});
            addNodeRef("Cube", {}, {}, {});
            addNodeRef("Cylinder", {}, {}, {});
            addNodeRef("IndexedFaceSet", {}, {}, {});
            addNodeRef("IndexedLineSet", {}, {}, {});
            addNodeRef("PointSet", {}, {}, {});
            addNodeRef("Sphere", {}, {}, {});
            addNodeRef("Coordinate3", {}, {}, {});
            addNodeRef("FontStyle", {}, {}, {});
            addNodeRef("Info", {}, {}, {});
            addNodeRef("LOD", {}, {}, {});
            addNodeRef("Material", {}, {}, {});
            addNodeRef("MaterialBinding", {}, {}, {});
            addNodeRef("Normal", {}, {}, {});
            addNodeRef("NormalBinding", {}, {}, {});
            addNodeRef("Texture2", {}, {}, {});
            addNodeRef("Texture2Transform", {}, {}, {});
            addNodeRef("TextureCoordinate2", {}, {}, {});
            addNodeRef("ShapeHints", {}, {}, {});
            addNodeRef("MatrixTransform", {}, {}, {});
            addNodeRef("Rotation", {}, {}, {});
            addNodeRef("Scale", {}, {}, {});
            addNodeRef("Transform", {}, {}, {});
            addNodeRef("Translation", {}, {}, {});
            addNodeRef("OrthographicCamera", {}, {}, {});
            addNodeRef("PerspectiveCamera", {}, {}, {});
            addNodeRef("DirectionalLight", {}, {}, {});
            addNodeRef("PointLight", {}, {}, {});
            addNodeRef("SpotLight", {}, {}, {});
            addNodeRef("Group", {}, {}, {});
            addNodeRef("Separator", {}, {}, {});
            addNodeRef("Switch", {}, {}, {});
            addNodeRef("TransformSeparator", {}, {}, {});
            addNodeRef("WWWAnchor", {}, {}, {});
            addNodeRef("WWWInline", {}, {}, {});
        }

        if (version == 2) {
            addNodeRef("Anchor", {"children", "description", "parameter", "url", "bboxCenter", "bboxSize"}, {"MFNode", "SFString", "MFString", "MFString", "SFVec3f", "SFVec3f"}, {"[]", "", "[]", "[]", "0 0 0", "-1 -1 -1"});
            addNodeRef("Appearance", {"material", "texture", "textureTransform"}, {"SFNode", "SFNode", "SFNode"}, {"NULL", "NULL", "NULL"});
            addNodeRef("AudioClip", {"description", "loop", "pitch", "startTime", "stopTime", "url"}, {"SFString", "SFBool", "SFFloat", "SFTime", "SFTime", "MFString"}, {"", "FALSE", "1.0", "0", "0", "[]"});
            addNodeRef("Background", {"groundAngle", "groundColor", "backUrl", "bottomUrl", "frontUrl", "leftUrl", "rightUrl", "topUrl", "skyAngle", "skyColor"}, {"MFFloat", "MFColor", "MFString", "MFString", "MFString", "MFString", "MFString", "MFString", "MFFloat", "MFColor"}, {"[]", "[]", "[]", "[]", "[]", "[]", "[]", "[]", "[]", "[ 0 0 0 ]"});
            addNodeRef("Billboard", {"axisOfRotation", "children", "bboxCenter", "bboxSize"}, {"SFVec3f", "MFNode", "SFVec3f", "SFVec3f"}, {"0 1 0", "[]", "0 0 0", "-1 -1 -1"});
            addNodeRef("Box", {"size"}, {"SFVec3f"}, {"2 2 2"});
            addNodeRef("Collision", {"children", "collide", "bboxCenter", "bboxSize", "proxy"}, {"MFNode", "SFBool", "SFVec3f", "SFVec3f", "SFNode"}, {"[]", "TRUE", "0 0 0", "-1 -1 -1", "NULL"});
            addNodeRef("Color", {"color"}, {"MFColor"}, {"[]"});
            addNodeRef("ColorInterpolator", {"key", "keyValue"}, {"MFFloat", "MFColor"}, {"[]", "[]"});
            addNodeRef("Cone", {"bottomRadius", "height", "side", "bottom"}, {"SFFloat", "SFFloat", "SFBool", "SFBool"}, {"1", "2", "TRUE", "TRUE"});
            addNodeRef("Coordinate", {"point"}, {"MFVec3f"}, {"[]"});
            addNodeRef("CoordinateInterpolator", {"key", "keyValue"}, {"MFFloat", "MFVec3f"}, {"[]", "[]"});
            addNodeRef("Cylinder", {"bottom", "height", "radius", "side", "top"}, {"SFBool", "SFFloat", "SFFloat", "SFBool", "SFBool"}, {"TRUE", "2", "1", "TRUE", "TRUE"});
            addNodeRef("CylinderSensor", {"autoOffset", "diskAngle", "enabled", "maxAngle", "minAngle", "offset"}, {"SFBool", "SFFloat", "SFBool", "SFFloat", "SFFloat", "SFFloat"}, {"TRUE", "0.262", "TRUE", "-1", "0", "0"});
            addNodeRef("DirectionalLight", {"ambientIntensity", "color", "direction", "intensity", "on"}, {"SFFloat", "SFColor", "SFVec3f", "SFFloat", "SFBool"}, {"0", "1 1 1", "0 0 -1", "1", "TRUE"});
            addNodeRef("ElevationGrid", {"color", "normal", "texCoord", "height", "ccw", "colorPerVertex", "creaseAngle", "normalPerVertex", "solid", "xDimension", "xSpacing", "zDimension", "zSpacing"}, {"SFNode", "SFNode", "SFNode", "MFFloat", "SFBool", "SFBool", "SFFloat", "SFBool", "SFBool", "SFInt32", "SFFloat", "SFInt32", "SFFloat"}, {"NULL", "NULL", "NULL", "[]", "TRUE", "TRUE", "0.0", "TRUE", "TRUE", "0", "0.0", "0", "0.0"});
            addNodeRef("Extrusion", {"beginCap", "ccw", "convex", "creaseAngle", "crossSection", "endCap", "orientation", "scale", "solid", "spine"}, {"SFBool", "SFBool", "SFBool", "SFFloat", "MFVec2f", "SFBool", "MFRotation", "MFVec2f", "SFBool", "MFVec3f"}, {"TRUE", "TRUE", "TRUE", "0", "[ 1 1, 1 -1, -1 -1, -1 1, 1 1 ]", "TRUE", "0 0 1 0", "1 1", "TRUE", "[ 0 0 0, 0 1 0 ]"});
            addNodeRef("Fog", {"color", "fogType", "visibilityRange"}, {"SFColor", "SFString", "SFFloat"}, {"1 1 1", "LINEAR", "0"});
            addNodeRef("FontStyle", {"family", "horizontal", "justify", "language", "leftToRight", "size", "spacing", "style", "topToBottom"}, {"SFString", "SFBool", "MFString", "SFString", "SFBool", "SFFloat", "SFFloat", "SFString", "SFBool"}, {"SERIF", "TRUE", "BEGIN", "", "TRUE", "1.0", "1.0", "PLAIN", "TRUE"});
            addNodeRef("Group", {"children", "bboxCenter", "bboxSize"}, {"MFNode", "SFVec3f", "SFVec3f"}, {"[]", "0 0 0", "-1 -1 -1"});
            addNodeRef("ImageTexture", {"url", "repeatS", "repeatT"}, {"MFString", "SFBool", "SFBool"}, {"[]", "TRUE", "TRUE"});
            addNodeRef("IndexedFaceSet", {"color", "coord", "normal", "texCoord", "ccw", "colorIndex", "colorPerVertex", "convex", "coordIndex", "creaseAngle", "normalIndex", "normalPerVertex", "solid", "texCoordIndex"}, {"SFNode", "SFNode", "SFNode", "SFNode", "SFBool", "MFInt32", "SFBool", "SFBool", "MFInt32", "SFFloat", "MFInt32", "SFBool", "SFBool", "MFInt32"}, {"NULL", "NULL", "NULL", "NULL", "TRUE", "[]", "TRUE", "TRUE", "[]", "0", "[]", "TRUE", "TRUE", "[]"});
            addNodeRef("IndexedLineSet", {"color", "coord", "colorIndex", "colorPerVertex", "coordIndex"}, {"SFNode", "SFNode", "MFInt32", "SFBool", "MFInt32"}, {"NULL", "NULL", "[]", "TRUE", "[]"});
            addNodeRef("Inline", {"url", "bboxCenter", "bboxSize"}, {"MFString", "SFVec3f", "SFVec3f"}, {"[]", "0 0 0", "-1 -1 -1"});
            addNodeRef("LOD", {"level", "center", "range"}, {"MFNode", "SFVec3f", "MFFloat"}, {"[]", "0 0 0", "[]"});
            addNodeRef("Material", {"ambientIntensity", "diffuseColor", "emissiveColor", "shininess", "specularColor", "transparency"}, {"SFFloat", "SFColor", "SFColor", "SFFloat", "SFColor", "SFFloat"}, {"0.2", "0.8 0.8 0.8", "0 0 0", "0.2", "0 0 0", "0"});
            addNodeRef("MovieTexture", {"loop", "speed", "startTime", "stopTime", "url", "repeatS", "repeatT"}, {"SFBool", "SFFloat", "SFTime", "SFTime", "MFString", "SFBool", "SFBool"}, {"FALSE", "1", "0", "0", "[]", "TRUE", "TRUE"});
            addNodeRef("NavigationInfo", {"avatarSize", "headlight", "speed", "type", "visibilityLimit"}, {"MFFloat", "SFBool", "SFFloat", "MFString", "SFFloat"}, {"[ 0.25, 1.6, 0.75 ]", "TRUE", "1.0", "WALK", "0.0"});
            addNodeRef("Normal", {"vector"}, {"MFVec3f"}, {"[]"});
            addNodeRef("NormalInterpolator", {"key", "keyValue"}, {"MFFloat", "MFVec3f"}, {"[]", "[]"});
            addNodeRef("OrientationInterpolator", {"key", "keyValue"}, {"MFFloat", "MFRotation"}, {"[]", "[]"});
            addNodeRef("PixelTexture", {"image", "repeatS", "repeatT"}, {"SFImage", "SFBool", "SFBool"}, {"0 0 0", "TRUE", "TRUE"});
            addNodeRef("PlaneSensor", {"autoOffset", "enabled", "maxPosition", "minPosition", "offset"}, {"SFBool", "SFBool", "SFVec2f", "SFVec2f", "SFVec2f"}, {"TRUE", "TRUE", "-1 -1", "0 0", "0 0 0"});
            addNodeRef("PointLight", {"ambientIntensity", "attenuation", "color", "intensity", "location", "on", "radius"}, {"SFFloat", "SFVec3f", "SFColor", "SFFloat", "SFVec3f", "SFBool", "SFFloat"}, {"0", "1 0 0", "1 1 1", "1", "0 0 0", "TRUE", "100"});
            addNodeRef("PointSet", {"color", "coord"}, {"SFNode", "SFNode"}, {"NULL", "NULL"});
            addNodeRef("PositionInterpolator", {"key", "keyValue"}, {"MFFloat", "MFVec3f"}, {"[]", "[]"});
            addNodeRef("ProximitySensor", {"center", "size", "enabled"}, {"SFVec3f", "SFVec3f", "SFBool"}, {"0 0 0", "0 0 0", "TRUE"});
            addNodeRef("ScalarInterpolator", {"key", "keyValue"}, {"MFFloat", "MFFloat"}, {"[]", "[]"});
            addNodeRef("Script", {"url", "directOutput", "mustEvaluate"}, {"MFString", "SFBool", "SFBool"}, {"[]", "FALSE", "FALSE"});
            addNodeRef("Shape", {"appearance", "geometry"}, {"SFNode", "SFNode"}, {"NULL", "NULL"});
            addNodeRef("Sound", {"direction", "intensity", "location", "maxBack", "maxFront", "minBack", "minFront", "priority", "source", "spatialize"}, {"SFVec3f", "SFFloat", "SFVec3f", "SFFloat", "SFFloat", "SFFloat", "SFFloat", "SFFloat", "SFNode", "SFBool"}, {"0 0 1", "1", "0 0 0", "10", "10", "1", "1", "0", "NULL", "TRUE"});
            addNodeRef("Sphere", {"radius"}, {"SFFloat"}, {"1"});
            addNodeRef("SphereSensor", {"autoOffset", "enabled", "offset"}, {"SFBool", "SFBool", "SFRotation"}, {"TRUE", "TRUE", "0 1 0 0"});
            addNodeRef("SpotLight", {"ambientIntensity", "attenuation", "beamWidth", "color", "cutOffAngle", "direction", "intensity", "location", "on", "radius"}, {"SFFloat", "SFVec3f", "SFFloat", "SFColor", "SFFloat", "SFVec3f", "SFFloat", "SFVec3f", "SFBool", "SFFloat"}, {"0", "1 0 0", "1.570796", "1 1 1", "0.785398", "0 0 -1", "1", "0 0 0", "TRUE", "100"});
            addNodeRef("Switch", {"choice", "whichChoice"}, {"MFNode", "SFInt32"}, {"[]", "-1"});
            addNodeRef("Text", {"string", "fontStyle", "length", "maxExtent"}, {"MFString", "SFNode", "MFFloat", "SFFloat"}, {"[]", "NULL", "[]", "0.0"});
            addNodeRef("TextureCoordinate", {"point"}, {"MFVec2f"}, {"[]"});
            addNodeRef("TextureTransform", {"center", "rotation", "scale", "translation"}, {"SFVec2f", "SFFloat", "SFVec2f", "SFVec2f"}, {"0 0", "0", "1 1", "0 0"});
            addNodeRef("TimeSensor", {"cycleInterval", "enabled", "loop", "startTime", "stopTime"}, {"SFTime", "SFBool", "SFBool", "SFTime", "SFTime"}, {"1", "TRUE", "FALSE", "0", "0"});
            addNodeRef("TouchSensor", {"enabled"}, {"SFBool"}, {"TRUE"});
            addNodeRef("Transform", {"center", "children", "rotation", "scale", "scaleOrientation", "translation", "bboxCenter", "bboxSize"}, {"SFVec3f", "MFNode", "SFRotation", "SFVec3f", "SFRotation", "SFVec3f", "SFVec3f", "SFVec3f"}, {"0 0 0", "[]", "0 0 1 0", "1 1 1", "0 0 1 0", "0 0 0", "0 0 0", "-1 -1 -1"});
            addNodeRef("Viewpoint", {"fieldOfView", "jump", "orientation", "position", "description"}, {"SFFloat", "SFBool", "SFRotation", "SFVec3f", "SFString"}, {"0.785398", "TRUE", "0 0 1 0", "0 0 10", ""});
            addNodeRef("VisibilitySensor", {"center", "enabled", "size"}, {"SFVec3f", "SFBool", "SFVec3f"}, {"0 0 0", "TRUE", "0 0 0"});
            addNodeRef("WorldInfo", {"info", "title"}, {"MFString", "SFString"}, {"[]", ""});
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

    bool isNumber(string number) {
        if (number[0] >= '0' && number[0] <= '9') return true;
        if (number[0] == '-' || number[0] == '+' || number[0] == '.') {
            if (number[1] >= '0' && number[1] <= '9') return true;
        }
        if (number[0] == '-' || number[0] == '+') {
            if (number[1] == '.' && number[2] >= '0' && number[3] <= '9') return true;
        }
        return false;
    }

    bool isBool(string b) {
        if (b[0] != 'f' && b[0] != 't' && b[0] != 'F' && b[0] != 'T') return false;
        if (b == "f" || b == "F" || b == "t" || b == "T") return true;
        if (b == "false" || b == "False" || b == "FALSE") return true;
        if (b == "true" || b == "True" || b == "TRUE") return true;
        if (b == "false " || b == "False " || b == "FALSE ") return true;
        if (b == "true " || b == "True " || b == "TRUE ") return true;
        return false;
    }

    bool isBracket(string bracket) {
        if (bracket == "{" || bracket == "}") return true;
        if (bracket == "{}") return true;

        /*if (version == 2) {
            if (bracket == "[" || bracket == "]") return true;
            if (bracket == "[]") return true;
        }*/

        return false;
    }

    bool isGroupNode(string node) {
        if (version == 1) {
            if (node == "Group") return true;
            if (node == "Separator") return true;
            if (node == "Switch") return true;
            if (node == "TransformSeparator") return true;
        }

        if (version == 2) {
            //if (node == "children") return true;
        }

        return false;
    }

    bool isGeometryNode(string node) {
        if (node == "IndexedFaceSet") return true;
        if (node == "IndexedLineSet") return true;
        if (node == "PointSet") return true;
        if (node == "Cone") return true;
        if (node == "Sphere") return true;
        if (node == "Cylinder") return true;

        if (version == 1) {
            if (node == "Cube") return true;
            if (node == "AsciiText") return true;
        }

        if (version == 2) {
            if (node == "Box") return true;
            if (node == "ElevationGrid") return true;
            if (node == "Extrusion") return true;
            if (node == "Text") return true;
        }

        return false;
    }

    bool isPropertyNode(string node) {
        if (node == "DirectionalLight") return true;
        if (node == "PointLight") return true;
        if (node == "SpotLight") return true;
        if (node == "Normal") return true;
        if (node == "FontStyle") return true;
        if (node == "Material") return true;
        if (node == "LOD") return true;
        if (node == "Transform") return true;

        if (version == 1) {
            if (node == "Coordinate3") return true;
            if (node == "Info") return true;
            if (node == "MaterialBinding") return true;
            if (node == "NormalBinding") return true;
            if (node == "Texture2") return true;
            if (node == "Texture2Transform") return true;
            if (node == "TextureCoordinate2") return true;
            if (node == "ShapeHints") return true;
            if (node == "MatrixTransform") return true;
            if (node == "Rotation") return true;
            if (node == "Scale") return true;
            if (node == "Translation") return true;
            if (node == "OrthographicCamera") return true;
            if (node == "PerspectiveCamera") return true;
        }

        if (version == 2) {
            if (node == "Group") return true;
            if (node == "Anchor") return true;
            if (node == "Billboard") return true;
            if (node == "Collision") return true;
            if (node == "Inline") return true;
            if (node == "Switch") return true;
            if (node == "AudioClip") return true;
            if (node == "Script") return true;
            if (node == "Shape") return true;
            if (node == "Sound") return true;
            if (node == "WorldInfo") return true;
            if (node == "Color") return true;
            if (node == "Coordinate") return true;
            if (node == "TextureCoordinate") return true;
            if (node == "Appearance") return true;
            if (node == "ImageTexture") return true;
            if (node == "MovieTexture") return true;
            if (node == "PixelTexture") return true;
            if (node == "TextureTransform") return true;
        }

        return false;
    }

    bool isOtherNode(string node) {
        if (version == 1) {
            if (node == "WWWAnchor") return true;
            if (node == "WWWInline") return true;
        }

        if (version == 2) {
            if (node == "CylinderSensor") return true;
            if (node == "PlaneSensor") return true;
            if (node == "ProximitySensor") return true;
            if (node == "SphereSensor") return true;
            if (node == "TimeSensor") return true;
            if (node == "TouchSensor") return true;
            if (node == "VisibilitySensor") return true;
            if (node == "ColorInterpolator") return true;
            if (node == "CoordinateInterpolator") return true;
            if (node == "NormalInterpolator") return true;
            if (node == "OrientationInterpolator") return true;
            if (node == "PositionInterpolator") return true;
            if (node == "ScalarInterpolator") return true;
            if (node == "Background") return true;
            if (node == "Fog") return true;
            if (node == "NavigationInfo") return true;
            if (node == "Viewpoint") return true;
        }
        return false;
    }

    bool isNode(string node) {
        if (isGroupNode(node)) return true;
        if (isGeometryNode(node)) return true;
        if (isPropertyNode(node)) return true;
        if (isOtherNode(node)) return true;
        return false;
    }

    bool isTransformationNode(string node) {
        if (version == 1) {
            if (node == "MatrixTransform") return true;
            if (node == "Rotation") return true;
            if (node == "Scale") return true;
            if (node == "Transform") return true;
            if (node == "Translation") return true;
        }

        if (version == 2) {}

        return false;
    }

    bool isTransformationResetNode(string node) {
        if (version == 1) {
            if (node == "Separator") return true;
            if (node == "TransformSeparator") return true;
        }

        if (version == 2) {}

        return false;
    }

};

struct GLTFNode : GLTFUtils {
    string name;
    string type;
    GLTFNode* parent = 0;
    vector<GLTFNode*> children;
    map<string, string> params;
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
    Matrix4d pose;

    GLTFNode(string t, string n = "Unnamed") : name(n), type(t) {}
    virtual ~GLTFNode() {
        for (auto c : children) delete c;
    }

    void addChild(GLTFNode* c) {
        c->parent = this;
        children.push_back(c);
    }

    virtual GLTFNode* newChild(string t, string n) = 0;

    vector<GLTFNode*> getSiblings() {
        vector<GLTFNode*> res;
        if (!parent) return res;
        for (auto c : parent->children) {
            if (c != this) res.push_back(c);
        }
        return res;
    }

    void print(string padding = "") {
        cout << padding << "Node '" << name << "' of type " << type << " with params [";
        for (auto p : params) cout << " (" << p.first << " : " << p.second << ") ";
        cout << "]" << endl;
        for (auto c : children) c->print(padding+" ");
    }


    // build OSG ---------------------

    vector<string> parseStringField(string mf) {
        vector<string> v;
        string s;
        int Qstate = 0;
        int Estate = 0;
        for (auto c : mf) {
            if (c == '\\') { // handle escape character
                if (Estate == 0) { Estate = 1; continue; }
                if (Estate == 1) Estate = 0;
            }

            if (c == '"' && Estate == 0) { // handle quote character
                if (Qstate == 0) {
                    s = "";
                    Qstate = 1;
                    continue;
                }

                if (Qstate == 1) {
                    Qstate = 0;
                    v.push_back(s);
                    continue;
                }
            }

            s += c;
        }
        if (v.size() == 0) v.push_back(mf);
        return v;
    }

    template<typename T>
    vector<T> splitNumberMF(string mf) {
        vector<T> v;
        string s;
        for (auto c : mf) {
            if (c == '[' || c == ']') continue; // ignore brackets
            if (c == ',') { // handle next number
                v.push_back( toValue<T>(s) );
                s = "";
                continue;
            }
            s += c;
        }
        v.push_back( toValue<T>(s) );
        return v;
    }

    typedef map<string,bool> BitMask;

    BitMask parseBitMask(map<string, string>& data, string var, vector<string> bits, vector<string> def) {
        vector<string> v;
        if (data.count(var)) {
            string mf = data[var];

            string s;
            for (auto c : mf) {
                if (c == '(' || c == ')') continue; // ignore brackets
                if (c == '|') { // handle next bit
                    v.push_back( s );
                    s = "";
                    continue;
                }
                s += c;
            }
            v.push_back( s );
        } else v = def;

        BitMask m;
        for (auto b : bits) m[b] = false;
        for (auto b : v)    m[b] = true;
        return m;
    }

    bool checkBit(BitMask& bitMask, const string& bit) {
        if (bitMask.count("ALL")) if (bitMask["ALL"]) return true;
        if (bitMask.count(bit)) return bitMask[bit];
        return false;
    }

    bool getBool(string b) {
        if (b == "f" || b == "F") return false;
        if (b == "t" || b == "T") return true;
        if (b == "false " || b == "False " || b == "FALSE ") return false;
        if (b == "true " || b == "True " || b == "TRUE ") return true;
        if (b == "false" || b == "False" || b == "FALSE") return false;
        if (b == "true" || b == "True" || b == "TRUE") return true;
        return toValue<bool>(b);
    }

    bool getSFBool(map<string, string>& data, string var, bool def) {
        if (! data.count(var)) return def;
        return getBool(data[var]);
    }

    float getSFFloat(map<string, string>& data, string var, float def) {
        return data.count(var) ? toValue<float>(data[var]) : def;
    }

    Color3f getSFColor(map<string, string>& data, string var, Color3f def) {
        return data.count(var) ? toValue<Color3f>(data[var]) : def;
    }

    Vec3d getSFVec3f(map<string, string>& data, string var, Vec3d def) {
        return data.count(var) ? toValue<Vec3d>(data[var]) : def;
    }

    Vec4d getSFRotation(map<string, string>& data, string var, Vec4d def) {
        return data.count(var) ? toValue<Vec4d>(data[var]) : def;
    }

    template<class T>
    vector<T> getMultiField(map<string, string>& data, string var, vector<T> def) {
        return data.count(var) ? toValue<vector<T>>(data[var]) : def;
    }


    VRObjectPtr makeObject() {
        //cout << "make object '" << name << "' of type " << type << endl;
        if (isGroupNode(type)) return VRObject::create(name);
        if (isGeometryNode(type)) {
            if (type == "AsciiText") return VRSprite::create(name);
            return VRGeometry::create(name);
        }
        if (isPropertyNode(type)) {
            if (type == "PointLight") return VRLight::create(name);
            if (type == "DirectionalLight") return VRLight::create(name);
            if (type == "SpotLight") return VRLight::create(name);
            if (type == "PerspectiveCamera") return VRCamera::create(name, false);
        }

        if (version == 2) {
            if (type == "Transform") return VRTransform::create(name);
        }

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

    void handleAsciiText(map<string, string> data) {
        float spacing = getSFFloat(data, "spacing", 1);
        string justification = "LEFT"; // SFEnum
        vector<string> text({string()}); // MFString
        vector<float> width({0}); // MFFloat
        if (data.count("justification")) justification = data["justification"];
        if (data.count("string")) text = parseStringField( data["string"] );
        if (data.count("width")) width = splitNumberMF<float>( data["width"] );
        VRSpritePtr s = dynamic_pointer_cast<VRSprite>(obj);
        if (!s) { cout << "WARNING in GLTF handleAsciiText, cast failed" << endl; return; }
        string t;
        for (auto l : text) t += l + "\n";
        s->setText(t);
        applyPose();
        applyMaterial();
    }

    void handleCone(map<string, string> data) {
        BitMask parts = parseBitMask(data, "parts", {"SIDES", "BOTTOM", "ALL"}, {"ALL"});
        float bottomRadius = getSFFloat(data, "bottomRadius", 1);
        float height = getSFFloat(data, "height", 2);
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) { cout << "WARNING in GLTF handleCone, cast failed" << endl; return; }
        bool doBottom = checkBit(parts, "BOTTOM");
        bool doSides = checkBit(parts, "SIDES");
        geo->setPrimitive("Cone " + toString(height) + " " + toString(bottomRadius) + " 16 " + toString(doBottom) + " " + toString(doSides));
        applyPose();
        applyMaterial();
    }

    void handleCube(map<string, string> data) {
        Vec3d s;
        if (version == 1) {
            s[0] = getSFFloat(data, "width", 2);
            s[1] = getSFFloat(data, "height", 2);
            s[2] = getSFFloat(data, "depth", 2);
        }
        if (version == 2) {
            s = getSFVec3f(data, "size", Vec3d(2,2,2));
        }
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) { cout << "WARNING in GLTF handleCube, cast failed" << endl; return; }
        geo->setPrimitive("Box " + toString(s[0]) + " " + toString(s[1]) + " " + toString(s[2]) + " 1 1 1");
        applyPose();
        applyMaterial();
    }

    void handleSphere(map<string, string> data) {
        float radius = getSFFloat(data, "radius", 1);
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) { cout << "WARNING in GLTF handleSphere, cast failed" << endl; return; }
        geo->setPrimitive("Sphere " + toString(radius) + " 2");
        applyPose();
        applyMaterial();
    }

    void handleCylinder(map<string, string> data) {
        BitMask parts = parseBitMask(data, "parts", {"SIDES", "TOP", "BOTTOM", "ALL"}, {"ALL"}); // SFBitMask
        float radius = getSFFloat(data, "radius", 1);
        float height = getSFFloat(data, "height", 2);
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) { cout << "WARNING in GLTF handleCylinder, cast failed" << endl; return; }
        bool doTop = checkBit(parts, "TOP");
        bool doBottom = checkBit(parts, "BOTTOM");
        bool doSides = checkBit(parts, "SIDES");
        geo->setPrimitive("Cylinder " + toString(height) + " " + toString(radius) + " 16 " + toString(doTop) + " " + toString(doBottom) + " " + toString(doSides));
        applyPose();
        applyMaterial();
    }

    void handlePointLight(map<string, string> data) {
        bool on = getSFBool(data, "on", 1);
        float intensity = getSFFloat(data, "intensity", 1);
        Color3f color = getSFColor(data, "color", Color3f(1,1,1));
        Vec3d location = getSFVec3f(data, "location", Vec3d(0,0,1));

        VRLightPtr light = dynamic_pointer_cast<VRLight>(obj);
        if (!light) { cout << "WARNING in GLTF handlePointLight, cast failed" << endl; return; }
        light->setPointlight();
        light->setOn(on);
        light->setDiffuse(toColor4f(color*intensity, 1));
        auto beacon = light->addBeacon();
        beacon->setFrom(location);
    }

    void handleDirectionalLight(map<string, string> data) {
        bool on = getSFBool(data, "on", 1);
        float intensity = getSFFloat(data, "intensity", 1);
        Color3f color = getSFColor(data, "color", Color3f(1,1,1));
        Vec3d direction = getSFVec3f(data, "direction", Vec3d(0,0,-1));

        VRLightPtr light = dynamic_pointer_cast<VRLight>(obj);
        if (!light) { cout << "WARNING in GLTF handleDirectionalLight, cast failed" << endl; return; }
        light->setDirectionallight();
        light->setOn(on);
        light->setDiffuse(toColor4f(color*intensity, 1));
        auto beacon = light->addBeacon();
        beacon->setDir(direction);
    }

    void handleSpotLight(map<string, string> data) {
        bool on = getSFBool(data, "on", 1);
        float intensity = getSFFloat(data, "intensity", 1);
        Color3f color = getSFColor(data, "color", Color3f(1,1,1));
        Vec3d location = getSFVec3f(data, "location", Vec3d(0,0,1));
        Vec3d direction = getSFVec3f(data, "direction", Vec3d(0,0,-1));
        float dropOffRate = getSFFloat(data, "dropOffRate", 1); // TODO
        float cutOffAngle = getSFFloat(data, "cutOffAngle", 1);

        VRLightPtr light = dynamic_pointer_cast<VRLight>(obj);
        if (!light) { cout << "WARNING in GLTF handleSpotLight, cast failed" << endl; return; }
        light->setSpotlight();
        light->setOn(on);
        light->setDiffuse(toColor4f(color*intensity, 1));
        auto beacon = light->addBeacon();
        beacon->setFrom(location);
        beacon->setDir(direction);
    }

    void handlePerspectiveCamera(map<string, string> data) {
        Vec3d position = getSFVec3f(data, "position", Vec3d(0,0,1));
        Vec4d orientation = getSFRotation(data, "orientation", Vec4d(0,0,1,0));
        float focalDistance = getSFFloat(data, "focalDistance", 5);
        float heightAngle = getSFFloat(data, "heightAngle", 0.785398);

        VRCameraPtr c = dynamic_pointer_cast<VRCamera>(obj);
        if (!c) { cout << "WARNING in GLTF handlePerspectiveCamera, cast failed" << endl; return; }
        c->rotate(orientation[3], Vec3d(orientation[0], orientation[1], orientation[2]));
        c->setFrom(position);
        c->setType(VRCamera::PERSPECTIVE);
        c->setAspect(1);
        c->setFov(1.0/heightAngle);
    }

    // transformation data
    void handleTranslation(map<string, string> data) {
        pose.setTranslate( getSFVec3f(data, "translation", Vec3d(0,0,0)) );
    }

    void handleRotation(map<string, string> data) {
        Vec4d rotation = getSFRotation(data, "rotation", Vec4d(0,0,1,0));
        pose.setRotate( Quaterniond( Vec3d(rotation[0], rotation[1], rotation[2]), rotation[3] ) );
    }

    void handleScale(map<string, string> data) {
        pose.setScale( getSFVec3f(data, "scaleFactor", Vec3d(1,1,1)) );
    }

    void handleTransform(map<string, string> data) {
        Vec3d translation = getSFVec3f(data, "translation", Vec3d(0,0,0));
        Vec4d rotation = getSFRotation(data, "rotation", Vec4d(0,0,1,0));
        Vec3d scaleFactor = getSFVec3f(data, "scaleFactor", Vec3d(1,1,1));
        if (version == 2) scaleFactor = getSFVec3f(data, "scale", Vec3d(1,1,1));
        Vec4d scaleOrientation = getSFRotation(data, "scaleOrientation", Vec4d(0,0,1,0));
        Vec3d center = getSFVec3f(data, "center", Vec3d(0,0,0));
        Matrix4d m1; m1.setTranslate(translation+center); m1.setRotate( Quaterniond( Vec3d(rotation[0], rotation[1], rotation[2]), rotation[3] ) );
        Matrix4d m2; m2.setRotate( Quaterniond( Vec3d(scaleOrientation[0], scaleOrientation[1], scaleOrientation[2]), scaleOrientation[3] ) );
        Matrix4d m3; m3.setScale(scaleFactor);
        Matrix4d m4; m4.setTranslate(-center); m4.setRotate( Quaterniond( Vec3d(scaleOrientation[0], scaleOrientation[1], scaleOrientation[2]), -scaleOrientation[3] ) );
        Matrix4d M = m1; M.mult(m2); M.mult(m3); M.mult(m4);
        pose = M;
    }

    // geo data
    void handleCoordinate3(map<string, string> data) {
        positions = getMultiField<Pnt3d>(data, "point", {});
    }

    void handleNormal(map<string, string> data) {
        if (version == 1) normals = getMultiField<Vec3d>(data, "normal", {});
        if (version == 2) normals = getMultiField<Vec3d>(data, "vector", {});
    }

    void handleColor(map<string, string> data) {
        colors = getMultiField<Color3f>(data, "color", {});
    }

    void handleTextureCoordinate(map<string, string> data) {
        texCoords = getMultiField<Vec2d>(data, "point", {});
    }

    void handleIndexedFaceSet(map<string, string> data) {
        coordIndex = getMultiField<int>(data, "coordIndex", {});
        normalIndex = getMultiField<int>(data, "normalIndex", {});
        if (version == 1) {
            colorIndex = getMultiField<int>(data, "materialIndex", {});
            texCoordIndex = getMultiField<int>(data, "textureCoordIndex", {});
        }
        if (version == 2) {
            colorIndex = getMultiField<int>(data, "colorIndex", {});
            texCoordIndex = getMultiField<int>(data, "texCoordIndex", {});
        }
    }


    void handleMaterial(map<string, string> data) {
        vector<Color3f> ambientColor = getMultiField<Color3f>(data, "ambientColor", {Color3f(0.2,0.2,0.2)});
        vector<Color3f> diffuseColor = getMultiField<Color3f>(data, "diffuseColor", {Color3f(0.8,0.8,0.8)});
        vector<Color3f> specularColor = getMultiField<Color3f>(data, "specularColor", {Color3f(0,0,0)});
        vector<Color3f> emissiveColor = getMultiField<Color3f>(data, "emissiveColor", {Color3f(0,0,0)});
        vector<float> shininess = getMultiField<float>(data, "shininess", {0.2});
        vector<float> transparency = getMultiField<float>(data, "transparency", {0});
        auto m = VRMaterial::create("material");
        for (int i=0; i<diffuseColor.size(); i++) {
            if (i > 0) m->addPass();
            m->setAmbient( ambientColor[i] );
            m->setDiffuse( diffuseColor[i] );
            m->setSpecular( specularColor[i] );
        }
        material = m;
    }

    void applyProperties() {
        if (isGroupNode( type )) return; // group nodes have no properties
        if (isOtherNode( type )) return; // other nodes are ignored
        if (type == "Untyped") return;
        if (type == "Link") return;
        if (type == "Shape" || type == "Appearance") return;
        else if (type == "PointLight") handlePointLight(params);
        else if (type == "SpotLight") handleSpotLight(params);
        else if (type == "DirectionalLight") handleDirectionalLight(params);
        else if (type == "PerspectiveCamera") handlePerspectiveCamera(params);
        else if (type == "Sphere") handleSphere(params);
        else if (type == "Cube" || type == "Box") handleCube(params);
        else if (type == "Cone") handleCone(params);
        else if (type == "Cylinder") handleCylinder(params);
        else if (type == "IndexedFaceSet") handleIndexedFaceSet(params);
        else if (type == "Coordinate3" || type == "Coordinate") handleCoordinate3(params);
        else if (type == "Normal") handleNormal(params);
        else if (type == "Color") handleColor(params);
        else if (type == "TextureCoordinate") handleTextureCoordinate(params);
        else if (type == "Translation") handleTranslation(params);
        else if (type == "Rotation") handleRotation(params);
        else if (type == "Scale") handleScale(params);
        else if (type == "Transform") handleTransform(params);
        else if (type == "Material") handleMaterial(params);
        else {
            cout << "GLTF applyProperties, node " << type << " not handled!" << endl;
        }
    }

    void buildOSG() {
        if (!obj) {
            obj = makeObject();
            if (obj) {
                if (parent && parent->obj) parent->obj->addChild(obj);
                else cout << "WARNING in GLTFNode::buildOSG, cannot append object to parent!" << endl;
            }
            applyProperties();
        }

        for (auto c : children) c->buildOSG();
    }

    virtual Matrix4d applyTransformations(Matrix4d m = Matrix4d()) = 0;
    virtual VRMaterialPtr applyMaterials(VRMaterialPtr m = 0) = 0;
    virtual VRGeoData applyGeometries(VRGeoData data, map<string, GLTFNode*>& references) = 0;

    void resolveLinks(map<string, GLTFNode*>& references) {
        if (type == "Link") obj->getParent()->addChild(references[name]->obj->duplicate());
        for (auto c : children) c->resolveLinks(references);
    }
};

struct GLTF1Node : GLTFNode {
    GLTF1Node(string type, string name = "Unnamed") : GLTFNode(type, name) { version = 1; }
    ~GLTF1Node() {}

    GLTF1Node* newChild(string t, string n) {
        //cout << "GLTF1Node::newChild '" << n << "' of type " << t << endl;
        auto c = new GLTF1Node(t,n);
        addChild(c);
        return c;
    }

    Matrix4d applyTransformations(Matrix4d m = Matrix4d()) {
        if (isGeometryNode(type)) {
            VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
            if (t) t->setMatrix(m);
        } else if(isTransformationNode(type)) {
            m.mult(pose);
        } else if(isTransformationResetNode(type)) {
            m.setIdentity();
        }

        for (auto c : children) m = c->applyTransformations(m);
        return m;
    }

    VRMaterialPtr applyMaterials(VRMaterialPtr m = 0) {
        if (isGeometryNode(type)) {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) g->setMaterial(m);
        } else if(material) {
            m = material;
        } else if(type == "Separator") {
            m = 0;
        }

        for (auto c : children) m = c->applyMaterials(m);
        return m;
    }

    VRGeoData applyGeometries(VRGeoData data, map<string, GLTFNode*>& references) {
        if (type == "IndexedFaceSet") {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                vector<int> face;
                cout << "handleIndexedFaceSet " << coordIndex.size() << endl;
                for (auto i : coordIndex) {
                    if (i == -1) {
                        cout << " face " << face.size() << endl;
                        if (face.size() == 3) data.pushTri(face[0], face[1], face[2]);
                        if (face.size() == 4) data.pushQuad(face[0], face[1], face[2], face[3]);
                        face.clear();
                        continue;
                    }
                    face.push_back(i);
                }
                data.apply(g);
            }
        } else if(type == "Coordinate3") {
            for (auto p : positions) data.pushVert(Pnt3d(p));
        } else if(type == "Normal") {
            for (auto n : normals) data.pushNorm(n);
        } else if(type == "Separator") {
            data = VRGeoData();
        }

        for (auto c : children) data = c->applyGeometries(data, references);
        return data;
    }
};

struct GLTF2Node : GLTFNode {
    GLTF2Node(string type, string name = "Unnamed") : GLTFNode(type, name) { version = 2; }
    ~GLTF2Node() {}

    GLTF2Node* newChild(string t, string n) {
        //cout << "GLTF2Node::newChild '" << n << "' of type " << t << endl;
        auto c = new GLTF2Node(t,n);
        addChild(c);
        return c;
    }

    Matrix4d applyTransformations(Matrix4d m = Matrix4d()) {
        if (type == "Transform") {
            VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
            if (t) t->setMatrix(pose);
        }

        for (auto c : children) c->applyTransformations(m);
        return m;
    }

    VRMaterialPtr applyMaterials(VRMaterialPtr m = 0) {
        if (isGeometryNode(type)) {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                VRMaterialPtr mat = 0;
                for (auto sibling : getSiblings()) {
                    if (sibling->type == "Appearance") {
                        for (auto c : sibling->children) {
                            if (c->type == "Material") mat = c->material;
                        }
                    }
                }
                if (mat) g->setMaterial(mat);
            }
        }

        for (auto c : children) c->applyMaterials(m);
        return m;
    }

    VRGeoData applyGeometries(VRGeoData data, map<string, GLTFNode*>& references) {
        if (type == "IndexedFaceSet") {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                VRGeoData geo;

                bool colorPerVertex = getBool(params["colorPerVertex"]);
                bool normalPerVertex = getBool(params["normalPerVertex"]);
                bool doNormals = false;
                bool doColors = false;
                bool doTexCoords = false;

                for (auto c : children) {
                    if (c->type == "Link") { if (references.count(c->name)) c = references[c->name]; }
                    if (c->type == "Coordinate") for (auto p : c->positions) geo.pushVert(p);
                    if (c->type == "Normal") { for (auto n : c->normals) geo.pushNorm(n); doNormals = true; }
                    if (c->type == "Color") { for (auto col : c->colors) geo.pushColor(col); doColors = true; }
                    if (c->type == "TextureCoordinate") { for (auto t : c->texCoords) geo.pushTexCoord(t); doTexCoords = true; }
                }


                vector<int> face;
                for (auto i : coordIndex) {
                    if (i == -1) {
                        if (face.size() == 3) geo.pushTri(face[0], face[1], face[2]);
                        if (face.size() == 4) geo.pushQuad(face[0], face[1], face[2], face[3]);
                        face.clear();
                        continue;
                    }
                    face.push_back(i);
                }
                // last face may not be followed by -1, so we have to check again!
                if (face.size() == 3) geo.pushTri(face[0], face[1], face[2]);
                if (face.size() == 4) geo.pushQuad(face[0], face[1], face[2], face[3]);

                if (doNormals) { // TODO, there may be negative values??
                    if (!normalPerVertex) {
                        if (normalIndex.size()) { // different normal indices, one index per face
                            for (int i = 0; i<normalIndex.size(); i++) {
                                int ID = normalIndex[i];
                                int N = geo.getFaceSize(i);
                                for (int j=0; j<N; j++) if (ID >= 0) geo.pushNormalIndex(ID);
                            }
                        } else {
                            for (int i = 0; i<geo.getNFaces(); i++) {
                                int N = geo.getFaceSize(i);
                                for (int j=0; j<N; j++) if (i >= 0) geo.pushNormalIndex(i);
                            }
                        }
                    } else {
                        if (normalIndex.size()) { // different normal indices, same primitives!
                            for (int i : normalIndex) if (i >= 0) geo.pushNormalIndex(i);
                        } else {} // nothing to do
                    }
                }

                if (doColors) {
                    if (!colorPerVertex) {
                        if (colorIndex.size()) { // different color indices, one index per face
                            for (int i = 0; i<colorIndex.size(); i++) {
                                int ID = colorIndex[i];
                                int N = geo.getFaceSize(i);
                                for (int j=0; j<N; j++) geo.pushColorIndex(ID);
                            }
                        } else {
                            for (int i = 0; i<geo.getNFaces(); i++) {
                                int N = geo.getFaceSize(i);
                                for (int j=0; j<N; j++) geo.pushColorIndex(i);
                            }
                        }
                    } else {
                        if (colorIndex.size()) { // different color indices, same primitives!
                            for (int i : colorIndex) if (i != -1) geo.pushColorIndex(i);
                        } else {} // nothing to do
                    }
                }

                geo.apply(g);
                if (!doNormals) g->updateNormals(false); // TODO: use creaseAngle
            }
        }

        for (auto c : children) c->applyGeometries(data, references);
        return data;
    }
};

class GLTFLoader : public GLTFUtils {
    private:
        string path;
        VRTransformPtr res;
        VRProgressPtr progress;
        bool threaded = false;
        GLTFNode* tree = 0;
        map<string, GLTFNode*> references;

        enum STATE {
            NODE,
            STRING,
            FIELD
        };

        struct Context {
            STATE state = NODE;
            string field;
            GLTFNode* currentNode = 0;

            // node header
            bool nextNodeDEF = false;
            bool nextNodeUSE = false;
            string nextNodeType = "Untyped";
            string nextNodeName = "Unnamed";
        };

        static string GetFilePathExtension(const string &FileName) {
            if (FileName.find_last_of(".") != std::string::npos)
                return FileName.substr(FileName.find_last_of(".") + 1);
            return "";
        }

        Context ctx;

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

        Matrix4d handleMatrixTransform(map<string, string> data) {
            if (!data.count("matrix")) return Matrix4d();
            vector<float> v = toValue<vector<float>>( data["matrix"] );
            return Matrix4d(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
        }

        void handleBracket(string bracket) {
            auto open = [&]() {
                if (ctx.currentNode) {
                    ctx.currentNode = ctx.currentNode->newChild(ctx.nextNodeType, ctx.nextNodeName);
                    if (ctx.nextNodeDEF) references[ctx.nextNodeName] = ctx.currentNode;
                    ctx.nextNodeType = "Untyped";
                    ctx.nextNodeName = "Unnamed";
                    ctx.nextNodeDEF = false;
                } else cout << "WARNING in GLTF handleBracket: currentNode at opening bracket is NULL" << endl;
            };

            auto close = [&]() {
                //applyProperties();
                if (ctx.currentNode->parent) {
                    ctx.currentNode = ctx.currentNode->parent;
                } else cout << "WARNING in GLTF handleBracket: currentNode at closing bracket is NULL" << endl;
            };

            ctx.state = NODE;

            if (ctx.currentNode) {
                if (bracket == "}") close();
                if (bracket == "{") open();
                if (bracket == "{}") {
                    open();
                    close();
                }

                /*if (version == 2) { // for children node
                    if ()
                    if (bracket == "]") close();
                    if (bracket == "[") open();
                }*/
            }
        };

        string stateToString(STATE s) {
            if (s == NODE) return "NODE";
            if (s == STRING) return "STRING";
            if (s == FIELD) return "FIELD";
            return "UNNOKWN";
        }

        void handleToken(string token) {
            //cout << " handle GLTF token: " << token << " " << stateToString(ctx.state) << endl;
            if (isBracket(token)) { handleBracket(token); return; }
            if (isNode(token)) {
                //cout << "  handle GLTF node: " << token << ", set as nextNodeType!" << endl;
                ctx.nextNodeType = token;
                return;
            }

            //cout << "  GLTF context state: " << stateToString(ctx.state) << endl;

            if (ctx.state == FIELD) {
                //cout << "   " << token << " in field " << isNumber(token) << " '" << ctx.currentNode->params[ctx.field] << "' field: " << ctx.field << endl;
                if (token == "[" || token == "]" || token == ",") return;
                if (isNumber(token) || isBool(token)) { ctx.currentNode->params[ctx.field] += token+" "; return; }
                ctx.state = NODE; // don't return here
            }

            if (ctx.state == NODE) {
                if (token == "DEF") { ctx.nextNodeDEF = true; return; }
                if (token == "USE") { ctx.nextNodeUSE = true; return; }
                if (ctx.nextNodeDEF) { ctx.nextNodeName = token; return; }
                if (ctx.nextNodeUSE) {
                    if (!references.count(token)) cout << "WARNING in GLTF handle token, no reference named " << token << " found!" << endl;
                    auto refNode = references[token];
                    ctx.currentNode->newChild("Link", token);
                    ctx.nextNodeUSE = false;
                    return;
                }

                if (gltfschema.isFieldOf(ctx.currentNode->type, token)) {
                    string fType = gltfschema.getField(ctx.currentNode->type, token).type;
                    if (fType == "SFNode" || fType == "MFNode") {
                        ctx.nextNodeName = token;
                        ctx.nextNodeType = "Parameter";
                        return;
                    }

                    ctx.currentNode->params[token] = "";
                    ctx.field = token;
                    ctx.state = FIELD;
                    return;
                }
            }

            if (token == "[" || token == "]") return;

            cout << "WARNING: GLTF token not handled: '" << token << "'" << endl;
        };

        bool parseFile(ifstream& file) {
            // coordinate system: OpenGL
            string line;
            while ( getline(file, line) ) {
                progress->update( line.size() );
                if (line.size() == 0) continue; // ignore empty lines
                if (line[0] == '#') continue; // ignore comments
                stringstream ss(line);
                while(!ss.eof()) {
                    string s; ss >> s;
                    if (s.size() == 0) continue; // ignore empty tokens
                    if (s[0] == '#') break; // ignore comments

                    int n = s.size()-1;
                    if (n > 0) {
                        if (s[0] == '{' || s[0] == '}' || s[0] == '[' || s[0] == ']') {
                            string s1 = subString(s, 0, 1);
                            string s2 = subString(s, 1, n);
                            handleToken(s1);
                            handleToken(s2);
                            continue;
                        }

                        if (s[n] == '{' || s[n] == '}' || s[n] == '[' || s[n] == ']') {
                            string s1 = subString(s, 0, n);
                            string s2 = subString(s, n, 1);
                            handleToken(s1);
                            handleToken(s2);
                            continue;
                        }
                    }

                    handleToken(s);
                }
            }
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
            tinygltf::Model model;
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
            }

            if (!ret) {
              printf("Failed to parse glTF\n");
              //return -1;
            }
            debugDump(&model);
            //debugModel(&model);
            progress->finish();
            return;

            gltfschema = GLTFSchema(version);
            ctx = Context();
            if (version == 1) tree = new GLTF1Node("Root", "Root");
            if (version == 2) tree = new GLTF2Node("Root", "Root");
            tree->obj = res;
            ctx.currentNode = tree;
            parseFile(file);
            //tree->print();
            tree->buildOSG();
            tree->applyTransformations();
            tree->applyMaterials();
            tree->applyGeometries(VRGeoData(), references);
            tree->resolveLinks(references);

            progress->finish();
            delete tree;
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

        void debugModel(tinygltf::Model *model){
            cout << "trying to read from model" << endl;
            auto allMeshes = model->meshes;
            cout << "Meshes " << allMeshes.size() << endl;
            long n = 0;

            VRGeoData gdata = VRGeoData();
            VRGeometryPtr VRgeo = VRGeometry::create("test");
            auto mat = VRgeo->getMaterial();
            auto scene = VRScene::getCurrent();
            scene->getRoot()->addChild(VRgeo);
            VRgeo->setPersistency(0);
            vector<Vec3d> positionVecs;
            vector<Vec3d> normalVecs;
            //Load texture data
            vector<VRTexturePtr> texes;
            bool first = true;
            for (const auto &gltfTexture : model->textures) {

                VRTexturePtr tex;

                //Get texture data layout information
                const auto &image = model->images[gltfTexture.source];
                int components = image.component;
                int width = image.width;
                int height = image.height;
                int bits = image.bits;
                cout << " components " << components << " width " << width << " height " << height << " bits " << bits  << endl;

                //Check if texture is used as part of material, if so, tag the texture with the usage enum
                /*const auto usageIt = textureUsedFor.find(gltfTexture.source);
                if (usageIt != textureUsedFor.end()) {
                  loadedTexture.use = (*usageIt).second;
                }*/

                //Load bytes into texture array

               //const auto size = components * width * height * sizeof(unsigned char);
                //char* data = new unsigned char[size];
                //memcpy(tex->img, image.image.data(), size);*/
                if (first) {
                    const auto size = components * width * height * sizeof(unsigned char);
                    char* data = new char[size];
                    memcpy(data, image.image.data(), size);
                    Vec3i dims = Vec3i(width, height, 1);
                    mat->setTexture(data, components, dims, bits == 32);
                    first = false;
                }

                //Add texture to usable textures
                //texes.push_back(tex);
            }
            for (auto mesh : allMeshes) {
                auto allPrims = mesh.primitives;
                cout << "Primitives " << allPrims.size() << endl;
                for (auto primitive : allPrims) {
                    string atts = "";
                    const tinygltf::Accessor& accessorP = model->accessors[primitive.attributes["POSITION"]];
                    const tinygltf::Accessor& accessorN = model->accessors[primitive.attributes["NORMAL"]];
                    const tinygltf::Accessor& accessorColor = model->accessors[primitive.attributes["COLOR_0"]];
                    const tinygltf::Accessor& accessorTexUV = model->accessors[primitive.attributes["TEXCOORD_0"]];
                    for (auto att : primitive.attributes) atts += att.first + " ";
                    cout << atts << endl;
                    const tinygltf::BufferView& bufferViewP = model->bufferViews[accessorP.bufferView];
                    const tinygltf::BufferView& bufferViewN = model->bufferViews[accessorN.bufferView];
                    const tinygltf::BufferView& bufferViewCO = model->bufferViews[accessorColor.bufferView];
                    const tinygltf::BufferView& bufferViewUV = model->bufferViews[accessorTexUV.bufferView];

                    // cast to float type read only. Use accessor and bufview byte offsets to determine where position data
                    // is located in the buffer.
                    const tinygltf::Buffer& bufferP = model->buffers[bufferViewP.buffer];
                    const tinygltf::Buffer& bufferN = model->buffers[bufferViewN.buffer];
                    const tinygltf::Buffer& bufferCO = model->buffers[bufferViewCO.buffer];
                    const tinygltf::Buffer& bufferUV = model->buffers[bufferViewUV.buffer];
                    // bufferView byteoffset + accessor byteoffset tells you where the actual position data is within the buffer. From there
                    // you should already know how the data needs to be interpreted.
                    const float* positions = reinterpret_cast<const float*>(&bufferP.data[bufferViewP.byteOffset + accessorP.byteOffset]);
                    const float* normals   = reinterpret_cast<const float*>(&bufferN.data[bufferViewN.byteOffset + accessorN.byteOffset]);
                    const float* colors   = reinterpret_cast<const float*>(&bufferCO.data[bufferViewCO.byteOffset + accessorColor.byteOffset]);
                    const float* UVs   = reinterpret_cast<const float*>(&bufferUV.data[bufferViewUV.byteOffset + accessorTexUV.byteOffset]);
                    // From here, you choose what you wish to do with this position data. In this case, we  will display it out.

                    cout << "ColorBufferLength " << accessorColor.count << endl;
                    cout << "ColorBuffer Type  " << accessorColor.type << endl;
                    cout << "TexUVBufferLength " << accessorTexUV.count << endl;
                    for (size_t i = 0; i < accessorP.count; ++i) {
                              // Positions are Vec3 components, so for each vec3 stride, offset for x, y, and z.
                              Vec3d pos = Vec3d( positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] );
                              Vec3d nor = Vec3d( normals  [i * 3 + 0], normals  [i * 3 + 1], normals  [i * 3 + 2] );
                              Vec2d UV = Vec2d( UVs[i*2 + 0], UVs[i*2 + 1] );
                              gdata.pushVert(pos);
                              gdata.pushNorm(nor);
                              gdata.pushTexCoord(UV);
                              if (accessorColor.type == 4){ auto cl = Color4f( colors[i * 4 + 0], colors[i * 4 + 1], colors[i * 4 + 2], colors[i * 4 + 2] ); gdata.pushColor(cl); }
                              //if (accessorColor.type == 4){ auto cl = Color4f( 212.0/255.0,175.0/255.0,55.0/255.0, 1 ); gdata.pushColor(cl); }
                              //if (accessorColor.type == 4){ auto cl = Color4f( 212.0/255.0,175.0/255.0,55.0/255.0, 0.8 ); gdata.pushColor(cl); }
                              n ++;
                              //positionVecs.push_back(pos);
                              //normalVecs.push_back(nor);
                              //cout << nor << endl;

                               /*std::cout << "(" << positions[i * 3 + 0] << ", "// x
                                                << positions[i * 3 + 1] << ", "// y
                                                << positions[i * 3 + 2] << ")" // z
                                                << "\n";*/
                    }
                    if (primitive.mode == 4) {
                        //DEFAULT TRIS
                        const tinygltf::Accessor& accessorIndices = model->accessors[primitive.indices];
                        const tinygltf::BufferView& bufferViewIndices = model->bufferViews[accessorIndices.bufferView];
                        const tinygltf::Buffer& bufferInd = model->buffers[bufferViewIndices.buffer];
                        const int* indices   = reinterpret_cast<const int*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                        for (size_t i = 0; i < accessorIndices.count/3; ++i) {
                            gdata.pushTri(indices[i*3+0],indices[i*3+1],indices[i*3+2]);
                        }
                    }
                }

                /*if (normalIndex.size()) { // different normal indices, same primitives!
                    for (int i : normalIndex) if (i >= 0) geo.pushNormalIndex(i);
                } else {} // nothing to do */
                //geo.apply(g);
                //if (!doNormals) g->updateNormals(false);
            }

            //for (auto p : positionVecs) { gdata.pushVert(p); }
            //for (auto n : normalVecs) gdata.pushNorm(n);
            //for ()
            //auto geo = gdata.asGeometry("arrow");
            gdata.apply(VRgeo);
            cout << "read " << n << " vertices" << endl;
            cout << "Felix kann jetzt auch triangles lul" << endl;
        }
};

void OSG::loadGLTF(string path, VRTransformPtr res, VRProgressPtr p, bool thread) {
    GLTFLoader gltf(path, res, p, thread);
    gltf.load();
}
