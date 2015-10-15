#version 120
#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect  texBufNorm;
uniform vec2           vpOffset;

void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;

    if(dot(norm, norm) < 0.95) discard;
    else gl_FragColor = vec4(0.02, 0.02, 0.02, 1.);
}
