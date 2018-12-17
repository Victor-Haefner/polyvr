#version 120
uniform int isLit;
varying vec4 vertPos;
varying vec3 vertNorm;
varying vec4 color;
uniform sampler2D tex0;

vec3 norm;

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
	vec3 pos = vertPos.xyz / vertPos.w;
	vec4 diffCol = texture2D(tex0, gl_TexCoord[0].xy);
	if (diffCol.a < 0.1) discard;
	diffCol.rgb = mix(vec3(0.5), diffCol.rgb, diffCol.a);
	vec3 norm = normalize(vertNorm);
	if (norm.z < 0) {
		diffCol = vec4(0.8,0.8,0.8,1);
		norm *= -1; 
	}
	applyBlinnPhong();
}





