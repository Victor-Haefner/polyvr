#version 130

uniform int grid;
uniform int isRightEye;
uniform vec2 OSGViewportSize;

bool inRect(vec2 p, vec2 mi, vec2 ma) {
    if (p[0] < mi[0] || p[1] < mi[1]) return false;
    if (p[0] > ma[0] || p[1] > ma[1]) return false;
    return true;
}

vec4 drawL(vec2 p, vec2 mi, vec2 ma, vec4 fg, vec4 bg) {
    vec2 s = ma-mi;
    p -= mi;
    if (inRect(p, vec2(0.3*s[0], 0.2*s[1]), vec2(0.4*s[0], 0.8*s[1]))) return fg;
    if (inRect(p, vec2(0.3*s[0], 0.2*s[1]), vec2(0.7*s[0], 0.3*s[1]))) return fg;
    return bg;
}

vec4 drawR(vec2 p, vec2 mi, vec2 ma, vec4 fg, vec4 bg) {
    vec2 s = ma-mi;
    p -= mi;
    if (inRect(p, vec2(0.3*s[0], 0.2*s[1]), vec2(0.4*s[0], 0.8*s[1]))) return fg;
    if (inRect(p, vec2(0.3*s[0], 0.7*s[1]), vec2(0.7*s[0], 0.8*s[1]))) return fg;
    if (inRect(p, vec2(0.3*s[0], 0.45*s[1]), vec2(0.7*s[0], 0.55*s[1]))) return fg;
    if (inRect(p, vec2(0.6*s[0], 0.45*s[1]), vec2(0.7*s[0], 0.8*s[1]))) return fg;
    if (inRect(p, vec2(0.6*s[0] - p[1]*0.15, 0.2*s[1]), vec2(0.7*s[0] - p[1]*0.15, 0.45*s[1]))) return fg;
    return bg;
}

void main(void) {
    ivec2 s = ivec2(OSGViewportSize.xy);
    ivec2 p = ivec2(gl_FragCoord.xy);

    //if (p.x > s.x*0.5) p.x = s.x-p.x;
    //if (p.y > s.y*0.5) p.y = s.y-p.y;

    vec4 blue = vec4(0.5,0.5,1.0,1.0);
    vec4 green = vec4(0.5,1.0,0.5,1.0);
    vec4 white = vec4(1.0);
    vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 grey = vec4(0.3, 0.3, 0.3, 1.0);

    vec4 c = vec4(0);
    int mx = int(s[0]*0.5);
    int my = int(s[1]*0.5);
    if (p[0]%grid == 0 || p[1]%grid == 0) c = white; // grid
    if (p[0] == 0 || p[1] == 0) c = blue;           // borders
    if (p[0] == s[0]-1 || p[1] == s[1]-1) c = blue; // borders
    if (p[0] == mx || p[1] == my) c = green; // middle

    if ( inRect(p, vec2(0.4*s[0], 0.2*s[1]), vec2(0.6*s[0], 0.3*s[1])) ) {
	c = black;
	if (isRightEye == 0 && p[0] < s[0]*0.5) c = drawL(p, vec2(0.4*s[0], 0.2*s[1]), vec2(0.5*s[0], 0.3*s[1]), white,blue);
	if (isRightEye == 1 && p[0] > s[0]*0.5) c = drawR(p, vec2(0.5*s[0], 0.2*s[1]), vec2(0.6*s[0], 0.3*s[1]), white,green);
    }
    if ( inRect(p, vec2(0.4*s[0], 0.7*s[1]), vec2(0.6*s[0], 0.8*s[1])) ) {
	c = black;
	if (isRightEye == 0 && p[0] < s[0]*0.5) c = drawL(p, vec2(0.4*s[0], 0.7*s[1]), vec2(0.5*s[0], 0.8*s[1]), white,blue);
	if (isRightEye == 1 && p[0] > s[0]*0.5) c = drawR(p, vec2(0.5*s[0], 0.7*s[1]), vec2(0.6*s[0], 0.8*s[1]), white,green);
    }

    gl_FragColor = c;
}

