#include "VRAnalyticGeometry.h"
#include "VRAnnotationEngine.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

#define GLSL(shader) #shader

using namespace OSG;

VRAnalyticGeometry::VRAnalyticGeometry(string name) : VRTransform(name) {
    ae = VRAnnotationEngine::create();
    vectorLinesGeometry = VRGeometry::create("AGLines");
    vectorEndsGeometry = VRGeometry::create("AGPoints");

    vecMat = VRMaterial::create("AnalyticGeometry");
    vecMat->setLit(false);
    vecMat->setLineWidth(3);
    vecMat->setDepthTest(GL_ALWAYS);
    vecMat->setSortKey(100);
    if (vectorLinesGeometry) vectorLinesGeometry->setMaterial(vecMat);

    pntMat = VRMaterial::create("AnalyticGeometry2");
    pntMat->setLit(false);
    pntMat->setPointSize(11);
    pntMat->setDepthTest(GL_ALWAYS);
    pntMat->setSortKey(100);
    if (vectorEndsGeometry) vectorEndsGeometry->setMaterial(pntMat);

    cirMat = VRMaterial::create("AnalyticGeometry3");
    cirMat->setLit(false);
    cirMat->setVertexShader(circle_vp, "analyticCircleVS");
    cirMat->setFragmentShader(circle_fp, "analyticCircleFS");
    cirMat->setDepthTest(GL_ALWAYS);
    cirMat->setSortKey(100);
}

VRAnalyticGeometry::~VRAnalyticGeometry() {}

VRAnalyticGeometryPtr VRAnalyticGeometry::ptr() { return static_pointer_cast<VRAnalyticGeometry>( shared_from_this() ); }
VRAnalyticGeometryPtr VRAnalyticGeometry::create(string name)  {
    auto ptr = shared_ptr<VRAnalyticGeometry>( new VRAnalyticGeometry(name) );
    ptr->init();
    return ptr;
}

void VRAnalyticGeometry::init() {
    addChild(vectorLinesGeometry);
    addChild(vectorEndsGeometry);
    addChild(ae);

    if (ae) {
        ae->getMaterial()->setDepthTest(GL_ALWAYS);
        ae->getMaterial()->setLit(0);
    }

    // lines
    if (vectorLinesGeometry) {
        GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
        GeoVec3fPropertyMTRecPtr cols = GeoVec3fProperty::create();
        GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
        lengths->addValue(0);

        vectorLinesGeometry->setType(GL_LINES);
        vectorLinesGeometry->setPositions(pos);
        vectorLinesGeometry->setColors(cols);
        vectorLinesGeometry->setLengths(lengths);
    }

    // ends
    if (vectorEndsGeometry) {
        GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
        GeoVec3fPropertyMTRecPtr cols = GeoVec3fProperty::create();
        GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
        lengths->addValue(0);

        vectorEndsGeometry->setType(GL_POINTS);
        vectorEndsGeometry->setPositions(pos);
        vectorEndsGeometry->setColors(cols);
        vectorEndsGeometry->setLengths(lengths);
    }

    // circles
    if (circlesGeometry) {
        GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
        GeoVec3fPropertyMTRecPtr cols = GeoVec3fProperty::create();
        GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
        lengths->addValue(0);
        GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();
        GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();

        circlesGeometry->setType(GL_QUADS);
        circlesGeometry->setPositions(pos);
        circlesGeometry->setColors(cols);
        circlesGeometry->setNormals(norms);
        circlesGeometry->setTexCoords(tcs);
        circlesGeometry->setLengths(lengths);
    }
}

VRAnnotationEnginePtr VRAnalyticGeometry::getAnnotationEngine() { return ae; }

void VRAnalyticGeometry::setLabelParams(float size, bool screen_size, bool billboard, Color4f fg, Color4f bg, Vec3d offset) {
    if (ae) {
        ae->setSize(size);
        ae->setBillboard(billboard);
        ae->setScreensize(screen_size);
        ae->setColor(fg);
        ae->setBackground(bg);
    }
    labelOffset = offset;
}

void VRAnalyticGeometry::resize(int i, int j, int k) {
    if (vectorLinesGeometry) {
        auto pos = vectorLinesGeometry->getMesh()->geo->getPositions();
        auto cols = vectorLinesGeometry->getMesh()->geo->getColors();
        auto lengths = vectorLinesGeometry->getMesh()->geo->getLengths();
        while (i >= (int)pos->size()) {
            pos->addValue(Pnt3d());
            cols->addValue(Vec3d());
            lengths->setValue(pos->size(), 0);
        }
    }

    if (vectorEndsGeometry) {
        auto pos = vectorEndsGeometry->getMesh()->geo->getPositions();
        auto cols = vectorEndsGeometry->getMesh()->geo->getColors();
        auto lengths = vectorEndsGeometry->getMesh()->geo->getLengths();
        while (j >= (int)pos->size()) {
            pos->addValue(Pnt3d());
            cols->addValue(Vec3d());
            lengths->setValue(pos->size(), 0);
        }
    }

    if (circlesGeometry) {
        auto pos = circlesGeometry->getMesh()->geo->getPositions();
        auto cols = circlesGeometry->getMesh()->geo->getColors();
        auto lengths = circlesGeometry->getMesh()->geo->getLengths();
        auto norms = circlesGeometry->getMesh()->geo->getNormals();
        auto tcs = circlesGeometry->getMesh()->geo->getTexCoords();
        while (k >= (int)pos->size()) {
            pos->addValue(Pnt3d());
            cols->addValue(Vec3d());
            norms->addValue(Vec3d());
            tcs->addValue(Vec2d());
            lengths->setValue(pos->size(), 0);
        }
    }
}

void VRAnalyticGeometry::setAngle(int i, Vec3d p, Vec3d v1, Vec3d v2, Color3f c1, Color3f c2, string label) {
    if (!vectorLinesGeometry) return;
    if (ae) ae->set(i, labelOffset+p+v1*0.1+v2*0.1, label);
    resize(2*i+1);

    // line
    auto pos = vectorLinesGeometry->getMesh()->geo->getPositions();
    auto cols = vectorLinesGeometry->getMesh()->geo->getColors();
    pos->setValue(p+v1*0.1, 2*i);
    pos->setValue(p+v2*0.1, 2*i+1);
    cols->setValue(c1, 2*i);
    cols->setValue(c2, 2*i+1);
}

void VRAnalyticGeometry::setCircle(int i, Vec3d p, Vec3d n, float r, Color3f color, string label) {
    if (!circlesGeometry) {
        circlesGeometry = VRGeometry::create("AGCircles"); // TODO: segfault prone :(
        circlesGeometry->setMaterial(cirMat);
        addChild(circlesGeometry);

        GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
        GeoVec3fPropertyMTRecPtr cols = GeoVec3fProperty::create();
        GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
        lengths->addValue(0);
        GeoVec2fPropertyMTRecPtr tcs = GeoVec2fProperty::create();
        GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();

        circlesGeometry->setType(GL_QUADS);
        circlesGeometry->setPositions(pos);
        circlesGeometry->setColors(cols);
        circlesGeometry->setNormals(norms);
        circlesGeometry->setTexCoords(tcs);
        circlesGeometry->setLengths(lengths);
    }

    if (!circlesGeometry) return;
    if (ae && label != "") ae->set(i, labelOffset+p, label);
    resize(-1, -1, 4*i+3);

    Vec3d v1 = Vec3d(0, -n[2], n[1]);
    if (abs(n[1]) <= abs(n[0]) && abs(n[1]) <= abs(n[2])) v1 = Vec3d(-n[2], 0, n[0]);
    if (abs(n[2]) <= abs(n[0]) && abs(n[2]) <= abs(n[1])) v1 = Vec3d(-n[1], n[0], 0);
    Vec3d v2 = v1.cross(n);
    v1.normalize();
    v2.normalize();
    v1 *= r; v2 *= r;

    Vec3d norm(r, 0, 0);

    // quad
    auto pos = circlesGeometry->getMesh()->geo->getPositions();
    auto cols = circlesGeometry->getMesh()->geo->getColors();
    auto norms = circlesGeometry->getMesh()->geo->getNormals();
    auto tcs = circlesGeometry->getMesh()->geo->getTexCoords();
    pos->setValue(p+v1+v2, 4*i);
    pos->setValue(p-v1+v2, 4*i+1);
    pos->setValue(p-v1-v2, 4*i+2);
    pos->setValue(p+v1-v2, 4*i+3);
    cols->setValue(color, 4*i);
    cols->setValue(color, 4*i+1);
    cols->setValue(color, 4*i+2);
    cols->setValue(color, 4*i+3);
    tcs->setValue(Vec2d(1,1), 4*i);
    tcs->setValue(Vec2d(-1,1), 4*i+1);
    tcs->setValue(Vec2d(-1,-1), 4*i+2);
    tcs->setValue(Vec2d(1,-1), 4*i+3);
    norms->setValue(norm, 4*i);
    norms->setValue(norm, 4*i+1);
    norms->setValue(norm, 4*i+2);
    norms->setValue(norm, 4*i+3);
}

void VRAnalyticGeometry::setVector(int i, Vec3d p, Vec3d vec, Color3f color, string label, bool doArrow) {
    if (!vectorLinesGeometry) return;
    if (ae) ae->set(i, labelOffset+p+vec*0.5, label);
    resize(2*i+1, i);

    // line
    auto pos = vectorLinesGeometry->getMesh()->geo->getPositions();
    auto cols = vectorLinesGeometry->getMesh()->geo->getColors();
    pos->setValue(p, 2*i);
    pos->setValue(p+vec, 2*i+1);
    cols->setValue(color, 2*i);
    cols->setValue(color, 2*i+1);

    // end
    if (!vectorEndsGeometry) return;
    pos = vectorEndsGeometry->getMesh()->geo->getPositions();
    cols = vectorEndsGeometry->getMesh()->geo->getColors();
    if (doArrow) {
        pos->setValue(p+vec, i);
        cols->setValue(color, i);
    } else { // TODO
        pos->setValue(Vec3d(0,-10000,0), i);
        cols->setValue(color, i);
    }
}

int VRAnalyticGeometry::addVector(Vec3d pos, Vec3d vec, Color3f color, string label, bool doArrow) {
    int i = vectorEndsGeometry->getMesh()->geo->getPositions()->size();
    setVector(i, pos, vec, color, label, doArrow);
    return i;
}

void VRAnalyticGeometry::clear() {
    if (ae) ae->clear();
    init();
}

void VRAnalyticGeometry::setZOffset(float factor, float bias) {
    cirMat->setZOffset(factor, bias);
    pntMat->setZOffset(factor, bias);
    vecMat->setZOffset(factor, bias);
}


string VRAnalyticGeometry::circle_vp =
"#version 120\n"
GLSL(
uniform vec2 OSGViewportSize;
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec3 osg_Color;
attribute vec2 osg_MultiTexCoord0;
varying vec2 texCoord;
varying vec2 pixelsize;
varying vec3 normal;
varying vec3 color;
varying vec2 ang;
void main() {
	gl_Position = ftransform();
	normal = osg_Normal;
	texCoord = osg_MultiTexCoord0.xy;
	color = osg_Color;
	pixelsize.xy = 1.0/OSGViewportSize.xy;
}
);

string VRAnalyticGeometry::circle_fp =
"#version 120\n"
GLSL(
varying vec2 texCoord;
varying vec2 ang;
varying vec2 pixelsize;
varying vec3 normal;
varying vec3 color;
void main() {
	float r = dot(texCoord, texCoord);
	if (r > 1.0) discard;

    float R = normal.x*r;
    float w = normal.x-R;
	if (r < 0.9) discard;

	//float width = dot(pixelsize, abs(texCoord));
	//if (r < 1.0 - 3.0*width) discard;

	gl_FragDepth = 0;
	gl_FragColor = vec4(color, 1);

	/*if (r < 1.0 - 1.5*width)
		gl_FragColor[3] = smoothstep(1.0 - 3.0*width, 1.0 - 1.5*width, r);
	if (r > 1.0 - 1.5*width)
		gl_FragColor[3] = 1.0 - smoothstep(1.0 - 1.5*width, 1.0, r);*/
}
);


