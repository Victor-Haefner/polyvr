#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;

void main(void) {
    vec2 lookup = gl_FragCoord.xy;

    vec4 pos = texture2DRect(texBufPos, lookup);
    if (pos.z >= 0) discard;

    gl_FragData[0] = pos;
    gl_FragData[1] = texture2DRect(texBufNorm, lookup);
    gl_FragData[2] = texture2DRect(texBufDiff, lookup);
}

