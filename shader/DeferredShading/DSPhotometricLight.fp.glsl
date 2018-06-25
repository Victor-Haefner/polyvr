#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform sampler2D     texPhotometricMap;
uniform vec2          vpOffset;
uniform int           channel;
uniform int           isFirstLamp;


uniform vec3 lightUp;
uniform vec3 lightDir;
uniform vec3 lightPos;

vec3 pos;
vec4 norm;
vec4 color = vec4(0);
vec2 lookup;

#define Pi 3.1415926535897932384626433832795


float getPhotometricIntensity(vec3 vertex, vec3 light) {
	vec3 lightVector = normalize( vertex - light );

	float dotV = dot( lightUp, lightVector ); 			// angle with up vector
	lightVector -= lightUp * dotV;				// project light vector in dir plane
	lightVector = normalize( lightVector );
	float aH = acos(dot( lightDir, lightVector ))/Pi;
	float aV = acos(dotV)/Pi;
	float dotX = dot( cross(lightUp, lightDir), lightVector );

	aV = 1.0 - aV;
	float u = 1.0 - aH;
	aH = u*0.5;
	float aH2 = 0.5-aH;
	if (dotX > 0) aH = 1.0 - aH;
	if (dotX < 0) aH2 = 0.5 - aH2;
	else aH2 -= 0.5;

	//if (aH < 0.5) aH = 0.3;
	//return aV;

	//apply intensity map
	float t1 = texture2D( texPhotometricMap, vec2(aH2,aV) ).a;
	float t2 = texture2D( texPhotometricMap, vec2(aH ,aV) ).a;
	return mix(t1, t2, u);
}

vec4 computePointLight(float amb) {
    vec4  res = vec4(0);
    vec3  lightDUN = lightPos - pos;
    vec3  lightD   = normalize(lightDUN);
    float NdotL      = max(dot(norm.xyz, lightD), 0.);

    if(NdotL > 0.) {
        float lightDist = length(lightDUN);
        float distAtt   = dot(vec3(gl_LightSource[0].constantAttenuation,
                                   gl_LightSource[0].linearAttenuation,
                                   gl_LightSource[0].quadraticAttenuation),
                              vec3(1., lightDist, lightDist * lightDist));
        distAtt = 1. / distAtt;



        res = amb * distAtt * NdotL * color * gl_LightSource[0].diffuse;
    }

    float intensity = getPhotometricIntensity(pos, lightPos);
    res.rgb *= intensity;
    return res;
}

void main(void) {
    lookup = gl_FragCoord.xy - vpOffset;
    norm   = texture2DRect(texBufNorm, lookup);
    bool isLit  = (norm.w > 0);

    if(channel != 0 || !isLit || dot(norm.xyz, norm.xyz) < 0.95) discard;
    else {
        vec4 posAmb = texture2DRect(texBufPos,  lookup);
        pos = posAmb.xyz;
        color = texture2DRect(texBufDiff, lookup);
	color = computePointLight(posAmb.w);
        gl_FragColor = color;
    }
}





