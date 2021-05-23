#include "VRSkin.h"
#include "VRSkeleton.h"

#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"

#define GLSL(shader) #shader

using namespace OSG;

VRSkin::VRSkin(VRSkeletonPtr s) : skeleton(s) {
    material = VRMaterial::create("skin");
    material->setDiffuse(Color3f(1,0,0));

    material->setVertexShader(skinning_vp, "skinningVP");
    material->setFragmentShader(skinning_fp, "skinningFP");
}

VRSkin::~VRSkin() {}

VRSkinPtr VRSkin::create(VRSkeletonPtr s) { return VRSkinPtr( new VRSkin(s) ); }
VRSkinPtr VRSkin::ptr() { return static_pointer_cast<VRSkin>(shared_from_this()); }

VRMaterialPtr VRSkin::getMaterial() { return material; }

void VRSkin::computeMapping(VRGeometryPtr hull) { // TODO: switch to texture if multiple bones are needed
    cout << "computeMapping!" << endl;
    map<int, int> mapping;

    bone0s = skeleton->getBones();

    VRGeoData data(hull);

    for (int i=0; i<data.size(); i++) {
        Pnt3d p = data.getPosition(i);
        int bID = floor(i*0.125); // TODO, this is just a test..
        mapping[i] = bID;
        data.setTexCoord(i, Vec2d(i+0.1,bID+0.1));
    }

    auto tg = VRTextureGenerator::create();
    tg->setSize(Vec3i(data.size(),16,1), false);
    tg->drawFill(Color4f(0,0,0,1));

    for (auto m : mapping) {
        int vID = m.first;
        tg->drawPixel( Vec3i(vID, 0, 0), Color4f(1, 0, 0, 1) ); // parameters, [Nbones, 0, 0, 1]
        float t = 1;
        //if (!(vID%4 > 1)) t = 0;
        tg->drawPixel( Vec3i(vID, 1, 0), Color4f(m.second, t, 0, 1) ); // per bone, [bID, t, 0, 1]
    }

    material->setTexture(tg->compose(0), false, 1);
    material->setMagMinFilter(GL_LINEAR, GL_LINEAR, 1);
    material->setShaderParameter("texMapping", 1);

    updateBoneTexture();
}

void VRSkin::updateBoneTexture() {
    auto bones = skeleton->getBones();
    int Nb = bones.size();

    auto tg = VRTextureGenerator::create();
    tg->setSize(Vec3i(Nb,8,1), true);
    tg->drawFill(Color4f(1,1,1,1));

    auto writeVec3 = [&](int i, int j, Vec3d v) {
        tg->drawPixel( Vec3i(i, j, 0), Color4f(v[0], v[1], v[2], 1) );
    };
    auto writeVec4 = [&](int i, int j, Vec4d v) {
        tg->drawPixel( Vec3i(i, j, 0), Color4f(v[0], v[1], v[2], v[3]) );
    };

    for (int i=0; i<bones.size(); i++) {
        auto& bone  = bones[i];
        auto& bone0 = bone0s[i];
        Vec3d p0 = (bone0.p1 + bone0.p2)*0.5;
        Vec3d p  = (bone.p1 + bone.p2)*0.5;
        auto m0 = Pose(p0, bone0.dir, bone0.up).asMatrix();
        m0.invert();
        auto m = Pose(p, bone.dir, bone.up).asMatrix();
        m.multLeft(m0);

        writeVec3(i,0,p0);
        writeVec4(i,1,m[0]);
        writeVec4(i,2,m[1]);
        writeVec4(i,3,m[2]);
        writeVec4(i,4,m[3]);
    }

    material->setTexture(tg->compose(0), false, 0);
    material->setMagMinFilter(GL_LINEAR, GL_LINEAR, 0);
    material->setShaderParameter("texBones", 0);
}


string VRSkin::skinning_vp =
"#version 400 compatibility\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(

in vec4 osg_Vertex;
in vec4 osg_Color;
in vec3 osg_Normal;
in vec2 osg_MultiTexCoord0;

uniform sampler2D texMapping;
uniform sampler2D texBones;

out vec4 color;
out vec3 norm;

void main(void) {
	int vID = int(osg_MultiTexCoord0.x);
	vec4 params = texelFetch(texMapping, ivec2(vID,0), 0);
	int Nb = int(params.x);

	vec3 pv = osg_Vertex.xyz;

	vec3 d = vec3(0.0);
	for (int i=0; i<1; i++) {
        vec4 b1 = texelFetch(texMapping, ivec2(vID,1), 0);
        int bID = int(b1.x);
        float t = b1.y;

        vec3 p0 = texelFetch(texBones, ivec2(bID,0), 0).rgb;
        vec4 m0 = texelFetch(texBones, ivec2(bID,1), 0).rgba;
        vec4 m1 = texelFetch(texBones, ivec2(bID,2), 0).rgba;
        vec4 m2 = texelFetch(texBones, ivec2(bID,3), 0).rgba;
        vec4 m3 = texelFetch(texBones, ivec2(bID,4), 0).rgba;

        mat4 M = mat4(m0[0],m1[0],m2[0],m3[0],
                      m0[1],m1[1],m2[1],m3[1],
                      m0[2],m1[2],m2[2],m3[2],
                      m0[3],m1[3],m2[3],m3[3]);

        vec3 p2 = p0 + ( M*vec4(pv-p0,1.0) ).xyz;

        d = (p2-pv)*t;
	}

    vec4 pos = osg_Vertex + vec4(d,0);
	gl_Position = gl_ModelViewProjectionMatrix * pos;
	color = osg_Color;
	norm = osg_Normal;
}
);

string VRSkin::skinning_fp =
"#version 400 compatibility\n"
GLSL(
in vec4 color;
in vec3 norm;

void applyBlinnPhong() {
	vec3 n = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
}

void main(void) {
    applyBlinnPhong();
}
);

