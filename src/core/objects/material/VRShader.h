#ifndef VRSHADER_H_INCLUDED
#define VRSHADER_H_INCLUDED

#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMaterialGroup.h>
#include <string>
#include <fstream>
#include <streambuf>

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRShader {
    private:
        SimpleSHLChunkRecPtr shader_chunk;

        static string vs_program;
        static string fs_program;
        static string gs_program;

        string getFromFile(string path);

    public:
        VRShader(ChunkMaterialRecPtr mat);

        SimpleSHLChunkRecPtr getShader();

        void setVertexProgram(string path);
        void setFragmentProgram(string path);
        void setGeometryProgram(string path);

        template<class T> inline
        void addParameter(string name, const T &value) {
            shader_chunk->addUniformVariable(name.c_str(), value);
        }
        template<class T> inline
        void setParameter(string name, const T &value) {
            //shader_chunk->updateUniformVariable(name.c_str(), value);
            shader_chunk->subUniformVariable(name.c_str());
            shader_chunk->addUniformVariable(name.c_str(), value);
        }
};

OSG_END_NAMESPACE;

#endif // VRSHADER_H_INCLUDED
