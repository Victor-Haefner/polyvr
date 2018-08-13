#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform vec4 fogParams;
uniform vec4 fogColor;

uniform int uBlurSize; // use size of noise texture

void main(void) {
    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);

    vec3 pos = posAmb.xyz;
    if (pos.z >= 0) discard;
    float amb = posAmb.w;

    float z = abs(pos.z);
    if (z > fogParams[1] && fogParams[0] > 0.5) {
	float t = (z-fogParams[1])/(fogParams[2]-fogParams[1]);
	t = clamp(pow(t,fogParams[0]), 0, 1);
	mDiff = mix(mDiff, fogColor, t);
	norm = normalize(mix(norm, vec3(0,0,1), t));
    }

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff.xyz,1);
}
