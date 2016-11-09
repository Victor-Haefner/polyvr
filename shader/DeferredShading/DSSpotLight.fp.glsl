#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

// compute spot light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computeSpotLight(int index, vec3 pos, vec3 norm, vec4 mDiff) {
    vec4  color      = vec4(0);
    vec3  lightDirUN = gl_LightSource[index].position.xyz - pos;
    vec3  lightDir   = normalize(lightDirUN);
    float spotEffect = dot(-lightDir, gl_LightSource[index].spotDirection);

    if(spotEffect > gl_LightSource[index].spotCosCutoff) {
        float NdotL = max(dot(lightDir, norm), 0.);
        
        if(NdotL > 0.) {
            float lightDist = length(lightDirUN);
            float att       = dot(vec3(gl_LightSource[index].constantAttenuation,
                                       gl_LightSource[index].linearAttenuation,
                                       gl_LightSource[index].quadraticAttenuation),
                                  vec3(1., lightDist, lightDist * lightDist)       );
            spotEffect = pow(spotEffect, gl_LightSource[index].spotExponent);
            att        = spotEffect / att;

            color = att * NdotL * mDiff * gl_LightSource[index].diffuse;
        }
    }
    
    return color;
}

// DS input buffers
uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;
uniform int               channel;

// DS pass
void main(void) {
    vec2  lookup = gl_FragCoord.xy - vpOffset;
    vec3  norm   = texture2DRect(texBufNorm, lookup).xyz;

    if (dot(norm, norm) < 0.95) discard;
    else {
        vec4  posAmb = texture2DRect(texBufPos,  lookup);
        vec3  pos    = posAmb.xyz;
        float amb    = posAmb.w;
        vec4  mDiff  = texture2DRect(texBufDiff, lookup);

        vec4 c = vec4(0);
	if (channel == 0) c = computeSpotLight(0, pos, norm, mDiff);
	if (channel == 1) c = vec4(posAmb.xyz, 1.0);
	if (channel == 2) c = vec4(norm.xyz, 1.0);
	if (channel == 3) c = vec4(mDiff.xyz, 1.0);
        gl_FragColor = c;
    }
}
