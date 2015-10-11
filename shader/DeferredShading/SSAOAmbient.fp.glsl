#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform sampler2DRect texBufAmb;

void main(void) {

    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec4 norm   = texture2DRect(texBufNorm, lookup);
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);
    vec4 amb    = texture2DRect(texBufAmb,  lookup);

    gl_FragData[0] = vec4(posAmb);
    gl_FragData[1] = vec4(norm);
    gl_FragData[2] = vec4(mDiff);
    gl_FragData[3] = vec4(amb*0.5);
}

