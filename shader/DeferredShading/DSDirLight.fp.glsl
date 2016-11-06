#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;
uniform int               channel;

vec3 pos;
vec3 norm;
vec4 color = vec4(0);

void computeDirLight() {
    vec3 lightDir = normalize( gl_LightSource[0].position.xyz );
    float NdotL = max(dot(norm, lightDir), 0.0);
    if (NdotL > 0.0) color = NdotL * color * gl_LightSource[0].diffuse;
    else color = vec4(0);
}

void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    norm = texture2DRect(texBufNorm, lookup).xyz;

    if (dot(norm, norm) < 0.95) discard;
    else {
        vec4  posAmb = texture2DRect(texBufPos,  lookup);
        float amb = posAmb.w;
        pos = posAmb.xyz;
        color = texture2DRect(texBufDiff, lookup);

	if (channel == 7168) computeDirLight();
	if (channel == 4611) color = vec4(posAmb.xyz, 1.0);
	if (channel == 2977) color = vec4(norm.xyz, 1.0);
	if (channel == 4609) color = vec4(color.xyz, 1.0);
        gl_FragColor = color;
    }
}
