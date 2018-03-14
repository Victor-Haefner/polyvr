#version 400 compatibility

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

// DS input buffers
uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform sampler2D     texPhotometricMap;
uniform vec2          vpOffset;
uniform int           channel;
uniform int           isFirstLamp;

uniform vec3 lightUp;
vec3 lightDirection;

vec2 lookup;

void calcLightParams() {
	lightDirection = gl_LightSource[0].spotDirection;
}

float getPhotometricIntensity(vec3 vertex, vec3 light, vec3 normal) {
	calcLightParams();

	vec3 lightVector = vertex - light;
	lightVector = normalize( lightVector );
	float cosLight = -dot( normal, lightVector); 		// diffusion factor

	float dotV = dot( lightUp, lightVector ); 			// angle with up vector
	lightVector -= lightUp * dotV;				// project light vector in dir plane
	lightVector = normalize( lightVector );
	float dotH = dot( lightDirection, lightVector );

	//apply intensity map				
	vec2 tc = vec2(0,0);
	tc.y = dotH*0.5 + 0.5; //phi
	tc.x = dotV*0.5 + 0.5; //theta

	return texture2D( texPhotometricMap, tc ).a;
}

// compute point light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computePointLight(float amb, vec3 pos, vec3 norm, vec4 mDiff) {
    vec4  color = vec4(0);
    vec3  lightDirUN = gl_LightSource[0].position.xyz - pos;
    vec3  lightDir   = normalize(lightDirUN);
    float NdotL      = max(dot(norm, lightDir), 0.);

    if(NdotL > 0.) {
        float lightDist = length(lightDirUN);
        float distAtt   = dot(vec3(gl_LightSource[0].constantAttenuation,
                                   gl_LightSource[0].linearAttenuation,
                                   gl_LightSource[0].quadraticAttenuation),
                              vec3(1., lightDist, lightDist * lightDist));
        distAtt = 1. / distAtt;
        color = amb * distAtt * NdotL * mDiff * gl_LightSource[0].diffuse;
    }

    float intensity = getPhotometricIntensity(pos, gl_LightSource[0].position.xyz, norm);
    color.rgb *= intensity;
    return color;
}

void main(void) {
    lookup = gl_FragCoord.xy - vpOffset;
    vec4 norm   = texture2DRect(texBufNorm, lookup);
    bool isLit  = (norm.w > 0);

    if (dot(norm.xyz, norm.xyz) < 0.95) discard;
    else {
        vec4  posAmb = texture2DRect(texBufPos,  lookup);
        vec3  pos    = posAmb.xyz;
        vec4  color  = texture2DRect(texBufDiff, lookup);

	if (channel == 0 && isLit) color = computePointLight(posAmb.w, pos, norm.xyz, color);
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





