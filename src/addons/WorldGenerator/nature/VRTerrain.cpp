#include "VRTerrain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/utils/VRFunction.h"
#include "core/math/boundingbox.h"

#include <OpenSG/OSGIntersectAction.h>

#define GLSL(shader) #shader

using namespace OSG;

VRTerrain::VRTerrain(string name) : VRGeometry(name) { setupMat(); }
VRTerrain::~VRTerrain() {}
VRTerrainPtr VRTerrain::create(string name) { return VRTerrainPtr( new VRTerrain(name) ); }

void VRTerrain::setParameters( Vec2f s, float r ) {
    size = s;
    resolution = r;
    grid = r*64;
    setupGeo();
    mat->setShaderParameter("resolution", resolution);
    updateTexelSize();
}

void VRTerrain::setMap( VRTexturePtr t ) {
    tex = t;
    mat->setTexture(t);
    updateTexelSize();
}

void VRTerrain::updateTexelSize() {
    Vec3i s = tex->getSize();
    texelSize[0] = size[0]/s[0];
    texelSize[1] = size[1]/s[1];
	mat->setShaderParameter("texelSize", texelSize);
}

void VRTerrain::setupGeo() {
    Vec2i gridN = Vec2i(size*1.0/grid);
    if (gridN[0] < 1) gridN[0] = 1;
    if (gridN[1] < 1) gridN[1] = 1;
    Vec2f gridS = size;
    gridS[0] /= gridN[0];
    gridS[1] /= gridN[1];
	Vec2f tcChunk = Vec2f(1.0/gridN[0], 1.0/gridN[1]);

	VRGeoData geo;
    for (int i =0; i < gridN[0]; i++) {
		float px1 = -size[0]*0.5 + i*gridS[0];
		float px2 = px1 + gridS[0];
		float tcx1 = 0 + i*tcChunk[0];
		float tcx2 = tcx1 + tcChunk[0];

		for (int j =0; j < gridN[1]; j++) {
			float py1 = -size[1]*0.5 + j*gridS[1];
			float py2 = py1 + gridS[1];
			float tcy1 = 0 + j*tcChunk[1];
			float tcy2 = tcy1 + tcChunk[1];

			geo.pushVert(Vec3f(px1,0,py1), Vec3f(0,1,0), Vec2f(tcx1,tcy1));
			geo.pushVert(Vec3f(px1,0,py2), Vec3f(0,1,0), Vec2f(tcx1,tcy2));
			geo.pushVert(Vec3f(px2,0,py2), Vec3f(0,1,0), Vec2f(tcx2,tcy2));
			geo.pushVert(Vec3f(px2,0,py1), Vec3f(0,1,0), Vec2f(tcx2,tcy1));
			geo.pushQuad();
		}
	}

	geo.apply(ptr());
	setType(GL_PATCHES);
	setPatchVertices(4);
	setMaterial(mat);
}

#include <BulletCollision/CollisionShapes/btCollisionMargin.h>
#include <BulletCollision/CollisionShapes/btPolyhedralConvexShape.h>
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btMinMax.h>

ATTRIBUTE_ALIGNED16(class) VRTerrainShape: public btPolyhedralConvexShape {
    private:
        VRTerrain* terrain = 0;

    public:
        BT_DECLARE_ALIGNED_ALLOCATOR();

        VRTerrainShape( VRTerrain* terrain ) : btPolyhedralConvexShape(), terrain(terrain) {
            btVector3 boxHalfExtents(10,1,10);
            m_shapeType = BOX_SHAPE_PROXYTYPE;
            setSafeMargin(boxHalfExtents);
            btVector3 margin(getMargin(),getMargin(),getMargin());
            m_implicitShapeDimensions = (boxHalfExtents * m_localScaling) - margin;
        };

        btVector3 getHalfExtentsWithMargin() const {
            btVector3 halfExtents = getHalfExtentsWithoutMargin();
            btVector3 margin(getMargin(),getMargin(),getMargin());
            halfExtents += margin;
            return halfExtents;
        }

        const btVector3& getHalfExtentsWithoutMargin() const {
            return m_implicitShapeDimensions;
        }

        btVector3 toSupportingVector(const btVector3& vec, const btVector3& halfExtents) const {
            return btVector3(btFsels(vec.x(), halfExtents.x(), -halfExtents.x()),
                btFsels(vec.y(), halfExtents.y(), -halfExtents.y()),
                btFsels(vec.z(), halfExtents.z(), -halfExtents.z()));
        }

        SIMD_FORCE_INLINE btVector3 localGetSupportingVertexWithoutMargin(const btVector3& vec) const {
            const btVector3& halfExtents = getHalfExtentsWithoutMargin();
            return toSupportingVector(vec, halfExtents);
        }

        virtual btVector3 localGetSupportingVertex(const btVector3& vec) const {
            const btVector3& halfExtents = getHalfExtentsWithMargin();
            return toSupportingVector(vec, halfExtents);
        }

        virtual void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors,btVector3* supportVerticesOut,int numVectors) const {
            const btVector3& halfExtents = getHalfExtentsWithoutMargin();
            for (int i=0;i<numVectors;i++) {
                const btVector3& vec = vectors[i];
                supportVerticesOut[i] = toSupportingVector(vec, halfExtents);
            }
        }

        virtual void setMargin(btScalar collisionMargin) {
            btVector3 oldMargin(getMargin(),getMargin(),getMargin());
            btVector3 implicitShapeDimensionsWithMargin = m_implicitShapeDimensions+oldMargin;
            btConvexInternalShape::setMargin(collisionMargin);
            btVector3 newMargin(getMargin(),getMargin(),getMargin());
            m_implicitShapeDimensions = implicitShapeDimensionsWithMargin - newMargin;
        }

        virtual void setLocalScaling(const btVector3& scaling) {
            btVector3 oldMargin(getMargin(),getMargin(),getMargin());
            btVector3 implicitShapeDimensionsWithMargin = m_implicitShapeDimensions+oldMargin;
            btVector3 unScaledImplicitShapeDimensionsWithMargin = implicitShapeDimensionsWithMargin / m_localScaling;
            btConvexInternalShape::setLocalScaling(scaling);
            m_implicitShapeDimensions = (unScaledImplicitShapeDimensionsWithMargin * m_localScaling) - oldMargin;
        }

        virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const {
            btTransformAabb(getHalfExtentsWithoutMargin(),getMargin(),t,aabbMin,aabbMax);
        }

        virtual void calculateLocalInertia(btScalar mass,btVector3& inertia) const {
            btVector3 halfExtents = getHalfExtentsWithMargin() * 2;
            btScalar x = halfExtents.x();
            btScalar y = halfExtents.y();
            btScalar z = halfExtents.z();
            inertia = btVector3( y*y + z*z, x*x + z*z, x*x + y*y) * (mass/12.0);
        }

        virtual void getPlane(btVector3& planeNormal,btVector3& planeSupport,int i ) const {
            btVector4 plane ;
            getPlaneEquation(plane,i);
            planeNormal = btVector3(plane.getX(),plane.getY(),plane.getZ());
            planeSupport = localGetSupportingVertex(-planeNormal);
        }

        virtual int getNumPlanes() const { return 6; }
        virtual int	getNumVertices() const { return 8; }
        virtual int getNumEdges() const { return 12; }

        virtual void getVertex(int i,btVector3& vtx) const {
            btVector3 halfExtents = getHalfExtentsWithMargin();
            vtx = btVector3(
                    halfExtents.x() * (1-(i&1)) - halfExtents.x() * (i&1),
                    halfExtents.y() * (1-((i&2)>>1)) - halfExtents.y() * ((i&2)>>1),
                    halfExtents.z() * (1-((i&4)>>2)) - halfExtents.z() * ((i&4)>>2));
        }

        virtual void getPlaneEquation(btVector4& plane,int i) const {
            btVector3 halfExtents = getHalfExtentsWithoutMargin();
            plane = btVector4(0,0,0,-halfExtents[i/2]);
            plane[i/2] = 1-2*i%2; // -1 or 1
        }

        virtual void getEdge(int i,btVector3& pa,btVector3& pb) const {
            int edgeVert0 = 0;
            int edgeVert1 = 0;

            switch (i) {
                case 0:
                    edgeVert0 = 0;
                    edgeVert1 = 1;
                    break;
                case 1:
                    edgeVert0 = 0;
                    edgeVert1 = 2;
                    break;
                case 2:
                    edgeVert0 = 1;
                    edgeVert1 = 3;
                    break;
                case 3:
                    edgeVert0 = 2;
                    edgeVert1 = 3;
                    break;
                case 4:
                    edgeVert0 = 0;
                    edgeVert1 = 4;
                    break;
                case 5:
                    edgeVert0 = 1;
                    edgeVert1 = 5;
                    break;
                case 6:
                    edgeVert0 = 2;
                    edgeVert1 = 6;
                    break;
                case 7:
                    edgeVert0 = 3;
                    edgeVert1 = 7;
                    break;
                case 8:
                    edgeVert0 = 4;
                    edgeVert1 = 5;
                    break;
                case 9:
                    edgeVert0 = 4;
                    edgeVert1 = 6;
                    break;
                case 10:
                    edgeVert0 = 5;
                    edgeVert1 = 7;
                    break;
                case 11:
                    edgeVert0 = 6;
                    edgeVert1 = 7;
                    break;
                default:
                    btAssert(0);
            }

            getVertex(edgeVert0,pa );
            getVertex(edgeVert1,pb );
        }

        virtual	bool isInside(const btVector3& pt,btScalar tolerance) const {
            btVector3 halfExtents = getHalfExtentsWithoutMargin();

            bool result =	(pt.x() <= (halfExtents.x()+tolerance)) &&
                            (pt.x() >= (-halfExtents.x()-tolerance)) &&
                            (pt.y() <= (halfExtents.y()+tolerance)) &&
                            (pt.y() >= (-halfExtents.y()-tolerance)) &&
                            (pt.z() <= (halfExtents.z()+tolerance)) &&
                            (pt.z() >= (-halfExtents.z()-tolerance));

            return result;
        }

        virtual const char*	getName() const { return "Terrain"; }

        virtual int getNumPreferredPenetrationDirections() const { return 6; }

        virtual void getPreferredPenetrationDirection(int i, btVector3& penetrationVector) const {
            penetrationVector = btVector3(0,0,0);
            penetrationVector[i/2] = 1-2*i%2; // -1 or 1
        }
};

void VRTerrain::physicalize(bool b) {
    auto shape = new VRTerrainShape(this);
    getPhysics()->setCustomShape( shape );
    getPhysics()->setPhysicalized(true);
}

void VRTerrain::setupMat() {
    Vec4f w = Vec4f(1,1,1,1);
    VRTextureGenerator tg;
	tg.setSize(Vec3i(400,400,1),true);
	tg.add("Perlin", 1.0, w*0.97, w);
	tg.add("Perlin", 1.0/2, w*0.95, w);
	tg.add("Perlin", 1.0/4, w*0.85, w);
	tg.add("Perlin", 1.0/8, w*0.8, w);
	tg.add("Perlin", 1.0/16, w*0.7, w);
	tg.add("Perlin", 1.0/32, w*0.5, w);
	tex = tg.compose(0);

	mat = VRMaterial::create("terrain");
	mat->setWireFrame(0);
	mat->setLineWidth(1);
	mat->setVertexShader(vertexShader, "terrainVS");
	mat->setFragmentShader(fragmentShader, "terrainFS");
	mat->setTessControlShader(tessControlShader, "terrainTCS");
	mat->setTessEvaluationShader(tessEvaluationShader, "terrainTES");
	mat->setShaderParameter("resolution", resolution);
    updateTexelSize();
	mat->setShaderParameter("texelSize", texelSize);
	mat->setTexture(tex);
}

bool VRTerrain::applyIntersectionAction(Action* action) {
    if (!mesh || !mesh->geo) return false;

    auto tex = getMaterial()->getTexture();

    auto clamp = [&](float x, float a, float b) {
        return max(min(x,b),a);
    };

    auto toUV = [&](Pnt3f p) -> Vec2f {
        auto mw = getWorldMatrix();
        mw.invert();
        //mw.mult(p,p); // transform point in local coords
		float u = clamp(p[0]/size[0] + 0.5, 0, 1);
		float v = clamp(p[2]/size[1] + 0.5, 0, 1);
		return Vec2f(u,v);
    };

	auto distToSurface = [&](Pnt3f p) -> float {
		Vec2f uv = toUV(p);
		auto c = tex->getPixel(uv);
		return p[1] - c[3];
	};

	if (!VRGeometry::applyIntersectionAction(action)) return false;

    IntersectAction* ia = dynamic_cast<IntersectAction*>(action);

    auto ia_line = ia->getLine();
    Pnt3f p0 = ia_line.getPosition();
    Vec3f d = ia_line.getDirection();
    Pnt3f p = ia->getHitPoint();

    Vec3f norm(0,1,0); // TODO
    int N = 1000;
    float step = 10; // TODO
    int dir = 1;
    for (int i = 0; i < N; i++) {
        p = p - d*step*dir; // walk
        float l = distToSurface(p);
        if (l > 0 && l < 0.03) {
            Real32 t = p0.dist( p );
            ia->setHit(t, ia->getActNode(), 0, norm, -1);
            break;
        } else if (l*dir > 0) { // jump over surface
            dir *= -1;
            step *= 0.5;
        }
    }

    return true;
}


string VRTerrain::vertexShader =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;

void main(void) {
    gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
	gl_Position = osg_Vertex;
}
);

string VRTerrain::fragmentShader =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
const ivec3 off = ivec3(-1,0,1);
const vec3 light = vec3(-1,-1,-0.5);
uniform vec2 texelSize;

vec3 norm;
vec4 color;

vec3 mixColor(vec3 c1, vec3 c2, float t) {
	t = clamp(t, 0.0, 1.0);
	return mix(c1, c2, t);
}

vec3 getColor() {
	return texture2D(tex, gl_TexCoord[0].xy).rgb;
}

void applyBlinnPhong() {
	norm = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( norm, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot( norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
	gl_FragColor[3] = 1.0;
}

vec3 getNormal() {
	vec2 tc = gl_TexCoord[0].xy;
    float s11 = texture(tex, tc).a;
    float s01 = textureOffset(tex, tc, off.xy).a;
    float s21 = textureOffset(tex, tc, off.zy).a;
    float s10 = textureOffset(tex, tc, off.yx).a;
    float s12 = textureOffset(tex, tc, off.yz).a;

    vec2 r2 = 2.0*texelSize;
    vec3 va = normalize(vec3(r2.x,s21-s01,0));
    vec3 vb = normalize(vec3(   0,s12-s10,r2.y));
    vec3 n = cross(vb,va);
	return n;
}

void main( void ) {
	norm = getNormal();
	color = vec4(getColor(),1.0);
	applyBlinnPhong();
}
);

string VRTerrain::tessControlShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout(vertices = 4) out;
out vec3 tcPosition[];
out vec2 tcTexCoords[];
uniform float resolution;
)
"\n#define ID gl_InvocationID\n"
GLSL(
void main() {
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    tcTexCoords[ID] = gl_in[ID].gl_TexCoord[0].xy;

    if (ID == 0) {
		vec4 mid = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position + gl_in[3].gl_Position) * 0.25;
		vec4 pos = gl_ModelViewProjectionMatrix * vec4(mid.xyz, 1.0);
		float D = length(pos.xyz);
		//int p = int(5.0/(resolution*D));
		//int res = int(pow(2,p));
		int res = int(resolution*32*64/D);
		res = clamp(res, 1, 64);
		if (res >= 64) res = 64; // take closest power of two to avoid jumpy effects
		else if (res >= 32) res = 32;
		else if (res >= 16) res = 16;
		else if (res >= 8) res = 8;
		else if (res >= 4) res = 4;
		else if (res >= 2) res = 2;

        gl_TessLevelInner[0] = res;
        gl_TessLevelInner[1] = res;

        gl_TessLevelOuter[0] = 64;
        gl_TessLevelOuter[1] = 64;
        gl_TessLevelOuter[2] = 64;
        gl_TessLevelOuter[3] = 64;
    }
}
);

string VRTerrain::tessEvaluationShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout( quads ) in;
in vec3 tcPosition[];
in vec2 tcTexCoords[];

uniform sampler2D texture;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 ta = mix(tcTexCoords[0], tcTexCoords[1], u);
    vec2 tb = mix(tcTexCoords[3], tcTexCoords[2], u);
    vec2 tc = mix(ta, tb, v);
    gl_TexCoord[0] = vec4(tc.x, tc.y, 1.0, 1.0);

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    vec3 tePosition = mix(a, b, v);
    tePosition.y = texture2D(texture, gl_TexCoord[0].xy).a;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(tePosition, 1);
}
);

