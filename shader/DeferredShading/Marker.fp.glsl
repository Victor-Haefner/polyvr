#version 130

/*

TODO: pass the markers with uniforms

*/

uniform int grid;
uniform int isRightEye;
uniform vec2 OSGViewportSize;

bool inRect(vec2 p, vec2 mi, vec2 ma) {
    if (p[0] < mi[0] || p[1] < mi[1]) return false;
    if (p[0] > ma[0] || p[1] > ma[1]) return false;
    return true;
}

vec4 decode(int m, int i, int j, vec4 fg, vec4 bg) {
    int k = j*4+(3-i);
    int bit = (m & ( 1 << k )) >> k;
    if (bit == 1) return fg;
    else return bg;
}

vec4 drawMarker(vec2 p, int m, vec2 pos, float size, vec4 fg, vec4 bg) {
    float si = size*0.125; // /8

    // white border
    if (inRect(p, pos, pos + vec2(size-si, si) )) return fg;
    if (inRect(p, pos, pos + vec2(si, size-si) )) return fg;
    if (inRect(p, pos + vec2(0, size-si), pos + vec2(size, size) )) return fg;
    if (inRect(p, pos + vec2(size-si, 0), pos + vec2(size, size) )) return fg;

    // black border
    if (inRect(p, pos + vec2(si, si), pos + vec2(size-si, 2*si) )) return bg;
    if (inRect(p, pos + vec2(si, si), pos + vec2(2*si, size-si) )) return bg;
    if (inRect(p, pos + vec2(si, size-2*si), pos + vec2(size-si, size-si) )) return bg;
    if (inRect(p, pos + vec2(size-2*si, si), pos + vec2(size-si, size-si) )) return bg;

    // grid
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            if (inRect(p, pos + vec2((2+i)*si, (2+j)*si), pos + vec2((3+i)*si, (3+j)*si) )) return decode(m, i, j, fg, bg);
        }
    }

    return bg;
}

void main(void) {
    ivec2 s = ivec2(OSGViewportSize.xy);
    ivec2 p = ivec2(gl_FragCoord.xy);

    vec4 white = vec4(1.0);
    vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
    float msize = min(0.1*s[0], 0.1*s[1]);

    vec4 c = vec4(0);
    if ( inRect(p, vec2(0, 0),                   vec2(msize, msize) ) ) c = drawMarker(p, 46386, vec2(0, 0), msize, white, black);
    if ( inRect(p, vec2(0, s[1]-msize),          vec2(msize, s[1]) ) )  c = drawMarker(p, 3994, vec2(0, s[1]-msize), msize, white, black);
    if ( inRect(p, vec2(s[0]-msize, s[1]-msize), vec2(s[0], s[1]) ) )   c = drawMarker(p, 13101, vec2(s[0]-msize, s[1]-msize), msize, white, black);
    if ( inRect(p, vec2(s[0]-msize, 0),          vec2(s[0], msize) ) )  c = drawMarker(p, 39238, vec2(s[0]-msize, 0), msize, white, black);
    gl_FragColor = c;
}

