#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D texBufDiff2;
uniform float eye;

uniform vec2 OSGViewportSize;

varying vec2 tcs;

const float PI = 3.1415926535;
// 178.0 0.095 0.88
const float aperture = 178.0;
const float overlap = 0.095;
const float scale = 0.88;

void main(void) {
    float a = OSGViewportSize.y/OSGViewportSize.x;
    float apertureHalf = 0.5 * aperture * (PI / 180.0);
    float maxFactor = sin(apertureHalf);
  
    vec2 uv;
    vec2 xy = 2.0 * tcs.xy - 1.0;
    xy.y *= a;
    xy.x += overlap*eye;
    xy *= scale;

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

