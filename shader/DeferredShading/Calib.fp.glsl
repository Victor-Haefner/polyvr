#version 130

uniform int grid;
uniform vec2 OSGViewportSize;

void main(void) {
    ivec2 s = ivec2(OSGViewportSize.xy);
    ivec2 p = ivec2(gl_FragCoord.xy);

    //if (p.x > s.x*0.5) p.x = s.x-p.x;
    //if (p.y > s.y*0.5) p.y = s.y-p.y;
    p = s - p - ivec2(1,1);

    vec4 blue = vec4(0.5,0.5,1.0,1.0);
    vec4 green = vec4(0.5,1.0,0.5,1.0);
    vec4 white = vec4(1.0);

    vec4 c = vec4(0);
    if (p[0]%grid == 0 || p[1]%grid == 0) c = white; // grid
    if (p[0] == 0 || p[1] == 0) c = blue;           // borders
    if (p[0] == s[0]-1 || p[1] == s[1]-1) c = blue; // borders
    if (p[0] == s[0]*0.5 || p[1] == s[1]*0.5) c = green; // middle
    gl_FragColor = c;
}

