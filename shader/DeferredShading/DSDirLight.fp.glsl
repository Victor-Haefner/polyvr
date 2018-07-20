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
uniform vec4 shadowColor;

vec3 pos;
vec4 norm;
vec4 color = vec4(0);



void computeDirLight() {
	vec3 light = normalize( lightDir );
	//if (norm.z < -0.1) norm *= -1; // two sided // induces artifacts
  	float NdotL = dot(norm.xyz, -light);
  	float mNdotL = max(NdotL, 0.0);
	vec4 ambient = gl_LightSource[0].ambient * color * (0.8+0.2*abs(NdotL));
	vec4 diffuse = gl_LightSource[0].diffuse * color * mNdotL;
	float NdotHV = max(dot(norm.xyz, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = vec4(0);
	if (mNdotL > 0.0) specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );


	color = ambient + diffuse + specular;
}

void main(void) {
	vec2 lookup = gl_FragCoord.xy - vpOffset;
	norm = texture2DRect(texBufNorm, lookup);
	bool isLit = (norm.w > 0);
    	if (channel != 0 || !isLit || dot(norm.xyz, norm.xyz) < 0.95) discard;

	pos = texture2DRect(texBufPos,  lookup).xyz;
	color = texture2DRect(texBufDiff, lookup);
	computeDirLight();
	gl_FragColor = color;
}
