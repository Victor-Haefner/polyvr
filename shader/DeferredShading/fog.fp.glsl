#version 120

#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform vec4 fogParams;
uniform vec4 fogColor;

uniform int uBlurSize; // use size of noise texture

void main(void) {
    vec2 lookup = gl_FragCoord.xy;
    vec4 posAmb = texture2DRect(texBufPos,  lookup);
    vec4 norm   = texture2DRect(texBufNorm, lookup);
    vec4 mDiff  = texture2DRect(texBufDiff, lookup);

    vec3 pos = posAmb.xyz;
    if (pos.z >= 0) discard;
    bool isLit = (norm.w > 0);
    float amb = posAmb.w;
    float lit = norm.w;

    float z = abs(pos.z);
    if (z > fogParams[1] && fogParams[0] > 0.5 && isLit) {
	float t = 0;
	if (fogParams[0] < 1.5) { // linear
		t = (z-fogParams[1])/(fogParams[2]-fogParams[1]);
		t = clamp(pow(t,fogParams[0]), 0, 1);
	}
	else if (fogParams[0] < 2.5) { // exponential
		t = clamp(exp((z-fogParams[1])*fogParams[3])-1.0, 0, 1);
	}

	mDiff = mix(mDiff, fogColor, t);
	norm.xyz = normalize(mix(norm.xyz, vec3(0,0,1), t));
	lit = 1.0-t;
    }

    gl_FragData[0] = vec4(pos, amb);
    gl_FragData[1] = vec4(norm.xyz,lit);
    gl_FragData[2] = vec4(mDiff.xyz,1);
}
