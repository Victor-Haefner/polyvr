#version 120

attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;
varying mat4 uProjectionMat;

void main(void) {
    gl_Position = osg_Vertex;
    gl_Position.z = -1.0;
    uProjectionMat = gl_ProjectionMatrix;
}
