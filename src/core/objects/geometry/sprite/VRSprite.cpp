#include "VRSprite.h"

#include "VRSpriteResizeTool.h"
#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif
#include "core/objects/geometry/VRGeoData.h"
#include "core/tools/VRAnnotationEngine.h"
#include "core/tools/VRGeoPrimitive.h"
#include "core/setup/VRSetup.h"
#ifndef WITHOUT_CEF
#include "addons/CEF/CEF.h"
#endif
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <sstream>

#define GLSL(shader) #shader

using namespace OSG;


VRSprite::VRSprite (string name, bool alpha, float w, float h) : VRGeometry(name) {
    width = w;
    height = h;
    type = "Sprite";
}

VRSprite::~VRSprite() {}

VRSpritePtr VRSprite::create(string name, bool alpha, float w, float h) {
    auto s = shared_ptr<VRSprite>(new VRSprite(name, alpha, w, h) );
    s->updateGeo();
    return s;
}

VRSpritePtr VRSprite::ptr() { return static_pointer_cast<VRSprite>( shared_from_this() ); }

void VRSprite::updateGeo() {
    VRGeoData data;
    float w2 = width*0.5;
    float h2 = height*0.5;
    Vec4d uvs = Vec4d(0,1,1,0);
    if (web) uvs = Vec4d(0,1,0,1);
    data.pushVert(Pnt3d(-w2,h2,0), Vec3d(0,0,1), Vec2d(uvs[0],uvs[2]));
    data.pushVert(Pnt3d(-w2,-h2,0), Vec3d(0,0,1), Vec2d(uvs[0],uvs[3]));
    data.pushVert(Pnt3d(w2,-h2,0), Vec3d(0,0,1), Vec2d(uvs[1],uvs[3]));
    data.pushVert(Pnt3d(w2,h2,0), Vec3d(0,0,1), Vec2d(uvs[1],uvs[2]));
    data.pushQuad();
    if (doubleSided) {
        double e = -1e-3;
        data.pushVert(Pnt3d(w2,h2,e), Vec3d(0,0,-1), Vec2d(uvs[0],uvs[2]));
        data.pushVert(Pnt3d(w2,-h2,e), Vec3d(0,0,-1), Vec2d(uvs[0],uvs[3]));
        data.pushVert(Pnt3d(-w2,-h2,e), Vec3d(0,0,-1), Vec2d(uvs[1],uvs[3]));
        data.pushVert(Pnt3d(-w2,h2,e), Vec3d(0,0,-1), Vec2d(uvs[1],uvs[2]));
        data.pushQuad();
        getMaterial()->setFrontBackModes(GL_FILL, GL_NONE);
    }
    data.apply(ptr());

    if (usingShaders) { // update billboard data
        GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();
        GeoVectorPropertyMTRecPtr pos = getMesh()->geo->getPositions();
        for (int i = 0; i < 4; i++) {
            Pnt3f p = pos->getValue<Pnt3f>(i);
            tcs->addValue(Vec2f(p[0], p[1]));
            pos->setValue(Pnt3f(), i);
        }
        setTexCoords(tcs, 1);
    }
}

VRTexturePtr VRSprite::setText(string l, float res, Color4f c1, Color4f c2, int ol, Color4f oc, string font) {
    label = l;
    auto m = VRMaterial::create(getName()+"label");
    auto tex = VRText::get()->create(l, font, res, 3, c1, c2, ol, oc);
    m->setTexture(tex);
    setMaterial(m);
    return tex;
}

void VRSprite::webOpen(string path, int res, float ratio) {
#ifndef WITHOUT_CEF
    VRMaterialPtr mat = VRMaterial::get(getName()+"web");
    setMaterial(mat);
    mat->setLit(false);
    web = CEF::create();

    web->setMaterial(mat);
    web->open(path);

    auto setup = VRSetup::getCurrent();
    if (setup) {
        VRDevicePtr mouse = setup->getDevice("mouse");
        VRDevicePtr keyboard = setup->getDevice("keyboard");
        VRDevicePtr flystick = setup->getDevice("flystick");
        VRDevicePtr multitouch = setup->getDevice("multitouch");

        if (mouse) web->addMouse(mouse, ptr(), 0, 2, 3, 4);
        if (flystick) web->addMouse(flystick, ptr(), 0, -1, -1, -1);
        if (multitouch) web->addMouse(multitouch, ptr(), 0, 2, 3, 4);
        if (keyboard) web->addKeyboard(keyboard);
    }

    web->setResolution(res);
    web->setAspectRatio(ratio);
    updateGeo();
#endif
}

void VRSprite::setTexture(string path) {
    auto m = VRMaterial::create(path);
    m->setTexture(path);
    setMaterial(m);
}

string VRSprite::getLabel() { return label; }
Vec2d VRSprite::getSize() { return Vec2d(width, height); }

void VRSprite::setSize(float w, float h) {
    width = w;
    height = h;
    updateGeo();
}

void VRSprite::setDoubleSided(bool b) {
    doubleSided = true;
    updateGeo();
}

void VRSprite::setBillboard(int i) {
    auto mat = getMaterial();

    if (!usingShaders) {
        mat->setVertexShader(spriteShaderVP, "spriteShaderVP");
        mat->setFragmentShader(spriteShaderFP, "spriteShaderFP");

        GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();
        GeoVectorPropertyMTRecPtr pos = getMesh()->geo->getPositions();
        for (int i=0; i<4; i++) {
            Pnt3f p = pos->getValue<Pnt3f>(i);
            tcs->addValue(Vec2f(p[0], p[1]));
            pos->setValue(Pnt3f(), i);
        }
        setTexCoords(tcs, 1);
        usingShaders = true;
    }

    mat->setShaderParameter("doBillboard", Real32(i));
}

void VRSprite::setScreensize(bool b) {
    auto mat = getMaterial();

    if (!usingShaders) {
        mat->setVertexShader(spriteShaderVP, "spriteShaderVP");
        mat->setFragmentShader(spriteShaderFP, "spriteShaderFP");

        GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();
        GeoVectorPropertyMTRecPtr pos = getMesh()->geo->getPositions();
        for (int i=0; i<4; i++) {
            Pnt3f p = pos->getValue<Pnt3f>(i);
            tcs->addValue(Vec2f(p[0], p[1]));
            pos->setValue(Pnt3f(), i);
        }
        setTexCoords(tcs, 1);
        usingShaders = true;
    }

    mat->setShaderParameter("doScreensize", Real32(b));
}

void VRSprite::showResizeTool(bool b, float size, bool doAnnotations) {
    if (!b && resizeTool) resizeTool->select(false);
    if (b) {
        if (!resizeTool) {
            setPrimitive("Plane "+toString(width)+" "+toString(height)+" 1 1");
            resizeTool = VRGeoPrimitive::create();
            resizeTool->setGeometry(ptr());
            addChild(resizeTool);
        }
        resizeTool->select(true);
        resizeTool->setHandleSize(size);
        resizeTool->getLabels()->setVisible(doAnnotations);
    }
}

void VRSprite::convertToCloth() {
#ifndef WITHOUT_BULLET
    getPhysics()->setDynamic(true);
    getPhysics()->setShape("Cloth");
    getPhysics()->setSoft(true);
    getPhysics()->setPhysicalized(true);
#endif
}



string VRSprite::spriteShaderVP =
GLSL(
varying vec2 texCoord;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
attribute vec2 osg_MultiTexCoord1;

uniform float doBillboard;
uniform float doScreensize;
uniform vec2 OSGViewportSize;

\n#ifdef __EMSCRIPTEN__\n
uniform mat4 OSGModelViewProjectionMatrix;
uniform mat4 OSGModelViewMatrix;
uniform mat4 OSGProjectionMatrix;
\n#endif\n

void main( void ) {
    if (doBillboard < 0.5) {
		if (doScreensize < 0.5) {
			vec4 v = osg_Vertex;
			v.xy += osg_MultiTexCoord1.xy;
\n#ifdef __EMSCRIPTEN__\n
			gl_Position = OSGModelViewProjectionMatrix * v;
\n#else\n
			gl_Position = gl_ModelViewProjectionMatrix * v;
\n#endif\n
		} else {
			vec4 v = osg_Vertex;
\n#ifdef __EMSCRIPTEN__\n
			vec4 k = OSGModelViewProjectionMatrix * v;
\n#else\n
			vec4 k = gl_ModelViewProjectionMatrix * v;
\n#endif\n
		    k.xyz = k.xyz/k.w;
		    k.w = 1.0;
    		float a = OSGViewportSize.y/OSGViewportSize.x;
    		vec4 d = vec4(osg_MultiTexCoord1.x*a, osg_MultiTexCoord1.y,0,0);
\n#ifdef __EMSCRIPTEN__\n
		    gl_Position = k + OSGModelViewProjectionMatrix * d * 0.1;
\n#else\n
		    gl_Position = k + gl_ModelViewProjectionMatrix * d * 0.1;
\n#endif\n
        }
    } else if (doBillboard < 1.5) {
        float a = OSGViewportSize.y/OSGViewportSize.x;
        vec4 d = vec4(osg_MultiTexCoord1.x*a, osg_MultiTexCoord1.y,0,0);
        vec4 v = osg_Vertex;
\n#ifdef __EMSCRIPTEN__\n
		vec4 k = OSGModelViewProjectionMatrix * v;
\n#else\n
		vec4 k = gl_ModelViewProjectionMatrix * v;
\n#endif\n
		if (doScreensize > 0.5) {
    		d *= 0.1;
		    k.xyz = k.xyz/k.w;
		    k.w = 1.0;
        }
    	gl_Position = k + d;
    } else {
	    float a = OSGViewportSize.y/OSGViewportSize.x;
	    vec4 d = vec4(osg_MultiTexCoord1.x*a, 0,0,0);
        vec4 v = osg_Vertex;
        vec4 k;
		if (doScreensize > 0.5) {
    		d *= 0.1;
\n#ifdef __EMSCRIPTEN__\n
			k = OSGModelViewProjectionMatrix * v;
\n#else\n
			k = gl_ModelViewProjectionMatrix * v;
\n#endif\n
		    k.xyz = k.xyz/k.w;
		    k.w = 1.0;
\n#ifdef __EMSCRIPTEN__\n
		    k += OSGModelViewProjectionMatrix * vec4(0, osg_MultiTexCoord1.y, 0, 0) * 0.1;
\n#else\n
		    k += gl_ModelViewProjectionMatrix * vec4(0, osg_MultiTexCoord1.y, 0, 0) * 0.1;
\n#endif\n
		} else {
			v.y += osg_MultiTexCoord1.y;
\n#ifdef __EMSCRIPTEN__\n
			k = OSGModelViewProjectionMatrix * v;
\n#else\n
			k = gl_ModelViewProjectionMatrix * v;
\n#endif\n
		}
    	gl_Position = k + d;
    }
    texCoord = osg_MultiTexCoord0;
}
);


string VRSprite::spriteShaderFP =
GLSL(
\n#ifdef __EMSCRIPTEN__\n
precision mediump float;
\n#endif\n
uniform sampler2D texture;

varying vec2 texCoord;

void main( void ) {
    //gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //gl_FragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);
    gl_FragColor = texture2D(texture, texCoord);
}
);
