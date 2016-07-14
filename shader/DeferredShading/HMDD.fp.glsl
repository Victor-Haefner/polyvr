#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform float uRadius;

varying vec2 tcs;

const float PI = 3.1415926535;

void main(void) {
    vec2 lookup = gl_FragCoord.xy;

    /*float aperture = 178.0;
    float apertureHalf = 0.5 * aperture * (PI / 180.0);
    float maxFactor = sin(apertureHalf);
  
    vec2 uv;
    vec2 xy = 2.0 * tcs.xy - 1.0;
    float d = length(xy);
    if (d < (2.0-uRadius)) {
        d = length(xy * uRadius);
        float z = sqrt(1.0 - d * d);
        float r = atan(d, z) / PI;
        float phi = atan(xy.y, xy.x);
    
        uv.x = r * cos(phi) + 0.5;
        uv.y = r * sin(phi) + 0.5;
    }
    else uv = tcs.xy;

    lookup.xy = lookup.xy*uv.xy;*/

    if (lookup.x >= 200) lookup.x -= 200;

    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);

    vec3 pos = posAmb.xyz;
    if (pos.z >= 0) discard;
    float amb = posAmb.w;

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff);
}

