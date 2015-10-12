#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform sampler3D uTexRandom;
uniform float texScale;
uniform int KernelSize;
uniform int NoiseSize;
uniform float uRadius;

varying mat4 uProjectionMat;

vec3 getKernel(int i) {
    float sx = mod(i,NoiseSize);
    float sy = i/NoiseSize;
    return texture3D(uTexRandom, vec3(sx*texScale, sy*texScale, 0) ).xyz;
}

vec3 computeSample(int i, mat3 tbn, float R) {
    vec3 sample = tbn * getKernel(i);
    return sample * R;
}

vec2 toScreen(vec3 p) {
    vec4 ps = vec4(p, 1.0);
    ps = uProjectionMat * ps;
    ps.xy /= ps.w;
    ps.xy = ps.xy * 0.5 + 0.5;
    return ps.xy;
}

void main(void) {
    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);
    vec3 noise  = texture3D(uTexRandom, vec3(lookup*texScale, 1) ).xyz;

    vec3 pos = posAmb.xyz;
    float amb = posAmb.w;
    vec2 view = lookup.xy/toScreen(pos);

    vec3 rvec = noise * 2.0 - 1.0;
    vec3 tangent = normalize(rvec - norm * dot(rvec, norm));
    vec3 bitangent = cross(norm, tangent);
    mat3 tbn = mat3(tangent, bitangent, norm);

    float occlusion = 0.0;
    int N = 0;
    float R = uRadius;
    for (int i = 0; i < KernelSize*KernelSize; ++i) {
        vec3 sample = computeSample(i, tbn, R) + pos;
        vec2 screen_sample = toScreen(sample) * view;

        // get sample depth:
        float sampleDepth = texture2DRect(texBufPos, screen_sample).z;

        // range check & accumulate:
        float rangeCheck = abs(pos.z - sampleDepth) < R ? 1.0 : 0.0;
        occlusion += (sampleDepth >= sample.z ? 1.0 : 0.0) * rangeCheck;
        N++;
    }

    occlusion = 1.0 - (occlusion / N);
    amb = occlusion;

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff);
}

