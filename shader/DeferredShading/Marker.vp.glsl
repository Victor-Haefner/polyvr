#version 120

attribute vec4 osg_Vertex;

void main(void) {
    gl_Position = osg_Vertex;
    gl_Position.z = -1.0;
}
