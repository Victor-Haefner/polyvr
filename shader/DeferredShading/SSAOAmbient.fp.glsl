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

vec3 computeSample(int i, mat3 tbn) {
    vec3 sample = tbn * getKernel(i);
    return sample * uRadius;
}

void main(void) {
    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);
    vec3 noise  = texture3D(uTexRandom, vec3(lookup*texScale, 1) ).xyz;

    vec3 pos = posAmb.xyz;
    float amb = posAmb.w;

    vec3 rvec = noise * 2.0 - 1.0;
    vec3 tangent = normalize(rvec - norm * dot(rvec, norm));
    vec3 bitangent = cross(norm, tangent);
    mat3 tbn = mat3(tangent, bitangent, norm);

    float occlusion = 0.0;
    for (int i = 0; i < KernelSize*KernelSize; ++i) {
        vec3 sample = computeSample(i, tbn) + pos;

        // project sample position:
        vec4 offset = vec4(sample, 1.0);
        //offset = uProjectionMat * offset;
        //offset.xy /= offset.w;
        //offset.xy = offset.xy * 0.5 + 0.5;

        // get sample depth:
        //float sampleDepth = texture(uTexLinearDepth, offset.xy).r;
        float sampleDepth = texture2DRect(texBufPos, offset.xy).z;

        // range check & accumulate:
        float rangeCheck = 1.0;//abs(pos.z - sampleDepth) < uRadius ? 1.0 : 0.0;
        occlusion += (sampleDepth <= sample.z ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / (KernelSize*KernelSize));
    amb = occlusion;
    /*amb = 1.0;
    mDiff = vec4(1,0,0,1);

    vec3 t = vec3(0);
    for (int i = 0; i < KernelSize*KernelSize; ++i) {
        //vec3 sample = computeSample(i, tbn) + pos;
        vec3 sample = getKernel(i);
        t += abs(sample);
        if (sample.x > gl_FragCoord.x)// && sample.y < gl_FragCoord.y) 
            mDiff = vec4(sample.x,0,1,1);
    }
    mDiff = vec4(pos,1);*/

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff);
}

