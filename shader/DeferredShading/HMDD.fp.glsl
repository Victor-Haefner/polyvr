#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D texBufDiff2;
uniform float eye;

uniform vec2 OSGViewportSize;

varying vec2 tcs;

const float PI = 3.1415926535;
// 178.0 0.095 0.88
const float aperture = 100.0;
const float overlap = 0.5;
const float scale = 1.0;

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

	float dis = 0.095;
    if (d < (2.0-maxFactor)) {
	//gl_FragColor = vec4(xy.y,xy.x,0,1); return;
	if (xy.x < 0.0){ //left
	}
	if (xy.x > 0.0) { //right
	}
	if (xy.y < 0.0) { //down
	}
	if (xy.y > 0.0) { //top
	}
    }
    else uv = tcs.xy;
	if ( eye < 0.0 ) { //left eye
		uv.x = tcs.x-xy.y*xy.y/30-dis; 
		uv.y = tcs.y-xy.x/30;
	//gl_FragColor = vec4(1,0,0,1); return; 
	}
	if ( eye > 0.0 ) { //right eye
		uv.x = tcs.x+xy.y*xy.y/10+dis; 
		uv.y = tcs.y+xy.x/10;
	//gl_FragColor = vec4(0,1,0,1); return; 
	}
	if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) uv = tcs.xy;

    vec4 mDiff2 = texture2D(texBufDiff2, uv);
    gl_FragColor = mDiff2;
}
/*const float PI = 3.1415926535;
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
}*/
