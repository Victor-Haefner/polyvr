#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;

uniform int uBlurSize; // use size of noise texture

void main(void) {
    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);

    vec3 pos = posAmb.xyz;
    float amb = posAmb.w;

    float result = 0.0;
    for (int i = 0; i < uBlurSize; ++i) {
        for (int j = 0; j < uBlurSize; ++j) {
            //vec2 offset = (hlim + vec2(float(x), float(y))) * texelSize;
            //result += texture2DRect(uTexInput, vTexcoord + offset).r;
            amb = texture2DRect(texBufPos,  lookup + vec2(i,j)).w;
            result += amb;
        }
    }

    amb = result / float(uBlurSize * uBlurSize);


    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff);
}
