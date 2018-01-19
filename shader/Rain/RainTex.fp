#version 400 compatibility

in vec4 pos;
in mat3 miN;
in mat4 miP;
vec3 fragDir;
float theta;
float camD = 40;

in vec3 N;
in vec3 v;
in vec2 tc;

uniform sampler2D tex0;
uniform sampler2D tex1;


void main() {
	vec4 c = texture2D(tex1, tc);
	gl_FragColor = vec4(c.xyz,1);
}
