#version 120
#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

#extension GL_EXT_gpu_shader4 : require
#extension GL_EXT_gpu_shader4 : enable

// forward decls
vec4 OSG_SSME_FP_calcShadow(in vec4 ecFragPos);

uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;

// compute point light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computePointLight(int index, vec3 pos, vec3 norm, vec4 mDiff)
{
    vec4  color      = vec4(0., 0., 0., 0.);

    vec3  lightDirUN = gl_LightSource[index].position.xyz - pos;
    vec3  lightDir   = normalize(lightDirUN);
    float NdotL      = max(dot(norm, lightDir), 0.);

    if(NdotL > 0.)
    {
        vec4  shadow    = OSG_SSME_FP_calcShadow(vec4(pos, 1.));

        float lightDist = length   (lightDirUN);
        float distAtt   = dot(vec3(gl_LightSource[index].constantAttenuation,
                                   gl_LightSource[index].linearAttenuation,
                                   gl_LightSource[index].quadraticAttenuation),
                              vec3(1., lightDist, lightDist * lightDist));
        distAtt = 1. / distAtt;

        color = shadow * distAtt * NdotL * mDiff * gl_LightSource[index].diffuse;
//        color = shadow * 0.5;
    }

    return color;
}

// compute directional light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computeDirLight(int index, vec3 pos, vec3 norm, vec4 mDiff)
{
    vec4 color = vec4(0., 0., 0., 0.);

    vec3  lightDir = gl_LightSource[index].position.xyz;
    float NdotL    = max(dot(norm, lightDir), 0.);

    if(NdotL > 0.)
    {
        vec4  shadow = OSG_SSME_FP_calcShadow(vec4(pos, 1.));

        color = shadow * NdotL * mDiff * gl_LightSource[index].diffuse;
    }

    return color;
}

// compute spot light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computeSpotLight(int index, vec3 pos, vec3 norm, vec4 mDiff)
{
    vec4 color = vec4(0., 0., 0., 0.);

    vec3  lightDirUN = gl_LightSource[index].position.xyz - pos;
    vec3  lightDir   = normalize(lightDirUN);
    float spotEffect = dot(-lightDir, gl_LightSource[index].spotDirection);

    if(spotEffect > gl_LightSource[index].spotCosCutoff)
    {
        float NdotL = max(dot(lightDir, norm), 0.);
        
        if(NdotL > 0.)
        {
            vec4  shadow    = OSG_SSME_FP_calcShadow(vec4(pos, 1.));

            float lightDist = length(lightDirUN);
            float distAtt   = dot(vec3(gl_LightSource[index].constantAttenuation,
                                       gl_LightSource[index].linearAttenuation,
                                       gl_LightSource[index].quadraticAttenuation),
                                  vec3(1., lightDist, lightDist * lightDist)       );
            spotEffect = pow(spotEffect, gl_LightSource[index].spotExponent);
            distAtt    = spotEffect / distAtt;

            color = shadow * distAtt * NdotL * mDiff * gl_LightSource[index].diffuse;
        }
    }
    
    return color;
}


void main(void)
{
    vec3  norm  = texture2DRect(texBufNorm, gl_FragCoord.xy).xyz;

    if(dot(norm, norm) < 0.95)
    {
        discard;
    }
    else
    {
        vec4  posAmb = texture2DRect(texBufPos,  gl_FragCoord.xy);
        vec3  pos    = posAmb.xyz;
        float amb    = posAmb.w;
        vec4  mDiff  = texture2DRect(texBufDiff, gl_FragCoord.xy);

        gl_FragColor = computePointLight(0, pos, norm, mDiff);
    }
}
