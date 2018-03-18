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
uniform float         shadowColor = 0.0;

uniform vec3 lightUp;
uniform vec3 lightDir;
uniform vec3 lightPos;

vec3 pos;
vec4 norm;
vec4 color = vec4(0);
vec2 lookup;

vec4 OSG_SSME_FP_calcShadow(in vec4 ecFragPos);

float getPhotometricIntensity(vec3 vertex, vec3 light, vec3 normal) {
	vec3 lightVector = vertex - light;
	lightVector = normalize( lightVector );
	float cosLight = -dot( normal, lightVector); 		// diffusion factor

	float dotV = dot( lightUp, lightVector ); 			// angle with up vector
	lightVector -= lightUp * dotV;				// project light vector in dir plane
	lightVector = normalize( lightVector );
	float dotH = dot( lightDir, lightVector );

	//apply intensity map				
	vec2 tc = vec2(0,0);
	tc.y = dotH*0.5 + 0.5; //phi
	tc.x = dotV*0.5 + 0.5; //theta

	return texture2D( texPhotometricMap, tc ).a;
}

vec4 computePointLight(float amb) {
    vec3  lightDUN = lightPos - pos;
    vec3  lightD   = normalize(lightDUN);
    float NdotL      = max(dot(norm.xyz, lightD), 0.);

    if(NdotL > 0.) {
        vec4  shadow    = OSG_SSME_FP_calcShadow(vec4(pos, 1.));
        shadow += shadowColor;

        float lightDist = length(lightDUN);
        float distAtt   = dot(vec3(gl_LightSource[0].constantAttenuation,
                                   gl_LightSource[0].linearAttenuation,
                                   gl_LightSource[0].quadraticAttenuation),
                              vec3(1., lightDist, lightDist * lightDist));
        distAtt = 1. / distAtt;
        color = shadow * amb * distAtt * NdotL * color * gl_LightSource[0].diffuse;
        //color = distAtt * NdotL * color * gl_LightSource[0].diffuse;
    }

    float intensity = getPhotometricIntensity(pos, lightPos, norm.xyz);
    color.rgb *= intensity;
    return color;
}

void main(void) {
    lookup = gl_FragCoord.xy - vpOffset;
    norm   = texture2DRect(texBufNorm, lookup);
    bool isLit  = (norm.w > 0);

    if (dot(norm.xyz, norm.xyz) < 0.95) discard;
    else {
        vec4 posAmb = texture2DRect(texBufPos,  lookup);
        pos = posAmb.xyz;
        color = texture2DRect(texBufDiff, lookup);

	if (channel == 0 && isLit) color = computePointLight(posAmb.w);
	else if (isFirstLamp == 1) {
		if (channel == 0) color = vec4(color.xyz, 1.0);
		if (channel == 1) color = vec4(posAmb.xyz, 1.0);
		if (channel == 2) color = vec4(norm.xyz, 1.0);
		if (channel == 3) color = vec4(color.xyz, 1.0);
	}
	else color = vec4(0);
        gl_FragColor = color;
    }
}





