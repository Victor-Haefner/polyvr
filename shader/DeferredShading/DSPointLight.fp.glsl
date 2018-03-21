#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

// DS input buffers
uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform vec2          vpOffset;
uniform int           channel;

uniform vec3 lightUp;
uniform vec3 lightDir;
uniform vec3 lightPos;

// compute point light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computePointLight(int index, float amb, vec3 pos, vec3 norm, vec4 mDiff) {
    vec4  color = vec4(0);
    vec3  lightDirUN = lightPos - pos;
    vec3  lightDir   = normalize(lightDirUN);
    float NdotL      = max(dot(norm, lightDir), 0.);

    if(NdotL > 0.) {
        float lightDist = length   (lightDirUN);
        float distAtt   = dot(vec3(gl_LightSource[index].constantAttenuation,
                                   gl_LightSource[index].linearAttenuation,
                                   gl_LightSource[index].quadraticAttenuation),
                              vec3(1., lightDist, lightDist * lightDist));
        distAtt = 1. / distAtt;

        color = amb * distAtt * NdotL * mDiff * gl_LightSource[index].diffuse;
        //color = (amb*0.8+0.2) * distAtt * NdotL * mDiff * gl_LightSource[index].diffuse;
        //color = amb*gl_LightSource[index].ambient + distAtt * NdotL * mDiff * gl_LightSource[index].diffuse;
    }

    return color;
}

// DS pass
void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    vec4 norm   = texture2DRect(texBufNorm, lookup);
    bool isLit  = (norm.w > 0);

    if(dot(norm.xyz, norm.xyz) < 0.95) discard;
    else {
        vec4  posAmb = texture2DRect(texBufPos,  lookup);
        vec3  pos    = posAmb.xyz;
        vec4  color  = texture2DRect(texBufDiff, lookup);

	if (channel == 0) {
		if (isLit) color = computePointLight(0, posAmb.w, pos, norm.xyz, color);
		else color = vec4(color.xyz, 1.0);
	}
	if (channel == 1) color = vec4(posAmb.xyz, 1.0);
	if (channel == 2) color = vec4(norm.xyz, 1.0);
	if (channel == 3) color = vec4(color.xyz, 1.0);
	if (channel == 4) color = vec4(posAmb.w, posAmb.w, posAmb.w, 1.0);
        gl_FragColor = color;
    }
}
