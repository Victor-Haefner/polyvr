#version 400 compatibility

vec4 fvAmbient  = vec4(0.36, 0.36, 0.36, 1.0);
//vec4 fvSpecular = vec4(0.7,  0.7,  0.7,  1.0);
vec4 fvSpecular = vec4(0.3,  0.3,  0.3,  1.0);
vec4 fvDiffuse  = vec4(0.5,  0.5,  0.5,  1.0);
//float fSpecularPower = 25.0;
float fSpecularPower = 10.0;

uniform sampler3D tex;

in float cylR1;
in float cylR2;
in vec3 cylDir;
in vec3 cylP0;
in vec3 cylP1;
in vec3 cylN0;
in vec3 cylN1;

in vec3 ViewDirection;
in vec3 fvObjectPosition;
in vec3 MVPos;
in vec3 Normal;
in vec3 TexCoord;

vec3 norm;

vec2 solveEq(float A, float B, float C) {
   	float D = B*B-4.0*A*C;
   	if (D < 0.0) discard; // no solution/intersection
   	D = sqrt(D);
   	float t1 = (-B+D)/A*0.5;
   	float t2 = (-B-D)/A*0.5;
   	//if (t1 < 0) return t2;
   	//if (t2 < 0) return t1;
   	return vec2(t1, t2);
}

vec3 raycastCylinder(vec3 rayStart, vec3 rayDir) {
   	vec3 rayDRad = rayDir - dot(rayDir, cylDir)*cylDir;
   	vec3 rayPRad = rayStart-cylP0 - dot(rayStart-cylP0, cylDir)*cylDir;
   	
   	float A = dot(rayDRad,rayDRad);
   	float B = 2.0*dot(rayDRad, rayPRad);
   	float C = dot(rayPRad, rayPRad) - cylR1*cylR1;
   	vec2 t = solveEq(A, B, C);
   	
   	vec3 pC = rayStart + max(t[0],t[1])*rayDir;
   	if ( dot(cylN0, pC-cylP0) < 0.0 ) discard;
   	if ( dot(cylN1, pC-cylP1) > 0.0 ) discard;
   	
   	norm = pC - cylP0 - dot(pC - cylP0, cylDir)*cylDir;
   	norm = normalize(norm);
   	return pC;
}

vec3 raycastCone(vec3 rayStart, vec3 rayDir) {
   	float H = distance(cylP0, cylP1);
   	float H2 = H*H;
   	float dR = cylR1-cylR2;
   	float dR2 = dR*dR;
   	
   	if (abs(dR) < 0.0001) return raycastCylinder(rayStart, rayDir);
   	
   	vec3 cylPa = cylP0 + cylDir * cylR1*H/dR;
   	float vrvc = dot(rayDir, cylDir);
   	float dpvc = dot(rayStart-cylPa, cylDir);
   	vec3 rayDRad = rayDir - vrvc*cylDir;
   	vec3 rayPRad = rayStart-cylPa - dpvc*cylDir;
   	float cos2a = H2 / (H2 + dR2);
   	float sin2a = 1.0 - cos2a;
   	
   	float A = cos2a * dot(rayDRad,rayDRad) - sin2a * vrvc * vrvc;
   	float B = 2.0 * (cos2a * dot(rayDRad, rayPRad) - sin2a * vrvc * dpvc);
   	float C = cos2a * dot(rayPRad, rayPRad) - sin2a * dpvc * dpvc;
   	vec2 t = solveEq(A, B, C);
   	
   	
   	vec3 pC = rayStart + max(t[0], t[1])*rayDir;
   	if ( dot(cylN0, pC-cylP0) < 0.0 ) discard;
   	if ( dot(cylN1, pC-cylP1) > 0.0 ) discard;
   	
   	norm = pC - cylP0 - dot(pC - cylP0, cylDir)*cylDir;
   	norm = normalize(norm);
   	return pC;
}

void main( void ) {
	norm = Normal;
   	vec3 rayStart = MVPos;
   	vec3 rayDir = -MVPos;
   	rayDir = normalize(rayDir);
   	
	vec3 I = fvObjectPosition;
	I = raycastCone(rayStart, rayDir);

   vec3  fvNormal         = normalize( norm );
   vec3  fvLightDirection = normalize( gl_LightSource[0].position.xyz - I.xyz);
   float fNDotL           = dot( fvNormal, fvLightDirection );

   vec3  fvReflection     = normalize( ( ( 2.0 * fvNormal ) * fNDotL ) - fvLightDirection );
   vec3  fvViewDirection  = normalize( ViewDirection );
   float fRDotV           = max( 0.0, dot( fvReflection, fvViewDirection ) );

   vec4  fvBaseColor      = texture(tex, TexCoord*10);

   vec4  fvTotalAmbient   = fvAmbient * fvBaseColor;
   vec4  fvTotalDiffuse   = fvDiffuse * fNDotL * fvBaseColor;
   vec4  fvTotalSpecular  = fvSpecular * ( pow( fRDotV, fSpecularPower ) );

   gl_FragColor = fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular;
   //gl_FragColor = vec4(fvObjectPosition.xyz, 1.0);
   //gl_FragColor = vec4(10*cylR1, 10*cylR2, 0.0, 1.0);
   //gl_FragColor = vec4(abs(cylDir.xyz), 1.0);
   //gl_FragColor = vec4(abs(cylP0.xyz), 1.0);
   //gl_FragColor = vec4(abs(I.xyz), 1.0);
}




