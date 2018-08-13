#version 400 compatibility

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;
uniform int               channel;

uniform vec3 lightUp;
uniform vec3 lightDir;
uniform vec3 lightPos;

uniform vec4 fogParams;
uniform vec4 fogColor;

vec3 pos;
vec4 norm;
vec4 color = vec4(0);

void computeFog() {
}

void main(void) {
	vec2 lookup = gl_FragCoord.xy - vpOffset;
	norm = texture2DRect(texBufNorm, lookup);
	bool isLit = (norm.w > 0);
    	if (channel != 0 || !isLit) discard;


	if (fogParams[0] > 0.5) {
		pos = texture2DRect(texBufPos,  lookup).xyz;
		color = texture2DRect(texBufDiff, lookup);
		float z = abs(pos.z);
		if (z < fogParams[1]) discard;
		float t = (z-fogParams[1])/(fogParams[2]-fogParams[1]);
		t = clamp(pow(t,fogParams[0]), 0, 1);
		gl_FragColor = mix(vec4(0,0,0,0), fogColor, t);
	} else discard;
}
