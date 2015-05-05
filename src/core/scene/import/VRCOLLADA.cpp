#include "VRCOLLADA.h"
#include "core/objects/geometry/VRGeometry.h"

#include "core/utils/rapidxml/rapidxml.hpp"
#include "core/utils/rapidxml/rapidxml_print.hpp"

#include <fstream>

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGSimpleMaterial.h>

using namespace rapidxml;

typedef xml_node<> Node;
typedef xml_attribute<> Attrib;

struct SamplerIn {
    string semantic;
    string source;
};

struct Sampler {
    string ID;
    vector<SamplerIn> inputs;
};

struct Channel {
    string source;
    string target;
};

struct Source {
    string ID;
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

vector<Node*> getNodes(Node* node, string name = "") {
    vector<Node*> res;
    if (name.size() > 0) for (Node* n = node->first_node(name.c_str()); n; n = n->next_sibling(name.c_str()) ) res.push_back(n);
    else                 for (Node* n = node->first_node(            ); n; n = n->next_sibling(            ) ) res.push_back(n);
    return res;
}

AnimationLibrary parseColladaAnimations(string data) {
    xml_document<> doc;
    doc.parse<0>(&data[0]);

    AnimationLibrary library;

    Node* node = doc.first_node("COLLADA");
    if (node) { //found COLLADA tag
        node = node->first_node("library_animations");
        if (node) { //found animations tag...look for animations
            for (Node* animNode : getNodes(node, "animation")) {
                Attrib* animID = animNode->first_attribute("id");
                Node* channels = animNode->first_node("channel");
                Node* sampler = animNode->first_node("sampler");
                if (!channels) continue;
                if (!sampler) continue;

                Attrib* source = channels->first_attribute("source");
                Attrib* target = channels->first_attribute("target");
                Attrib* samplerID = sampler->first_attribute("id");

                Animation anim;
                anim.ID = animID->value();
                anim.channel = Channel();
                anim.channel.source = source->value();
                anim.channel.target = target->value();
                anim.sampler = Sampler();
                anim.sampler.ID = samplerID->value();

                if (anim.channel.source.find(anim.sampler.ID) == string::npos) continue;

                for (Node* samplerIn : getNodes(sampler, "input")) {
                    SamplerIn sampIn;
                    sampIn.semantic = samplerIn->first_attribute("semantic")->value();
                    sampIn.source = samplerIn->first_attribute("source")->value();

                    for (Node* animSource : getNodes(animNode, "source")) {
                        Source source;
                        source.ID = animSource->first_attribute("id")->value();

                        if (sampIn.source.find(source.ID) != string::npos) {
                            //now we have the semantics (sampler node) INPUT,OUTPUT,INTERPOLATION,INTANGENT and OUTTANGET to parse through...
                            for (Node* sourceNode : getNodes(animSource)) {
                                string sourceName = sourceNode->name();
                                if (sourceName.find("_array") != string::npos) { // parse array

                                } else if(sourceName == "technique_common") {

                                }
                                ////////////////////////////////////////////////
                                //VRFunction<float>* fkt = new VRFunction<float>("TransAnim", boost::bind(setFromPath, this, p, redirect, _1));//this = get the object that is to be animated
                                //VRScene* scene = VRSceneManager::getCurrent();
                                //int a = scene->addAnimation(4.125, 0, fkt, 0.f, 1.f, true);
                                ////////////////////////////////////////////////
                            }
                            break;
                        }

                        anim.sources[source.ID] = source;
                    }

                    anim.sampler.inputs.push_back(sampIn);
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
        cout << "   Channel source: " << a.second.channel.source << endl;
        cout << "   Channel target: " << a.second.channel.target << endl;
        cout << "   Sources:\n";
        for (auto s : a.second.sources) cout << "    Source " << s.first << endl;
        cout << "   Sampler " << a.second.sampler.ID << endl;
        cout << "    Sampler inputs:\n";
        for (auto i : a.second.sampler.inputs) {
            cout << "     Input source: " << i.source << endl;
            cout << "     Input semantic: " << i.semantic << endl;
        }
    }
}

using namespace OSG;

VRObject* OSG::loadCollada(string path, VRObject* objects) {
    ifstream file(path);
    string data( (std::istreambuf_iterator<char>(file) ), (std::istreambuf_iterator<char>() ) );
    file.close();

    auto library = parseColladaAnimations(data);

    printAll(library);

    VRObject* res = new VRObject("COLLADA");
    //res->addChild(n);
    return res;
}
