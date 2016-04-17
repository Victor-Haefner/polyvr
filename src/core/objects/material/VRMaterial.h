#ifndef VRMATERIAL_H_INCLUDED
#define VRMATERIAL_H_INCLUDED

#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"

class VRVideo;

OSG_BEGIN_NAMESPACE;
using namespace std;

class Material; OSG_GEN_CONTAINERPTR(Material);
class Image; OSG_GEN_CONTAINERPTR(Image);
class ShaderProgram; OSG_GEN_CONTAINERPTR(ShaderProgram);
class ChunkMaterial; OSG_GEN_CONTAINERPTR(ChunkMaterial);
class MultiPassMaterial; OSG_GEN_CONTAINERPTR(MultiPassMaterial);
class TextureObjChunk; OSG_GEN_CONTAINERPTR(TextureObjChunk);

struct VRMatData;

Color4f toColor4f(Color3f c, float t = 1);
Color3f toColor3f(Color4f c);

class VRMaterial : public VRObject {
    public:
        static map<string, VRMaterialWeakPtr> materials;
        static map<MaterialMTRecPtr, VRMaterialWeakPtr> materialsByPtr;

        string constructShaderVP(VRMatData* data);
        string constructShaderFP(VRMatData* data);

        void setup();

    protected:
        MultiPassMaterialMTRecPtr passes;
        vector<VRMatData*> mats;
        int activePass = 0;
        bool force_transparency = false;
        bool deferred = false;

        VRObjectPtr copy(vector<VRObjectPtr> children);

        bool isCMat(MaterialMTUncountedPtr matPtr);
        bool isSMat(MaterialMTUncountedPtr matPtr);
        bool isSTMat(MaterialMTUncountedPtr matPtr);

    public:
        VRMaterial(string name);
        virtual ~VRMaterial();

        static VRMaterialPtr create(string name = "None");
        VRMaterialPtr ptr();

        void setDeffered(bool b);

        void setActivePass(int i);
        int getActivePass();
        int getNPasses();
        int addPass();
        void remPass(int i);
        void appendPasses(VRMaterialPtr mat);
        void prependPasses(VRMaterialPtr mat);
        void clearExtraPasses();

        static VRMaterialPtr getDefault();
        static VRMaterialPtr get(MaterialMTRecPtr mat);
        static VRMaterialPtr get(string s);
        static void clearAll();

        //** Set the material of the mesh **/
        void setMaterial(MaterialMTRecPtr mat);
        void resetDefault();

        /** Load a texture && apply it to the mesh as new material **/
        TextureObjChunkMTRefPtr getTexChunk(int unit);
        void setTexture(TextureObjChunkMTRefPtr texChunk, int unit = 0);
        void setTexture(string img_path, bool alpha = true, int unit = 0);
        void setTexture(VRTexturePtr img, bool alpha = true, int unit = 0);
        void setTexture(char* data, int format, Vec3i dims, bool isfloat);
        void setTextureAndUnit(VRTexturePtr img, int unit);
        void setTextureParams(int min, int mag, int envMode = GL_MODULATE, int wrapS = GL_REPEAT, int wrapT = GL_REPEAT, int unit = 0);
        void setTextureType(string type);
        void setQRCode(string s, Vec3f fg, Vec3f bg, int offset);
        void setVideo(string vid_path);
        VRVideo* getVideo();

        void toggleMaterial(string mat1, string mat2, bool b);

        void setDiffuse(Color3f c);
        void setSpecular(Color3f c);
        void setAmbient(Color3f c);
        void setShininess(float s);
        void setEmission(Color3f c);
        void setTransparency(float t);
        void setDepthTest(int d);
        void enableTransparency(bool user_override = false);
        void clearTransparency(bool user_override = false);
        void setLineWidth(int w, bool smooth = true);
        void setPointSize(int s, bool smooth = true);
        void setWireFrame(bool b);
        void setZOffset(float factor, float bias);
        void setSortKey(int key);
        void setFrontBackModes(int front, int back);
        void setClipPlane(bool active, Vec4f equation, VRTransformPtr beacon);
        void setStencilBuffer(bool clear, float value, float mask, int func, int opFail, int opZFail, int opPass);

        Color3f getDiffuse();
        Color3f getSpecular();
        Color3f getAmbient();
        float getShininess();
        Color3f getEmission();
        float getTransparency();

        void initShaderChunk();
        void remShaderChunk();
        void setDefaultVertexShader();
        void setVertexShader(string s);
        void setFragmentShader(string s);
        void setGeometryShader(string s);
        void readVertexShader(string s);
        void readFragmentShader(string s);
        void readGeometryShader(string s);
        void setVertexScript(string script);
        void setFragmentScript(string script);
        void setGeometryScript(string script);
        string getVertexShader();
        string getFragmentShader();
        string getGeometryShader();
        string getVertexScript();
        string getFragmentScript();
        string getGeometryScript();
        ShaderProgramMTRecPtr getShaderProgram();

        template<class T> void setShaderParameter(string name, const T &value);

        void setMagMinFilter(string mag, string min);

        void setLit(bool b);
        bool isLit();

        MultiPassMaterialMTRecPtr getMaterial();
        ChunkMaterialMTRecPtr getMaterial(int i);
        VRTexturePtr getTexture(int unit = 0);
        TextureObjChunkMTRecPtr getTextureObjChunk(int unit = 0);
};

OSG_END_NAMESPACE;

#endif // VRMATERIAL_H_INCLUDED
