#ifndef VRMATERIAL_H_INCLUDED
#define VRMATERIAL_H_INCLUDED

#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGGLEXT.h> // for GL_MODULATE

#ifdef __EMSCRIPTEN__
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define GL_SPHERE_MAP 0x2402
#endif

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Material; OSG_GEN_CONTAINERPTR(Material);
class Image; OSG_GEN_CONTAINERPTR(Image);
class ShaderProgram; OSG_GEN_CONTAINERPTR(ShaderProgram);
class ChunkMaterial; OSG_GEN_CONTAINERPTR(ChunkMaterial);
class MultiPassMaterial; OSG_GEN_CONTAINERPTR(MultiPassMaterial);
class TextureObjChunk; OSG_GEN_CONTAINERPTR(TextureObjChunk);

struct VRMatData;
typedef shared_ptr<VRMatData> VRMatDataPtr;

Color4f toColor4f(Color3f c, float t = 1);
Color3f toColor3f(Color4f c);

class VRMaterial : public VRObject {
    public:
        static map<string, VRMaterialWeakPtr> materials;
        static map<MaterialMTRecPtr, VRMaterialWeakPtr> materialsByPtr;
        static map<size_t, size_t> fieldContainerMap;

        void setup();

    protected:
        OSGMaterialPtr passes;
        vector<VRMatDataPtr> mats;
        int activePass = 0;
        bool force_transparency = false;
        bool deferred = false;

        VRObjectPtr copy(vector<VRObjectPtr> children);

        bool isCMat(MaterialMTUncountedPtr matPtr);
        bool isSMat(MaterialMTUncountedPtr matPtr);
        bool isSTMat(MaterialMTUncountedPtr matPtr);

        bool checkShader(int type, string shader, string name);
        void forceShaderUpdate();

    public:
        VRMaterial(string name);
        virtual ~VRMaterial();

        static VRMaterialPtr create(string name = "None");
        VRMaterialPtr ptr();

        void init();

        void setDeferred(bool b);
        void updateDeferredShader();
        void updateOGL2Shader();
        void updateOGL2Parameters();
        void testFix();

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
        static vector<VRMaterialPtr> getAll();
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
        void setTextureAndUnit(VRTexturePtr img, int unit = 0);
        void setCubeTexture(VRTexturePtr img, string side, int unit = 0);
        void setTextureParams(int min, int mag, int envMode = GL_MODULATE, int wrapS = GL_REPEAT, int wrapT = GL_REPEAT, int unit = 0);
        void setMagMinFilter(int mag, int min, int unit = 0);
        void setTextureWrapping(int wrapS, int wrapT, int unit = 0);
        void setTextureType(string type, int unit = 0);
        void setMappingBeacon(VRObjectPtr obj, int unit = 0);
        void setMappingPlanes(Vec4d p1, Vec4d p2, Vec4d p3, Vec4d p4, int unit = 0);
        void setQRCode(string s, Vec3d fg, Vec3d bg, int offset);
        VRVideoPtr setVideo(string vid_path);
        VRVideoPtr getVideo();

        void toggleMaterial(string mat1, string mat2, bool b);

        static Color3f toColor(string c);

        void setDiffuse(string c);
        void setSpecular(string c);
        void setAmbient(string c);
        void setDiffuse(Color3f c);
        void setSpecular(Color3f c);
        void setAmbient(Color3f c);
        void setShininess(float s);
        void setEmission(Color3f c);
        void setTransparency(float t);
        void setDepthTest(int d);
        void ignoreMeshColors(bool b);
        void enableTransparency(bool user_override = false);
        void clearTransparency(bool user_override = false);
        void setLineWidth(int w, bool smooth = true);
        void setPointSize(int s, bool smooth = true);
        void setWireFrame(bool b);
        void setZOffset(float factor, float bias);
        void setSortKey(int key);
        void setFrontBackModes(int front, int back);
        void setClipPlane(bool active, Vec4d equation, VRTransformPtr beacon);
        void setStencilBuffer(bool clear, float value, float mask, int func, int opFail, int opZFail, int opPass);

        bool isWireFrame();
        bool doesIgnoreMeshColors();

        Color3f getDiffuse();
        Color3f getSpecular();
        Color3f getAmbient();
        float getShininess();
        Color3f getEmission();
        float getTransparency();
        int getDepthTest();

        string constructShaderVP(VRMatDataPtr data = 0);
        string constructShaderFP(VRMatDataPtr data = 0, bool deferred = true, int forcedTextureDim = -1 );

        void initShaderChunk();
        void remShaderChunk();
        void setDefaultVertexShader();
        void setVertexShader(string s, string name);
        void setFragmentShader(string s, string name, bool deferred = false);
        void setGeometryShader(string s, string name);
        void setTessControlShader(string s, string name);
        void setTessEvaluationShader(string s, string name);
        void readVertexShader(string s);
        void readFragmentShader(string s, bool deferred = false);
        void readGeometryShader(string s);
        void readTessControlShader(string s);
        void readTessEvaluationShader(string s);
        void setVertexScript(string script);
        void setFragmentScript(string script, bool deferred = false);
        void setGeometryScript(string script);
        void setTessControlScript(string script);
        void setTessEvaluationScript(string script);
        string getVertexShader();
        string getFragmentShader(bool deferred = false);
        string getGeometryShader();
        string getTessControlShader();
        string getTessEvaluationShader();
        string getVertexScript();
        string getFragmentScript(bool deferred = false);
        string getGeometryScript();
        string getTessControlScript();
        string getTessEvaluationScript();
        ShaderProgramMTRecPtr getShaderProgram();

        template<class T> void setShaderParameter(string name, const T &value);
        void enableShaderParameter(string name);

        void setLit(bool b);
        bool isLit();

        OSGMaterialPtr getMaterial();
        ChunkMaterialMTRecPtr getMaterial(int i);
        VRTexturePtr getTexture(int unit = 0);
        TextureObjChunkMTRecPtr getTextureObjChunk(int unit = 0);

        string diffPass(VRMaterialPtr, int pass);
        string diff(VRMaterialPtr);
};

OSG_END_NAMESPACE;

#endif // VRMATERIAL_H_INCLUDED
