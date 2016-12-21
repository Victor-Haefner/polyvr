#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform vec2 direction;

uniform int uBlurSize; // use size of noise texture

void main(void) {
    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec3 norm   = texture2DRect(texBufNorm, lookup).xyz;
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);

    vec3 pos = posAmb.xyz;
    if (pos.z >= 0) discard;
    float amb = posAmb.w;

    int S = int(uBlurSize*0.5);

    float result = 0.0;
    vec4 sample;
    int N = 0;
    for (int i = -S; i <= S; ++i) {
		sample = texture2DRect(texBufPos,  lookup + i*direction);
		if (sample.z < 0) {
			result += sample.w;
			N++;
		}
    }

    amb = result / float(N);

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm,1);
    gl_FragData[2] = vec4(mDiff);
}
