#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform sampler2D uTexKernel;
uniform sampler2D uTexNoise;
uniform float texScale;
uniform int KernelSize;
uniform int NoiseSize;
uniform float uRadius;

varying mat4 uProjectionMat;

vec2 toScreen(vec3 p) {
    vec4 ps = vec4(p, 1.0);
    ps = uProjectionMat * ps;
    ps.xy /= ps.w;
    ps.xy = ps.xy * 0.5 + 0.5;
    return ps.xy;
}

void main(void) {
    /*vec2 lookup = gl_FragCoord.xy;

    vec4 pos = texture2DRect(texBufPos, lookup);
    //if (pos.z >= 0) discard;

    gl_FragData[0] = pos;
    gl_FragData[1] = texture2DRect(texBufNorm, lookup);
    gl_FragData[2] = texture2DRect(texBufDiff, lookup);*/


    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);
    vec3 noise  = texture2D(uTexNoise, vec2(lookup*texScale) ).xyz;

    vec3 pos = posAmb.xyz;
    if (pos.z >= 0) discard;
    float amb = posAmb.w;

    //if (mod(lookup.x,2.0) <= 0.5 && mod(lookup.y,2.0) <= 0.5) {
    if (true) {
        vec2 view = lookup.xy/toScreen(pos);
        vec3 rvec = noise;
        vec3 tangent = normalize(rvec - norm * dot(rvec, norm));
        vec3 bitangent = cross(norm, tangent);
        mat3 tbn = mat3(tangent, bitangent, norm);

        float occlusion = 0.0;
        int N = 0;
        float R = uRadius;
        for (int i = 0; i < KernelSize*KernelSize; ++i) {
            float sx = mod(i,NoiseSize);
            float sy = i/NoiseSize;
            vec3 sample = tbn * texture2D(uTexKernel, vec2(sx*texScale, sy*texScale) ).xyz;
            sample = sample*R + pos;

            vec2 screen_sample = toScreen(sample) * view;
            float sampleDepth = texture2DRect(texBufPos, screen_sample).z;
            float rangeCheck = abs(pos.z - sampleDepth) < R ? 1.0 : 0.0;
            occlusion += (sampleDepth >= sample.z ? 1.0 : 0.0) * rangeCheck;
            N++;
        }

        occlusion = 1.0 - (occlusion / N);
        amb = occlusion;
    } else { discard; }

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff);
}

