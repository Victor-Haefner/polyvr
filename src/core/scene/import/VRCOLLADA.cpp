#include "VRCOLLADA.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRAnimation.h"

#include "core/utils/rapidxml/rapidxml.hpp"
#include "core/utils/rapidxml/rapidxml_print.hpp"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/objects/object/VRObjectT.h"

#include "core/math/path.h"

#include <fstream>
#include <math.h>

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGSimpleMaterial.h>

using namespace rapidxml;
using namespace OSG;

typedef xml_node<> xNode;
typedef xml_attribute<> Attrib;

enum AnimType{
    ANIM_TRANSLATE = 0,
    ANIM_ROTATE,
    ANIM_SCALE
};

struct SamplerIn {
    string semantic;//common name like INTANGENT
    string source;//id name of a source like #Cube_location_Y-input
};

struct Sampler {
    string ID;
    map<string, string> inputs;
};

struct Channel {
    string source;
    string target;
    string type;
    AnimType aType;
    string axis;
};

struct TechniqueCommon{
    string accessor_source;
    int accessor_stride;
    int accessor_count;
    map<string, string> params;
};

struct Source {
    string ID;
    string array_element_ID;
    int array_element_count;
    string array_element;
    TechniqueCommon tq;
};

struct Animation {
    string ID;
    Channel channel;
    Sampler sampler;
    map<string, Source> sources;
};

struct AnimationLibrary {
    map<string, Animation> animations;
};

vector<xNode*> getxNodes(xNode* node, string name = "") {
    vector<xNode*> res;
    if (name.size() > 0) for (xNode* n = node->first_node(name.c_str()); n; n = n->next_sibling(name.c_str()) ) res.push_back(n);
    else                 for (xNode* n = node->first_node(            ); n; n = n->next_sibling(            ) ) res.push_back(n);
    return res;
}

AnimationLibrary parseColladaAnimations(string data) {
    xml_document<> doc;
    doc.parse<0>(&data[0]);

    AnimationLibrary library;

    xNode* node = doc.first_node("COLLADA");
    if (node) { //found COLLADA tag
        node = node->first_node("library_animations");
        if (node) { //found animations tag...look for animations
            for (xNode* animxNode : getxNodes(node, "animation")) {
                Attrib* animID = animxNode->first_attribute("id");
                xNode* channels = animxNode->first_node("channel");
                xNode* sampler = animxNode->first_node("sampler");
                if (!channels) continue;
                if (!sampler) continue;

                Attrib* source = channels->first_attribute("source");
                Attrib* target = channels->first_attribute("target");
                Attrib* samplerID = sampler->first_attribute("id");

                string tta = target->value();
                int ttaSlash = 0;
                for (int i=tta.size()-1; i>= 0; i--) if (tta[i] == '/') { ttaSlash = i; break; }
                int ttaDot = 0;
                for (int i=tta.size()-1; i>= 0; i--) if (tta[i] == '.') { ttaDot = i; break; }

                Animation anim;
                anim.ID = animID->value();
                anim.channel = Channel();
                anim.channel.source = source->value();
                //preparation for enums
                //string target = tta.substr(0, ttaSlash);
                //string type = tta.substr(ttaSlash+1, tta.size()-ttaSlash-3);
                //string axis = tta.substr(tta.size()-1, 1);
                //TODO FILIP add angle parameter
                anim.channel.target = tta.substr(0, ttaSlash);
                anim.channel.type = tta.substr(ttaSlash+1, ttaDot-ttaSlash-1);
                anim.channel.axis = tta.substr(tta.size()-1, 1);

                if(anim.channel.type.find("rotation") != string::npos){
                    anim.channel.axis = anim.channel.type.substr(anim.channel.type.size()-1, 1);
                    anim.channel.type = anim.channel.type.substr(0, anim.channel.type.size()-1);
                }

                anim.sampler = Sampler();
                anim.sampler.ID = samplerID->value();

                if (anim.channel.source.find(anim.sampler.ID) == string::npos) continue;

                for (xNode* samplerIn : getxNodes(sampler, "input")) {
                    //SamplerIn sampIn;
                    string samplerSemantic = samplerIn->first_attribute("semantic")->value();
                    string samplerSource = samplerIn->first_attribute("source")->value();
                    samplerSource = samplerSource.substr(1, samplerSource.size()-1);
                    anim.sampler.inputs[samplerSemantic] = samplerSource;

                    for (xNode* animSource : getxNodes(animxNode, "source")) {//parse through all <source> nodes
                        if (samplerSource.find(animSource->first_attribute("id")->value()) == string::npos) continue;

                        Source source;
                        source.ID = animSource->first_attribute("id")->value();
                        TechniqueCommon technique;

                        for (xNode* sourcexNode : getxNodes(animSource)) {//parse through child nodes of source node
                            string sourceName = sourcexNode->name();
                            if (sourceName.find("_array") != string::npos) {
                                source.array_element_ID = sourcexNode->first_attribute("id")->value();
                                source.array_element_count= toInt(sourcexNode->first_attribute("count")->value());
                                source.array_element = sourcexNode->value();
                            } else if(sourceName == "technique_common") {
                                xNode* acc = sourcexNode->first_node("accessor");

                                technique.accessor_source = acc->first_attribute("source")->value();
                                technique.accessor_stride = toInt(acc->first_attribute("stride")->value());
                                technique.accessor_count = toInt(acc->first_attribute("count")->value());

                                xNode* par = acc->first_node("param");

                                while(par){
                                    technique.params[par->first_attribute("name")->value()] = par->first_attribute("type")->value();
                                    par = par->next_sibling();
                                }
                                source.tq = technique;
                            }
                        }
                        anim.sources[source.ID] = source;
                    }
                }
                library.animations[anim.ID] = anim;
            }
        } else cout << "<library_animations> tag not found" << endl;
    } else cout << "<COLLADA> tag not found" << endl;
    return library;
}

void printAll(const AnimationLibrary& library) {
    cout << "Imported COLLADA animations:\n";
    cout << " Animations:\n";
    for (auto a : library.animations) {
        cout << "  Animation " << a.first << endl;

        cout << "      Channel source: " << a.second.channel.source << endl;
        cout << "      Channel target: " << a.second.channel.target << endl;
        cout << "      Channel type: " << a.second.channel.type << " | " << a.second.channel.axis << endl;

        cout << "      Sampler " << a.second.sampler.ID << endl;
        cout << "         Sampler inputs:\n";
        for (auto i : a.second.sampler.inputs) {
            cout << "            Input semantic: " << i.first << endl;
            cout << "            Input source: " << i.second << endl;
        }

        cout << "      Sources:\n";
        for (auto s : a.second.sources){
            cout << "         Source " << s.second.ID << endl;//prints source id
            cout << "            Source *_array " << s.second.array_element_ID << endl;
            cout << "               count "  << s.second.array_element_count<< endl;
            cout << "               values "  << s.second.array_element << endl;
            cout << "            Source technique_common " << endl;
            cout << "               accessor source " << s.second.tq.accessor_source << endl;
            cout << "               accessor count " << s.second.tq.accessor_count << endl;
            cout << "               accessor stride " << s.second.tq.accessor_stride << endl;
            cout << "                   params ";
            for(auto elem : s.second.tq.params){cout << "(" << elem.first << ", " << elem.second << ")  ";}
        }
    }
}



void setPose(OSG::VRTransform* o, int i, path *p, float t) {// object, axis, new axis values
    if (i < 0 || i > 2) { return; }
    Vec3f f = o->getFrom();
    if(p) f[i] = p->getPosition(t)[1];
    else f[i] = t;
    o->setFrom(f);
}

void setRot(OSG::VRTransform* o, int i, path *p, float t) {
    if (i < 0 || i > 2) { return; }
    Vec3f f = o->getEuler();
    if(p) f[i] = p->getPosition(t)[1];
    else f[i] = t;
    o->setEuler(f);
}

void setScale(OSG::VRTransform* o, int i, path *p, float t) {
    if (i < 0 || i > 2) { return; }
    Vec3f f = o->getScale();
    if(p) f[i] = p->getPosition(t)[1];
    else f[i] = t;
    o->setScale(f);
}

void setPose3(VRTransform* o, int i, Vec3f t) {
    o->setFrom(t);
}

VRObject* findTarget(VRObject* o, string Name) {
    if (o->hasAttachment("collada_name")) cout << "findTarget " << Name << " current " << o->getAttachment<string>("collada_name") << endl;
    if (o->hasAttachment("collada_name")) {
        if (o->getAttachment<string>("collada_name") == Name) return o;
    }

    for (auto c : o->getChildren(false)) {
        if (c == o) continue;
        VRObject* tmp = findTarget(c,Name);
        if (tmp != 0) return tmp;
    }
    return 0;
}

template<class T>
void string2Vector(string &input, vector<T> &output){
        std::istringstream iss(input);
        std::copy(std::istream_iterator<T>(iss),
        std::istream_iterator<T>(),
        std::back_inserter(output));
}

int getAxis(const Animation& a) {
    if (a.channel.axis == "X") return 0;
    else if (a.channel.axis == "Y") return 1;
    else if (a.channel.axis == "Z") return 2;
    return -1;
}

void buildAnimations(AnimationLibrary& lib, VRObject* objects) {
    for (auto a : lib.animations) {
        cout << "search object " << a.second.channel.target << endl;
        VRObject* obj = findTarget(objects, a.second.channel.target);
        if(obj==0) cout << "object is 0 "<< endl;
        if (obj == 0) continue;
        if(!obj->hasAttachment("transform")) cout << "has no attachment " << obj->getName() << endl;
        if (!obj->hasAttachment("transform")) continue;

        map<string, Source> sources = a.second.sources;
        Sampler sampler = a.second.sampler;
        Source inputSource = sources.find(sampler.inputs.find("INPUT")->second)->second;
        Source outputSource = sources.find(sampler.inputs.find("OUTPUT")->second)->second;
        Source interpolationSource = sources.find(sampler.inputs.find("INTERPOLATION")->second)->second;
        Source intangentSource;
        Source outtangentSource;

        if(sampler.inputs.find("IN_TANGENT") != sampler.inputs.end()) intangentSource = sources.find(sampler.inputs.find("IN_TANGENT")->second)->second;
        if(sampler.inputs.find("OUT_TANGENT") != sampler.inputs.end()) outtangentSource = sources.find(sampler.inputs.find("OUT_TANGENT")->second)->second;

        vector<float> inputValues;
        vector<float> outputValues;
        vector<string> interpolationValues;
        vector<float> intangentValues;
        vector<float> outtangentValues;

        string2Vector(inputSource.array_element, inputValues);
        string2Vector(outputSource.array_element, outputValues);
        string2Vector(interpolationSource.array_element, interpolationValues);
        string2Vector(intangentSource.array_element, intangentValues);
        string2Vector(outtangentSource.array_element, outtangentValues);

        VRTransform* t = (VRTransform*)obj;
        int axis = getAxis(a.second);

        VRFunction<float>* fkt;
        bool bezier = false;

        path* p = 0;

        void (*callback)(OSG::VRTransform* o, int i, path *p, float t) = 0;
        if (a.second.channel.type == "rotation") callback = setRot;
        if (a.second.channel.type == "location") callback = setPose;
        if (a.second.channel.type == "scale") callback = setScale;

        if (a.second.channel.type == "rotation") {
            for (size_t i=0; i<outputValues.size(); i++) outputValues[i] = outputValues[i]*Pi/180.f; // convert degrees to radians
            for (size_t i=1; i<outtangentValues.size(); i+=2) outtangentValues[i] = outtangentValues[i]*Pi/180.f; // convert degrees to radians
            for (size_t i=1; i<intangentValues.size(); i+=2) intangentValues[i] = intangentValues[i]*Pi/180.f; // convert degrees to radians
        }

        //used for linear interpolation
        for(int i = 0; i < inputSource.array_element_count - 1; i++){
            if(interpolationValues[i] == "BEZIER") bezier = true;
            else bezier = false;

            Vec3f start(inputValues[i], outputValues[i], 0);
            Vec3f end(inputValues[i+1], outputValues[i+1], 0);
            float duration = end[0] - start[0];
            if (bezier) {
                p = new path();
                Vec3f h0 = Vec3f(outtangentValues[2*i], outtangentValues[2*i+1], 0) - start;
                Vec3f h1 = Vec3f(intangentValues[2*i+2], intangentValues[2*i+1+2], 0) - end;
                p->addPoint( start, h0, Vec3f() );
                p->addPoint( end, h1, Vec3f() );
                p->compute(80);
            }
            bool loop = false;
            fkt = new VRFunction<float>(a.first, boost::bind(callback, t, axis, p, _1) );

            VRAnimation* anim = 0;
            if (bezier) anim = new VRAnimation(duration, start[0], fkt, 0.f, 1.f, loop);
            else anim = new VRAnimation(duration, start[0], fkt, start[1], end[1], loop);
            t->addAnimation(anim);
        }
    }
}

VRObject* OSG::loadCollada(string path, VRObject* objects) {
    ifstream file(path);
    string data( (std::istreambuf_iterator<char>(file) ), (std::istreambuf_iterator<char>() ) );
    file.close();

    auto library = parseColladaAnimations(data);

    //printAll(library);

    buildAnimations(library, objects);

    VRObject* res = new VRObject("COLLADA");
    //res->addChild(n);
    return res;
}
