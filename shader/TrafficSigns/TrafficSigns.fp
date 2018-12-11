#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

// gen
uniform sampler2D tex;
in vec3 norm;
in vec4 pos;
in vec2 tcs;
in mat3 miN;
in mat4 miP;

float theta;
vec3 PCam0;
vec3 newfragDir;
vec4 color;

void computeDirection() {
	newfragDir = normalize( miN * (miP * pos).xyz );
}

float gettheta(vec3 d){
	return acos(d.y);
}

void applyBlinnPhong() {
	vec3  n = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess ); 
	gl_FragColor = ambient + diffuse + specular;
}

void main() {
	//computeDirection();

	//float ca = col[1];
	//float ch = col[2];
	color = texture(tex,tcs);
	if (color.a < 0.3) discard;
	//color.a = 1.0;
	gl_FragColor = color;
	
	//applyBlinnPhong();
	//discard;
}





