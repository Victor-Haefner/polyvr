#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;
uniform int               channel;

uniform vec3 lightUp;
uniform vec3 lightDir;
uniform vec3 lightPos;

vec3 pos;
vec4 norm;
vec4 color = vec4(0);

vec4 OSG_SSME_FP_calcShadow(in vec4 ecFragPos);

void computeDirLight() {
    vec3  lightDir = gl_LightSource[0].position.xyz; // normalize( lightDir ); // TODO: which is right??
    float NdotL    = max(dot(norm.xyz, lightDir), 0.);

    if (NdotL > 0.) {
        vec4  shadow = OSG_SSME_FP_calcShadow(vec4(pos, 1.));
        color = shadow * NdotL * color * gl_LightSource[0].diffuse;
    } else color = vec4(0);
}

void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    norm = texture2DRect(texBufNorm, lookup);
    bool isLit = (norm.w > 0);

    if (channel != 0 || !isLit || dot(norm.xyz, norm.xyz) < 0.95) discard;
    else {
        pos = texture2DRect(texBufPos,  lookup).xyz;
        color = texture2DRect(texBufDiff, lookup);
	computeDirLight();
        gl_FragColor = color;
    }
}
