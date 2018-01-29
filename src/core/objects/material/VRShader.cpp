#include "VRShader.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

string VRShader::getFromFile(string path) {
    ifstream file(path.c_str());
    string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return str;
}

VRShader::VRShader(ChunkMaterialMTRecPtr mat) {
    shader_chunk = ShaderProgramChunk::create();
    mat->addChunk(shader_chunk);

    vProgram = ShaderProgram::createVertexShader  ();
    fProgram = ShaderProgram::createFragmentShader();
    gProgram = ShaderProgram::createGeometryShader();
    shader_chunk->addShader(vProgram);
    shader_chunk->addShader(fProgram);
    shader_chunk->addShader(gProgram);

    vProgram->createDefaulAttribMapping();
    vProgram->addOSGVariable("OSGViewportSize");
}

ShaderProgramChunkMTRecPtr VRShader::getShader() { return shader_chunk; }

void VRShader::setVertexProgram(string path) { vProgram->setProgram(getFromFile(path).c_str()); }
void VRShader::setFragmentProgram(string path) { fProgram->setProgram(getFromFile(path).c_str()); }
void VRShader::setGeometryProgram(string path) { gProgram->setProgram(getFromFile(path).c_str()); }

OSG_END_NAMESPACE;
