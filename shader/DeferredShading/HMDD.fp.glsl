#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D texBufDiff2;
uniform float uRadius;

varying vec2 tcs;

const float PI = 3.1415926535;

void main(void) {
    float aperture = 178.0;
    float apertureHalf = 0.5 * aperture * (PI / 180.0);
    float maxFactor = sin(apertureHalf);
  
    vec2 uv;
    vec2 xy = 2.0 * tcs.xy - 1.0;
    float d = length(xy);
    if (d < (2.0-maxFactor)) {
        d = length(xy * maxFactor);
        float z = sqrt(1.0 - d * d);
        float r = atan(d, z) / PI;
        float phi = atan(xy.y, xy.x);
    
        uv.x = r * cos(phi) + 0.5;
        uv.y = r * sin(phi) + 0.5;
    }
    else uv = tcs.xy;

    vec4 mDiff2 = texture2D(texBufDiff2, uv);
    gl_FragColor = mDiff2;
}

