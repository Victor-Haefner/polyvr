#version 120
#extension GL_ARB_texture_rectangle : require
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect texBufPos;
uniform sampler2DRect texBufNorm;
uniform sampler2DRect texBufDiff;
uniform vec2          vpOffset;
uniform int           channel;

void main(void) {
    vec2 lookup = gl_FragCoord.xy - vpOffset;
    vec4 norm   = texture2DRect(texBufNorm, lookup);

    if(dot(norm.xyz, norm.xyz) < 0.95) discard;
    else {
	vec4 color = vec4(0,1,1,1);
	if (channel == 0) {
    		bool isLit  = (norm.w > 0);
		if (isLit) color = vec4(0.02, 0.02, 0.02, 1.);
		else {
	  		color = texture2DRect(texBufDiff, lookup);
			color = vec4(color.xyz, 1.0);
		}
	}

	if (channel == 1) {
		vec4 posAmb = texture2DRect(texBufPos, lookup);
		color = vec4(posAmb.xyz, 1.0);
	}

	if (channel == 2) { color = vec4(norm.xyz, 1.0); }

	if (channel == 3) {
	  	color = texture2DRect(texBufDiff, lookup);
		color = vec4(color.xyz, 1.0);
	}

	if (channel == 4) {
		vec4 posAmb = texture2DRect(texBufPos, lookup);
		color = vec4(posAmb.w, posAmb.w, posAmb.w, 1.0);
	}

        gl_FragColor = color;
    }
}
