#version 120
//#extension GL_ARB_texture_rectangle : require
//#extension GL_ARB_texture_rectangle : enable

//uniform sampler2DRect  texBufNorm;
//uniform vec2           vpOffset;

void main(void) {
    //vec2 lookup = gl_FragCoord.xy - vpOffset;
    //vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;

    //gl_FragData[0] = vec4(1,1,0,1);
    //gl_FragData[1] = vec4(1,0,1,1);
    //gl_FragData[2] = vec4(0,1,1,1);
    //gl_FragData[3] = vec4(1,0,0,1);

    gl_FragColor = vec4(1,0,0,1);
}
