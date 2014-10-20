varying vec3 N;
varying vec3 v;
uniform sampler2D myTexture;

varying vec2 vTexCoord;

void main (void)
{
   	vec4 finalColor = vec4(0.0, 0.0, 0.0, 0.0);

	vec3 L = normalize(gl_LightSource[0].position.xyz);
  	vec3 E = normalize(-v); // we are in Eye Coordinates, so EyePos is (0,0,0)
  	vec3 R = normalize(-reflect(L,N));

  	//calculate Ambient Term:
 	vec4 Iamb = gl_FrontLightProduct[0].ambient;

	//calculate Diffuse Term:
   	vec4 Idiff = gl_FrontLightProduct[0].diffuse * max(dot(N,L), 0.0);

  	// calculate Specular Term:
 	vec4 Ispec = gl_FrontLightProduct[0].specular * pow(max(dot					(R,E),0.0),0.3*gl_FrontMaterial.shininess);

   	finalColor+=Iamb + Idiff + Ispec;
  	// write Total Color:
	vec4 sColor = gl_FrontLightModelProduct.sceneColor;
	sColor.a = 0.0;
	vec4 tex = texture2D(myTexture, vTexCoord);
	if(tex.a < 0.6) discard;
  	vec4 result = sColor + (tex);
	gl_FragColor = result;
	//gl_FragColor = finalColor;
}

