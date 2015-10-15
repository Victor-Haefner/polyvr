varying vec3 N;
varying vec3 v;
uniform sampler2D myTexture;

varying vec2 tc;
varying vec2 tc2;

void main (void) {
	vec3 L = normalize(gl_LightSource[0].position.xyz);
	//vec3 L = normalize(vec3(0,0,1));
  	vec3 E = normalize(-v); // we are in Eye Coordinates, so EyePos is (0,0,0)
  	vec3 R = normalize(-reflect(L,N));

 	vec4 Iamb = vec4(0.3,0.3,0.3,1.0);
   	vec4 Idiff = vec4(1.0,1.0,0.8,1.0) * max(dot(N,L), 0.0);
 	vec4 Ispec = vec4(0.5,0.5,0.5,1.0) * pow(max(dot(R,E),0.0), 0.3);

  	// write Total Color:
	vec4 sColor = vec4(0.0);
	sColor.a = 0.0;

	vec4 tex = texture2D(myTexture, tc);
  	vec4 result = (sColor + tex) * ( Iamb + Idiff + Ispec );
	gl_FragColor = result;
}

