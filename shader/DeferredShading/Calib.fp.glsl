#version 130

uniform int grid;
uniform vec2 OSGViewportSize;

void main(void) {
    ivec2 s = ivec2(OSGViewportSize.xy);
    ivec2 p = ivec2(gl_FragCoord.xy);
    vec4 c = vec4(0);
    if (p[0]%grid == 0 || p[1]%grid == 0) c = vec4(1); // grid
    if (p[0] == 0 || p[1] == 0) c = vec4(1);           // borders
    if (p[0] == s[0]-1 || p[1] == s[1]-1) c = vec4(1); // borders
    gl_FragColor = c;
}

