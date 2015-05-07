#include "VRCOLLADA.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRAnimation.h"

#include "core/utils/rapidxml/rapidxml.hpp"
#include "core/utils/rapidxml/rapidxml_print.hpp"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/objects/object/VRObjectT.h"

#include <fstream>

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGSimpleMaterial.h>

using namespace rapidxml;
using namespace OSG;

typedef xml_node<> xNode;
typedef xml_attribute<> Attrib;

struct SamplerIn {
    string semantic;//common name like INTANGENT
    string source;//id name of a source like #Cube_location_Y-input
};

struct Sampler {
    string ID;
    map<string, string> inputs;
    //vector<SamplerIn> inputs;
};

struct Channel {
    string source;
    string target;
    string type;
    string axis;
};

/*

Source has 2 child nodes: array_element (variable) and technique common
array_element can be: have id and count of elements in the array
        <IDREF_array>
        <Name_array> used by interpolation input
        <bool_array>
        <float_array> by all others
        <int_array>

technique_common
    accessor has source=array_element ID and count/stride=how the data in the array should be parsed
        param (1+) a name (eg X or TIME) these define like TIME or float for array_element

*/

struct Source {
    string ID;
    xNode* array_element;
    xNode* technique_common;
   // string data;//contains the data associated with semantic, see above comment
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

                Animation anim;
                anim.ID = animID->value();
                anim.channel = Channel();
                anim.channel.source = source->value();
                anim.channel.target = tta.substr(0, ttaSlash);
                anim.channel.type = tta.substr(ttaSlash+1, tta.size()-ttaSlash-3);
                anim.channel.axis = tta.substr(tta.size()-1, 1);
                anim.sampler = Sampler();
                anim.sampler.ID = samplerID->value();

                if (anim.channel.source.find(anim.sampler.ID) == string::npos) continue;

                for (xNode* samplerIn : getxNodes(sampler, "input")) {
                    //SamplerIn sampIn;
                    string samplerSemantic = samplerIn->first_attribute("semantic")->value();
                    string samplerSource = samplerIn->first_attribute("source")->value();
                    samplerSource = samplerSource.substr(1, samplerSource.size()-1);

                    anim.sampler.inputs[samplerSemantic] = samplerSource;

                    for (xNode* animSource : getxNodes(animxNode, "source")) {
                        Source source;
                        source.ID = animSource->first_attribute("id")->value();

                        if (samplerSource.find(source.ID) != string::npos) {
                            //now we have the semantics (sampler node) INPUT,OUTPUT,INTERPOLATION,INTANGENT and OUTTANGET to parse through...
                            for (xNode* sourcexNode : getxNodes(animSource)) {
                                string sourceName = sourcexNode->name();
                                if (sourceName.find("_array") != string::npos) { // parse array
                                    source.array_element = sourcexNode;
                                } else if(sourceName == "technique_common") {
                                    source.technique_common = sourcexNode;
                                }
                            }
                            break;
                        }

                        anim.sources[source.ID] = source;
                    }

                    //anim.sampler.inputs.push_back(sampIn);
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
            cout << "            Source technique " << s.second.technique_common << endl;//prints source id
            cout << "            Source array " << s.second.array_element << endl;//prints source id
        }
    }
}

void setPose(OSG::VRTransform* o, int i, float t) {
    cout << "setPose " << o->getName() << " " << t << endl;
    Vec3f f = o->getFrom();
    f[i] = t;
    o->setFrom(f);
}

void setPose3(VRTransform* o, int i, Vec3f t) {
    o->setFrom(t);
}

VRObject* findTarget(VRObject* o, string Name) {
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

void buildAnimations(AnimationLibrary& lib, VRObject* objects) {
    for (auto a : lib.animations) {
        VRObject* obj = findTarget(objects, a.second.channel.target);
        if (obj == 0) continue;
        if (!obj->hasAttachment("transform")) continue;

        float duration = 0;
        float offset = 0.0;
        bool loop = false;
        int i = 0; // x axis

        map<string, Source> sources = a.second.sources;
        Sampler sampler = a.second.sampler;
        Source inputSource = sources.find(sampler.inputs.find("INPUT")->second)->second;
        cout << "starting animation build for source: " << inputSource.ID << endl;


        xNode* technique_common = inputSource.technique_common;
        xNode* accessor = technique_common->first_node("accessor");

        int inputCount = toInt(accessor->first_attribute("count")->value());
        int inputStride = toInt(accessor->first_attribute("stride")->value());
        string arrayType =  accessor->first_node("param")->first_attribute("type")->value();//differentiate type

        vector<float> inputValues;
        string arrayElemCount = inputSource.array_element->first_attribute("count")->value();




  // If possible, always prefer std::vector to naked array

  // Build an istream that holds the input string
        std::istringstream iss(inputSource.array_element->value());
        cout << "values to be parsed: " << inputSource.array_element->value() << endl;

  // Iterate over the istream, using >> to grab floats
  // and push_back to store them in the vector
        std::copy(std::istream_iterator<float>(iss),
        std::istream_iterator<float>(),
        std::back_inserter(inputValues));

  // Put the result on standard out
        cout << "input values: ";
        for(float i : inputValues) cout << i <<" ";

        cout << "accessor count " << inputCount << endl;
        cout << "accessor stride " << inputStride << endl;
        cout << "arrayType " << arrayType << endl;




/*
        for(auto in : a.second.sampler.inputs){
            cout << "***********buildAnimations, in.source" << in.second << endl;
            auto source = sources.find(in.second)->second;
            cout << "***********buildAnimations, source name" <<source.ID << endl;

            xNode* accessor = source.technique_common->first_node("accessor");




            if(in.semantic == "INPUT") {
                cout << "***INPUT: "  << endl;
               // cout << "***INPUT: " << input.ID << endl;

               //my source is the input, so time
            }
            else if(in.semantic == "OUTPUT") {
                cout << "***OUTPUT" << endl;
            }
            else if(in.semantic == "INTERPOLATION") {
                cout << "***INTERPOLATION" << endl;
            }
            else if(in.semantic == "IN_TANGENT") {
                cout << "***INTANGENT" << endl;
            }
            else if(in.semantic == "OUT_TANGENT") {
                cout << "***OUTTANGENT" << endl;
            }


            cout << "accessor count " << counts << endl;
            cout << "accessor stride " << stride << endl;

        }


        */
        VRTransform* t = (VRTransform*)obj;

        //float duration = 2.0;
        //float offset = 0.0;
        //bool loop = false;

        //int i = 0; // x axis
        auto fkt = new VRFunction<float>(a.first, boost::bind(setPose, t, i, _1) );
        float start = 0.0;
        float end = 1.0;

        //auto fkt = new VRFunction<Vec3f>(a.first, boost::bind(setPose, t, i, _1) );
        //Vec3f start = Vec3f(x,y,z);
        //Vec3f end = Vec3f(2*x,y,z);

        VRAnimation* anim = new VRAnimation(duration, offset, fkt, start, end, loop);
        t->addAnimation(anim);
    }
}

VRObject* OSG::loadCollada(string path, VRObject* objects) {
    ifstream file(path);
    string data( (std::istreambuf_iterator<char>(file) ), (std::istreambuf_iterator<char>() ) );
    file.close();

    auto library = parseColladaAnimations(data);
    buildAnimations(library, objects);

    printAll(library);

    VRObject* res = new VRObject("COLLADA");
    //res->addChild(n);
    return res;
}
