#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

// compute directional light INDEX for fragment at POS with normal NORM
// and diffuse material color MDIFF
vec4 computeDirLight(int index, vec3 pos, vec3 norm, vec4 mDiff)
{
    vec4  color    = vec4(0., 0., 0., 0.);

    vec3  lightDir = gl_LightSource[index].position.xyz;
    float NdotL    = max(dot(norm, lightDir), 0.);

    if(NdotL > 0.)
    {
        color = NdotL * mDiff * gl_LightSource[index].diffuse;
    }

    return color;
}

// DS input buffers
uniform sampler2DRect     texBufPos;
uniform sampler2DRect     texBufNorm;
uniform sampler2DRect     texBufDiff;
uniform vec2              vpOffset;

// DS pass
void main(void)
{
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;

    if(dot(norm, norm) < 0.95)
    {
        discard;
    }
    else
    {
        vec4  posAmb = texture2DRect(texBufPos,  lookup);
        vec3  pos    = posAmb.xyz;
        float amb    = posAmb.w;
        vec4  mDiff  = texture2DRect(texBufDiff, lookup);

        gl_FragColor = computeDirLight(0, pos, norm, mDiff);
    }
}
