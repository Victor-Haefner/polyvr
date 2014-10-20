varying vec3 N;
varying vec3 v;
uniform sampler2D myTexture;

varying vec2 tc;
varying vec2 tc2;

void main (void) {
	//vec3 L = normalize(gl_LightSource[0].position.xyz - v);
	vec3 L = normalize(gl_LightSource[0].position.xyz); // directional light
  	vec3 E = normalize(-v); // we are in Eye Coordinates, so EyePos is (0,0,0)
  	vec3 R = normalize(-reflect(L,N));

 	vec4 Iamb = gl_FrontLightProduct[0].ambient;
   	vec4 Idiff = gl_FrontLightProduct[0].diffuse * max(dot(N,L), 0.0);
 	vec4 Ispec = gl_FrontLightProduct[0].specular * pow(max(dot(R,E),0.0),0.3*gl_FrontMaterial.shininess);

  	// write Total Color:
	vec4 sColor = gl_FrontLightModelProduct.sceneColor;
	sColor.a = 0.0;

	// blend textures
	vec4 tex = texture2D(myTexture, tc);
	vec4 tex2 = texture2D(myTexture, tc2);
	tex = tex*(1-tex2[3]) + tex2*tex2[3];

  	vec4 result = (sColor + tex) * ( Iamb + Idiff + Ispec );
  	result.a = 1.0;
	gl_FragColor = result;

    //float c = max(dot(N,L), 0.0);
	//gl_FragColor = vec4(gl_LightSource[0].position.xyz, 1);
	//gl_FragColor = vec4(L,1.0);
}

