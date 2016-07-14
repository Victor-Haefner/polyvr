#version 120

attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;
varying vec2 tcs;

void main(void) {
    gl_Position = osg_Vertex;
    gl_Position.z = -1.0;
    tcs = osg_MultiTexCoord0;
}
