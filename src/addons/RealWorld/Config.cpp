#include "Config.h"
#include "core/objects/material/VRShader.h"
#include "core/scene/VRSceneManager.h"
#include <OpenSG/OSGSimpleMaterial.h>
#include <sys/stat.h>

using namespace OSG;
using namespace std;

Config::Config() {
    // override defaults:
    LAYER_DISTANCE = 0.04;
    STREET_HEIGHT = 0;
    SIGN_DISTANCE = 3;
}

Config* Config::get() {
    static Config* c = new Config();
    return c;
}

//Lighting && Shading
void Config::createPhongShader(SimpleMaterial* mat, bool phong) {
    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    if(phong){
        mat->setAmbient(Color3f(0.5, 0.5, 0.5)); //light reflection in all directions
        mat->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambiente (without lightsource)
        mat->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction
        VRShader* wshader = new VRShader(mat);
        wshader->setVertexProgram(wdir+"/shader/TexturePhong/phong.vp"); //Vertex Shader
        wshader->setFragmentProgram(wdir+"/shader/TexturePhong/phong.fp"); //Fragment Shader
    } else { //without phong shading, but with transparent textures
        mat->setAmbient(Color3f(1.0, 1.0, 1.0));
        VRShader* wshader = new VRShader(mat);
        wshader->setVertexProgram(wdir+"/shader/TexturePhong/phong.vp"); //Vertex Shader
        wshader->setFragmentProgram(wdir+"/shader/TexturePhong/phong_tree.fp"); //Fragment Shader
    }
}

//Start Position
Vec2d Config::getStartPosition(){
    Vec2d tierGarten = Vec2d(48.998969, 8.400171); // Tiergarten
    Vec2d fasanengarten = Vec2d(49.013606, 8.418295);
    Vec2d karlKriegStr = Vec2d(49.005, 8.395); //Kreuzung Kriegsstr. und Karlstr.
    Vec2d karlWolfWeg = Vec2d(49.000324, 8.371793);
    Vec2d muehlburgerTor = Vec2d(49.010247, 8.388852);
    Vec2d schloss = Vec2d(49.013449, 8.404389);
    Vec2d huegel = Vec2d(48.912276, 8.448914); //große Höhenunterschiede
    return tierGarten;
}
