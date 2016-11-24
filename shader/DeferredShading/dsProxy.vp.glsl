#version 120

attribute vec4 osg_Vertex;

void main(void) {
    gl_Position = osg_Vertex;
    gl_Position.xy *= 1.1;
    gl_Position.z = -1;
    gl_Position.xy = gl_Position.xy;
}
