#version 400 compatibility

uniform sampler2D tex;

in vec3 vn;
in vec4 vc;

vec4 color;

void applyBlinnPhong() {
	vec3 n = normalize( gl_NormalMatrix * vn );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(n, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess ); 
	gl_FragColor = ambient + diffuse + specular;
}

void main( void ) {
	color = vc;
	applyBlinnPhong();
}
