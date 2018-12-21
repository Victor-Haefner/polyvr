#ifndef DEFERREDTEXTURERENDERING_H_INCLUDED
#define DEFERREDTEXTURERENDERING_H_INCLUDED

#include <string>
#include <iostream>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/gl.h>

#include <OpenSG/OSGBackground.h>
#include <OpenSG/OSGSimpleStage.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTexGenChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>

#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGPassiveWindow.h>
#include <OpenSG/OSGViewport.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGSolidBackground.h>
#include <OpenSG/OSGFBOViewport.h>

#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGPerspectiveCamera.h>
#include <OpenSG/OSGDirectionalLight.h>
#include <OpenSG/OSGShaderProgram.h>
#include <OpenSG/OSGShaderProgramChunk.h>
#include <OpenSG/OSGDeferredShadingStage.h>

#define GLSL(shader) #shader

using namespace std;
using namespace OSG;

string DSAmbient_vp_glsl =
"#version 120\n"
GLSL(
void main(void) {
    gl_Position = ftransform();
}
);

string DSAmbient_fp_glsl =
"#version 120\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(
uniform sampler2DRect  texBufNorm;
uniform vec2           vpOffset;
void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    if(dot(norm, norm) < 0.95) discard;
    else gl_FragColor = vec4(0.02, 0.02, 0.02, 1.);
}
);

string DSGBuffer_vp_glsl =
"#version 120\n"
GLSL(
varying vec4 vertPos;
varying vec3 vertNorm;
void main(void) {
    vertPos        = gl_ModelViewMatrix * gl_Vertex;
    vertNorm       = gl_NormalMatrix    * gl_Normal;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FrontColor  = gl_Color;
    gl_Position    = ftransform();
}
);

string DSGBuffer_fp_glsl =
"#version 120\n"
GLSL(
varying vec4      vertPos;
varying vec3      vertNorm;
uniform sampler2D tex0;
float luminance(vec4 color) {
    return dot(color, vec4(0.3, 0.59, 0.11, 0.0));
}
void main(void) {
    vec3 pos = vertPos.xyz / vertPos.w;
    float ambVal  = luminance(gl_Color);
    vec3  diffCol = gl_FrontMaterial.diffuse.rgb;
    gl_FragData[0] = vec4(pos, ambVal);
    gl_FragData[1] = vec4(normalize(vertNorm), 0);
    gl_FragData[2] = vec4(diffCol, 0);
}
);

string DSDirLight_vp_glsl =
"#version 120\n"
GLSL(
void main(void) {
    gl_Position = ftransform();
}
);

string DSDirLight_fp_glsl =
"#version 120\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(
// compute directional light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computeDirLight(int index, vec3 pos, vec3 norm, vec4 mDiff) {
    vec4  color    = vec4(0., 0., 0., 0.);
    vec3  lightDir = gl_LightSource[index].position.xyz;
    float NdotL    = max(dot(norm, lightDir), 0.);
    if(NdotL > 0.) color = NdotL * mDiff * gl_LightSource[index].diffuse;
    return color;
}

// DS input buffers
uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;

// DS pass
void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    if(dot(norm, norm) < 0.95) discard;
    else {
        vec4  posAmb = texture2DRect(texBufPos,  lookup);
        vec3  pos    = posAmb.xyz;
        float amb    = posAmb.w;
        vec4  mDiff  = texture2DRect(texBufDiff, lookup);
        gl_FragColor = computeDirLight(0, pos, norm, mDiff);
    }
}
);

struct OsgTestScene2 {
    TransformUnrecPtr camBeacon;
    NodeUnrecPtr camBeaconNode;
    PerspectiveCameraUnrecPtr cam;
    SolidBackgroundUnrecPtr background;
    NodeUnrecPtr lightNode;
    DirectionalLightUnrecPtr light;

    NodeUnrecPtr dsStageN;
    DeferredShadingStageUnrecPtr dsStage;
    ShaderProgramUnrecPtr lightVP;
    ShaderProgramUnrecPtr lightFP;
    ShaderProgramChunkUnrecPtr lightSH;

    int fboWidth = 256;
    int fboHeight = 256;
    FrameBufferObjectRefPtr fbo;
    TextureObjChunkRefPtr   fboTex;
    ImageRefPtr             fboTexImg;
    TextureObjChunkRefPtr   fboDTex;
    ImageRefPtr             fboDTexImg;

    // render once ressources
    RenderActionRefPtr ract;
    WindowRecPtr win;
    ViewportRecPtr view;
    FBOViewportRecPtr fboView;

    GLXContext glc;
    Display* dpy;
    XID XWinID;

    OsgTestScene2() {
        cout << "OsgTestScene2" << endl;

        // camera
        camBeacon = Transform::create();
        camBeaconNode = makeNodeFor( camBeacon );
        cam = PerspectiveCamera::create();
        cam->setBeacon(camBeaconNode);
        cam->setFov   (osgDegree2Rad(90));
        cam->setNear  (0.1f);
        cam->setFar   (1000);

        // light
        light = DirectionalLight::create();
        lightNode = makeNodeFor(light);
        light->setAmbient  (.3f, .3f, .3f, 1);
        light->setDiffuse  ( 1,  1,  1, 1);
        light->setDirection( 0,  -1, 0   );
        light->setBeacon   (camBeaconNode);

        // background
        background = SolidBackground::create();
        background->setColor(Color3f(0,1,0));

        // scene
        NodeUnrecPtr torus = makeTorus(.5, 2, 16, 16);
        lightNode->addChild(camBeaconNode);
        lightNode->addChild(torus);

        Matrix m;
        m.setTranslate(Vec3f(0,0,5));
        camBeacon->setMatrix(m);


        // deferred stage
        ShaderProgramUnrecPtr      vpGBuffer = ShaderProgram::createVertexShader  ();
        ShaderProgramUnrecPtr      fpGBuffer = ShaderProgram::createFragmentShader();
        ShaderProgramUnrecPtr      vpAmbient = ShaderProgram::createVertexShader  ();
        ShaderProgramUnrecPtr      fpAmbient = ShaderProgram::createFragmentShader();
        ShaderProgramChunkUnrecPtr shGBuffer = ShaderProgramChunk::create();
        ShaderProgramChunkUnrecPtr shAmbient = ShaderProgramChunk::create();

        dsStage  = DeferredShadingStage::create();
        dsStageN = makeNodeFor(dsStage);
        //dsStageN = makeNodeFor(Group::create());
        dsStage->setCamera(cam);
        dsStage->setBackground(background);
        dsStageN->addChild(lightNode);

        dsStage->editMFPixelFormats()->push_back(Image::OSG_RGBA_PF);// positions (RGB) + ambient (A) term buffer
        dsStage->editMFPixelTypes  ()->push_back(Image::OSG_FLOAT32_IMAGEDATA);
        dsStage->editMFPixelFormats()->push_back(Image::OSG_RGB_PF); // normals (RGB) buffer
        dsStage->editMFPixelTypes  ()->push_back(Image::OSG_FLOAT32_IMAGEDATA);
        dsStage->editMFPixelFormats()->push_back(Image::OSG_RGB_PF); // diffuse (RGB) buffer
        dsStage->editMFPixelTypes  ()->push_back(Image::OSG_UINT8_IMAGEDATA);

        // G Buffer shader (one for the whole scene)
        vpGBuffer->setProgram(DSGBuffer_vp_glsl);
        fpGBuffer->setProgram(DSGBuffer_fp_glsl);
        fpGBuffer->addUniformVariable<Int32>("tex0", 0);
        shGBuffer->addShader(vpGBuffer);
        shGBuffer->addShader(fpGBuffer);
        dsStage->setGBufferProgram(shGBuffer);

        // ambient shader
        vpAmbient->setProgram(DSAmbient_vp_glsl);
        fpAmbient->setProgram(DSAmbient_fp_glsl);
        fpAmbient->addUniformVariable<Int32>("texBufNorm", 1);
        shAmbient->addShader(vpAmbient);
        shAmbient->addShader(fpAmbient);
        dsStage->setAmbientProgram(shAmbient);

        // ds light
        lightVP = ShaderProgram::createVertexShader();
        lightFP = ShaderProgram::createFragmentShader();
        lightSH = ShaderProgramChunk::create();
        lightVP->setProgram(DSDirLight_vp_glsl);
        lightFP->setProgram(DSDirLight_fp_glsl);
        lightFP->addUniformVariable<Int32>("texBufPos",  0);
        lightFP->addUniformVariable<Int32>("texBufNorm", 1);
        lightFP->addUniformVariable<Int32>("texBufDiff", 2);
        lightSH->addShader(lightVP);
        lightSH->addShader(lightFP);
        dsStage->editMFLights()->push_back(light);
        dsStage->editMFLightPrograms()->push_back(lightSH);



        fboTex = TextureObjChunk::create();
        fboTexImg = Image::create();
        fboTexImg->set(Image::OSG_RGB_PF, fboWidth, fboHeight);
        fboTex->setImage(fboTexImg);
        fboTex->setMinFilter(GL_NEAREST);
        fboTex->setMagFilter(GL_NEAREST);
        fboTex->setWrapS(GL_CLAMP_TO_EDGE);
        fboTex->setWrapT(GL_CLAMP_TO_EDGE);

        TextureBufferRefPtr texBuf = TextureBuffer::create();
        texBuf->setTexture(fboTex);

        fboDTexImg = Image::create();
        fboDTexImg->set(Image::OSG_RGB_PF, fboWidth, fboHeight);
        fboDTex = TextureObjChunk::create();
        fboDTex->setImage(fboDTexImg);
        fboDTex->setMinFilter(GL_NEAREST);
        fboDTex->setMagFilter(GL_NEAREST);
        fboDTex->setWrapS(GL_CLAMP_TO_EDGE);
        fboDTex->setWrapT(GL_CLAMP_TO_EDGE);
        fboDTex->setExternalFormat(GL_DEPTH_COMPONENT);
        fboDTex->setInternalFormat(GL_DEPTH_COMPONENT24); //24/32
        fboDTex->setCompareMode(GL_NONE);
        fboDTex->setCompareFunc(GL_LEQUAL);
        fboDTex->setDepthMode(GL_INTENSITY);
        TextureBufferRefPtr texDBuf = TextureBuffer::create();
        texDBuf->setTexture(fboDTex);

        RenderBufferRefPtr depthBuf = RenderBuffer::create();
        depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24);

        fbo = FrameBufferObject::create();
        fbo->setColorAttachment(texBuf, 0);
        //fbo->setColorAttachment(texDBuf, 1);
        fbo->setDepthAttachment(texDBuf); //HERE depthBuf/texDBuf
        fbo->editMFDrawBuffers()->clear();
        fbo->editMFDrawBuffers()->push_back(GL_DEPTH_ATTACHMENT_EXT);
        fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
        fbo->setWidth (fboWidth );
        fbo->setHeight(fboHeight);
        fbo->setPostProcessOnDeactivate(true);

        texBuf->setReadBack(true);
        texDBuf->setReadBack(true);

        ract = RenderAction::create();

        fboView = FBOViewport::create();
        fboView->setSize(0, 0, 1, 1);
        fboView->setFrameBufferObject(fbo); // replaces stage!
        fboView->setCamera(cam);
        fboView->setBackground(background);

        //fboView->setRoot(lightNode);
        fboView->setRoot(dsStageN);
    }

    void initPassiveWindow() {
        cout << " initPassiveWindow" << endl;
        dpy = XOpenDisplay(NULL);
        if ( !dpy ) { cout << "  Warning! could not connect to X" << endl; return; }
        XWinID = DefaultRootWindow(dpy);
        GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        XVisualInfo* vi = glXChooseVisual(dpy, 0, att);
        if ( !vi ) { cout << "  Warning! could not get visual" << endl; return; }
        glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
        glXMakeCurrent(dpy, XWinID, glc);

        PassiveWindowRecPtr pwin = PassiveWindow::create();
        pwin->init();
        pwin->addPort(fboView);
        pwin->setSize(fboWidth, fboHeight);
        win = pwin;
        glXMakeCurrent(dpy, XWinID, glc);
    }

    void initGlutWindow() {
        cout << " initGlutWindow" << endl;
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        GLUTWindowRecPtr gwin = GLUTWindow::create();
        glutInitWindowSize(fboWidth, fboHeight);
        int winID = glutCreateWindow("PolyVR");
        gwin->setGlutId(winID);
        gwin->addPort(fboView);
        gwin->setSize(fboWidth, fboHeight);
        gwin->init();
        win = gwin;
    }

    void renderOnce(string path) {
        cout << " renderOnce to " << path << endl;
        if (!win) { cout << "  Warning! window invalid!" << endl; return; }
        win->render(ract);
        ImageMTRecPtr img = Image::create();
        img->set( fboTexImg );
        img->write(path.c_str());
    }
};

void runDeferredTextureRenderingTest(int argc, char **argv) {
    glutInit(&argc, argv);
    ChangeList::setReadWriteDefault();
    preloadSharedObject("OSGFileIO");
    preloadSharedObject("OSGImageFileIO");
    osgInit(argc,argv);

    OsgTestScene2 t;
    //t.initPassiveWindow();
    t.initGlutWindow();
    t.renderOnce("A.png");
    t.renderOnce("B.png");
    t.renderOnce("C.png");
}


#endif // DEFERREDTEXTURERENDERING_H_INCLUDED
