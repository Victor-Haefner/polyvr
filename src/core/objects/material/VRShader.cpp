#include "VRShader.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

string VRShader::getFromFile(string path) {
    ifstream file(path.c_str());
    string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return str;
}

VRShader::VRShader(ChunkMaterialRecPtr mat) {
    shader_chunk = SimpleSHLChunk::create();
    mat->addChunk(shader_chunk);
}

SimpleSHLChunkRecPtr VRShader::getShader() { return shader_chunk; }

void VRShader::setVertexProgram(string path) { shader_chunk->setVertexProgram(getFromFile(path).c_str()); }
void VRShader::setFragmentProgram(string path) { shader_chunk->setFragmentProgram(getFromFile(path).c_str()); }
void VRShader::setGeometryProgram(string path) { shader_chunk->setGeometryProgram(getFromFile(path).c_str()); }

OSG_END_NAMESPACE;
