#include "VRCOLLADA.h"
#include "core/objects/geometry/VRGeometry.h"

#include "core/utils/rapidxml/rapidxml.hpp"
#include "core/utils/rapidxml/rapidxml_print.hpp"

#include <fstream>

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGSimpleMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace rapidxml;

NodeTransitPtr fileReadCallback(string data){
    NodeTransitPtr ptr;

    //cout << "File Type: " << type->getName() << endl;

    //cout << "stream: ";
    //std::copy(std::istream_iterator<char>(is), std::istream_iterator<char>(), std::ostream_iterator<char>(cout));
    //cout << endl;

    //cout << "ext " << ext <<"\t with type: "; type->print(); cout << endl;

    //ptr = VRSceneLoader::get()->getDefaultReadOSG()(type, is, ext);
    //ptr = SceneFileHandler::the()->read(is,ext);//when reading with isstream, you need the extension as well
    cout << "printing rapidxml document:" << endl;

    xml_document<> doc;
    doc.parse<0>(&data[0]);



    xml_node<> *node = doc.first_node("COLLADA");
    if(node){ //found COLLADA tag

        node = node->first_node("library_animations");
        if(node){ //found animations tag...look for animations

            xml_node<> *animNode = node->first_node("animation");
            while(animNode){
                xml_attribute<> *animNode_id = animNode->first_attribute("id");
                cout << animNode_id->value() << endl;

                /*
                Introduction
                    Declares an output channel of an animation.
                Concepts
                    As an animation’s sampler transforms values over time, those values are directed out to channels. The
                    animation channels describe where to store the transformed values from the animation engine. The
                    channels target the data structures
                    that receive the animated values.
                */
                xml_node<> *animation_nodes = animNode->first_node("channel");
                if(!animation_nodes) {cout << "<channel> tag not found" << endl; animNode = animNode->next_sibling("animation"); continue;} //no channel found!

                xml_attribute<> *animationChannel_source = animation_nodes->first_attribute("source");
                cout << animationChannel_source->value() << endl;
                xml_attribute<> *animationChannel_target = animation_nodes->first_attribute("target");
                cout << animationChannel_target->value() << endl;
                //TODO check if target exists in the tree

                /*
                Introduction
                    Declares an interpolation sampling function for an animation.
                Concepts
                    Animation function curves are represented by 1-D <sampler>
                    elements in COLLADA. The sampler
                    defines sampling points and how to
                    interpolate between them. When used to compute values for an
                    animation channel, the sampling points are the animation key frames.
                    Sampling points (key frames) are input data sources
                    to the sampler, as are interpolation type symbolic
                    names. Animation channels direct
                    the output data values of
                    the sampler to their targets.
                */
                animation_nodes = animNode->first_node("sampler");
                if(!animation_nodes) {cout << "<sampler> tag not found" << endl; animNode = animNode->next_sibling("animation"); continue;} //no channel found!
                xml_attribute<> *animationSampler_id = animation_nodes->first_attribute("id");
                cout << animationSampler_id->value() << endl;

                std::string string_with_tag = animationChannel_source->value();
                std::string string_compare = animationSampler_id->value();


                if (string_with_tag.find(string_compare) == string::npos){cout << "could not find animation sampler id in source name!" << endl; animNode = animNode->next_sibling("animation"); continue;}

                cout << "getting sampler inputs" << endl;
                xml_node<> *animationSampler_input = animation_nodes->first_node("input");
                while(animationSampler_input){
                    xml_attribute<> *animationSampler_inputSemantic =animationSampler_input->first_attribute("semantic");
                    xml_attribute<> *animationSampler_inputSource =animationSampler_input->first_attribute("source");
                    cout << animationSampler_inputSemantic->value() << endl;

                    xml_node<> *animation_source = animNode->first_node("source");
                    while(animation_source){
                        cout << animation_source->first_attribute("id")->value() << endl;
                        string_with_tag = animationSampler_inputSource->value();
                        string_compare = animation_source->first_attribute("id")->value();
                        if(string_with_tag.find(string_compare) != string::npos){
                            //now we have the semantics (sampler node) INPUT,OUTPUT,INTERPOLATION,INTANGENT and OUTTANGET to parse through...
                            cout << " == " << endl;
                            xml_node<> *animationSource_node = animation_source->first_node();
                            while(animationSource_node){
                                cout << animationSource_node->name() << endl;
                                //float/name_array and technique_common

                                string animationSource_node_name = animationSource_node->name();
                                if(animationSource_node_name.find("_array") != string::npos){ // parse array

                                } else if(animationSource_node_name =="technique_common"){

                                }
                                ////////////////////////////////////////////////
                                //VRFunction<float>* fkt = new VRFunction<float>("TransAnim", boost::bind(setFromPath, this, p, redirect, _1));//this = get the object that is to be animated
                                //VRScene* scene = VRSceneManager::getCurrent();
                                //int a = scene->addAnimation(4.125, 0, fkt, 0.f, 1.f, true);
                                ////////////////////////////////////////////////


                                animationSource_node = animationSource_node->next_sibling();
                            }
                            break;
                        }
                        animation_source = animation_source->next_sibling("source");
                    }
                    animationSampler_input = animationSampler_input->next_sibling("input");
                }
                animNode = animNode->next_sibling("animation");//get next animation node when done parsing previous
            }
        } else cout << "<library_animations> tag not found" << endl;
    } else cout << "<COLLADA> tag not found" << endl;

    //cout << doc << endl;  // 0 means default printing flags

    cout << "the import worked" << endl;

    return Node::create();
}



VRObject* loadCollada(string path, VRObject* objects) {
    ifstream file(path);
    string content( (std::istreambuf_iterator<char>(file) ), (std::istreambuf_iterator<char>() ) );
    file.close();
    NodeRecPtr n = fileReadCallback(content);

    VRObject* res = new VRObject("COLLADA");
    res->addChild(n);
    return res;
}

OSG_END_NAMESPACE;
