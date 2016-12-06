#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;
uniform int               channel;

vec4 norm;
vec4 color;

void computeDirLight() {
    vec3 lightDir = normalize( gl_LightSource[0].position.xyz );
    float NdotL = max(dot(norm.xyz, lightDir), 0.0);
    if (NdotL > 0.0) color = NdotL * color * gl_LightSource[0].diffuse;
    else color = vec4(0);
}

void main(void) {
	vec2 lookup = gl_FragCoord.xy - vpOffset;
	norm = texture2DRect(texBufNorm, lookup);
	if (dot(norm.xyz, norm.xyz) < 0.95) discard;

	color = texture2DRect(texBufDiff, lookup);
	bool isLit = (norm.w > 0);

	if (channel == 0) {
		if (isLit) computeDirLight();
		else color = vec4(color.xyz, 1.0);
	}
	if (channel == 1) color = vec4(texture2DRect(texBufPos,  lookup).xyz, 1.0);
	if (channel == 2) color = vec4(norm.xyz, 1.0);
	if (channel == 3) color = vec4(color.xyz, 1.0);
	gl_FragColor = color;
}
