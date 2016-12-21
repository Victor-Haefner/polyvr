#version 400 compatibility

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;
uniform int               channel;

vec4 norm;
vec4 color;

void computeDirLight(float ambFactor) {
	vec3 light = normalize( gl_LightSource[0].position.xyz );
  	float NdotL = max(dot(norm.xyz, light), 0.0);
	vec4 ambient = gl_LightSource[0].ambient * color;
	//vec4 ambient = color * ambFactor;
	vec4 diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(norm.xyz, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = vec4(0);
	if (NdotL > 0.0) specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	color = (ambient + diffuse) * ambFactor;
}

void main(void) {
	vec2 lookup = gl_FragCoord.xy - vpOffset;
	norm = texture2DRect(texBufNorm, lookup);
	if (dot(norm.xyz, norm.xyz) < 0.95) discard;
	vec4 pos = texture2DRect(texBufPos,  lookup);

	color = texture2DRect(texBufDiff, lookup);
	bool isLit = (norm.w > 0);

	if (channel == 0) {
		if (isLit) computeDirLight(pos.w);
		else color = vec4(color.xyz, 1.0);
	}
	if (channel == 1) color = vec4(pos.xyz, 1.0);
	if (channel == 2) color = vec4(norm.xyz, 1.0);
	if (channel == 3) color = vec4(color.xyz, 1.0);
	gl_FragColor = color;
}
