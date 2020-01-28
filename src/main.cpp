#include "PolyVR.h"



#ifdef WASM
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <emscripten.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <OpenSG/OSGGL.h>
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMaterialChunk.h>
#include <OpenSG/OSGShaderProgram.h>
#include <OpenSG/OSGShaderProgramChunk.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGPerspectiveCamera.h>
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGDirectionalLight.h>

#include "core/objects/OSGObject.h"

using namespace OSG;
using namespace std;

PerspectiveCameraRecPtr camera;
ViewportRecPtr viewport;
WindowRecPtr window;
NodeRecPtr scene, camBeacon, lightBeacon, lightNode;
RenderActionRefPtr renderAction;
ShaderProgramRecPtr vProgram, fProgram;
SolidBackgroundRecPtr background;
TransformRefPtr trans;
ChunkMaterialRecPtr mat;
ShaderProgramChunkRecPtr shaderChunk;
ShaderProgramChunkRecPtr shaderFailChunk;
bool matVShaderFail = false;
bool matFShaderFail = false;


string vdata = "super vertex shader!";
string fdata = "super fragment shader!";

string vFailData =
"attribute vec4 osg_Vertex;\n"
"uniform mat4 OSGModelViewProjectionMatrix;\n"
"void main(void) {\n"
"  gl_Position = OSGModelViewProjectionMatrix * osg_Vertex;\n"
"}\n";

string fFailData =
"precision mediump float;\n"
"void main(void) {\n"
"  gl_FragColor = vec4(0.0,0.8,1.0,1.0);\n"
"}\n";

bool checkShader(int type, string shader, string name) {
    //if (!eglGetCurrentContext()) return;
    GLuint shaderObject = glCreateShader(type);
    int N = shader.size();
    const char* str = shader.c_str();
    glShaderSource(shaderObject, 1, &str, &N);
    glCompileShader(shaderObject);

    GLint compiled;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &compiled);
    if (!compiled) cout << "Shader "+name+" did not compiled!\n";

    GLint blen = 0;
    GLsizei slen = 0;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH , &blen);
    if (blen > 1) {
        GLchar* compiler_log = (GLchar*)malloc(blen);
        glGetShaderInfoLog(shaderObject, blen, &slen, compiler_log);
        cout << "Shader "+name+" warnings and errors:\n";
        cout << string(compiler_log);
        free(compiler_log);
	return false;
    }
    return true;
}

NodeTransitPtr createEmptyScene(void) {
	// camera
	Matrix camM;
	camM.setTransform(Vec3f(0,  0, 10));

	TransformRecPtr camTrans  = Transform::create();
	camTrans->setMatrix(camM);

	camBeacon = Node::create();
	camBeacon->setCore(camTrans);

	// light
	Matrix lightM;
	lightM.setTransform(Vec3f( 1, 10,  2));

	TransformRecPtr lightTrans = Transform::create();
	lightTrans->setMatrix(lightM);

	lightBeacon = Node::create();
	lightBeacon->setCore(lightTrans);
	DirectionalLightRecPtr dLight = DirectionalLight::create();
	dLight->setDirection(Vec3f(0,1,2));
	dLight->setDiffuse(Color4f(1,1,1,1));
	dLight->setAmbient(Color4f(0.2,0.2,0.2,1));
	dLight->setSpecular(Color4f(1,1,1,1));
	dLight->setBeacon(lightBeacon);

	lightNode = Node::create();
	lightNode->setCore(dLight);

	// scene
	NodeRecPtr root = Node::create();
	root->setCore(Group::create());
	root->addChild(camBeacon);
	root->addChild(lightNode);
	root->addChild(lightBeacon);
	return NodeTransitPtr(root);
}

string constructShaderVP() {
	string vp;
	vp += "attribute vec4 osg_Vertex;\n";
	vp += "attribute vec3 osg_Normal;\n";
	vp += "uniform mat4 OSGModelViewProjectionMatrix;\n";
	vp += "uniform mat4 OSGNormalMatrix;\n";
	vp += "varying vec4 vertPos;\n";
	vp += "varying vec3 vertNorm;\n";
	vp += "varying vec4 color;\n";
	vp += "void main(void) {\n";
	vp += "  vertNorm = (OSGNormalMatrix * vec4(osg_Normal,1.0)).xyz;\n";
	vp += "  color = vec4(1.0,1.0,1.0,1.0);\n";
	vp += "  gl_Position = OSGModelViewProjectionMatrix * osg_Vertex;\n";
	vp += "}\n";
	return vp;
}

string constructShaderFP() {
	string fp;
	fp += "precision mediump float;\n";
	fp += "varying vec3 vertNorm;\n";
	fp += "varying vec4 color;\n";

	fp += "void main(void) {\n";
	fp += " vec3  n = normalize(vertNorm);\n";
	fp += " vec3  light = normalize( vec3(0.8,1.0,0.5) );\n";// directional light
	fp += " float NdotL = max(dot( n, light ), 0.0);\n";
	fp += " vec4  ambient = vec4(0.2,0.2,0.2,1.0) * color;\n";
	fp += " vec4  diffuse = vec4(1.0,1.0,0.9,1.0) * NdotL * color;\n";
	fp += " vec4  specular = vec4(1.0,1.0,1.0,1.0) * 0.0;\n";
        fp += " gl_FragColor = ambient + diffuse + specular;\n";
	fp += "}\n";
	return fp;
}

NodeTransitPtr createScene(void) {
	// fail material
	shaderFailChunk = ShaderProgramChunk::create();
	ShaderProgramRefPtr vFProgram = ShaderProgram::createVertexShader  ();
	ShaderProgramRefPtr fFProgram = ShaderProgram::createFragmentShader();
	vFProgram->createDefaulAttribMapping();
	vFProgram->addOSGVariable("OSGModelViewProjectionMatrix");
	vFProgram->setProgram(vFailData);
	fFProgram->setProgram(fFailData);
	shaderFailChunk->addShader(vFProgram);
	shaderFailChunk->addShader(fFProgram);

	// material
	mat = ChunkMaterial::create();
        MaterialChunkRecPtr colChunk = MaterialChunk::create();
        colChunk->setBackMaterial(false);
        mat->addChunk(colChunk);

	shaderChunk = ShaderProgramChunk::create();
#ifdef WASM
	mat->addChunk(shaderChunk);
#endif

	vProgram = ShaderProgram::createVertexShader  ();
	fProgram = ShaderProgram::createFragmentShader();
	vProgram->createDefaulAttribMapping();
	vProgram->addOSGVariable("OSGNormalMatrix");
	vProgram->addOSGVariable("OSGModelViewProjectionMatrix");

	vdata = constructShaderVP().c_str();
	fdata = constructShaderFP().c_str();

	checkShader(GL_VERTEX_SHADER, vdata, "vertex shader");
	checkShader(GL_FRAGMENT_SHADER, fdata, "fragment shader");

	vProgram->setProgram(vdata);
	fProgram->setProgram(fdata);

	shaderChunk->addShader(vProgram);
	shaderChunk->addShader(fProgram);

	// scene
	NodeRecPtr root = createEmptyScene();
	GeometryRecPtr torus = makeTorusGeo(1,5,8,16);
	//GeometryRecPtr torus = makeBoxGeo(1,1,1,1,1,1);
	torus->setMaterial(mat);
	NodeRecPtr torusN = Node::create();
	torusN->setCore(torus);
	trans = Transform::create();
	NodeRecPtr transN = Node::create();
	transN->setCore(trans);
	transN->addChild(torusN);
	lightNode->addChild(transN);
	return NodeTransitPtr(root);
}

int main(int argc, char **argv) {
    auto pvr = OSG::PolyVR::get();
	pvr->init(argc,argv);
	pvr->run();
    /*scene = createScene();
    pvr->startTestScene(OSGObject::create(scene), Vec3d(0,0,10));*/
}




