#version 400 compatibility

uniform sampler2D tex;

in vec2 tcs;
in vec3 norm;
in vec3 col;
vec4 color;


void applyBlinnPhong() {
	vec3 n = gl_NormalMatrix * norm;
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess ); 
	gl_FragColor = ambient + diffuse + specular;
}

void main( void ) {
	float ca = col[1];
	float ch = col[2];
	color = texture(tex,tcs);
	if (color.a < 0.3) discard;
	//color.a = 1.0;
	color.x *= 0.4*ca;
	color.y *= 0.8*ch;
	color.z *= 0.2*ch;
	
	applyBlinnPhong();
}



