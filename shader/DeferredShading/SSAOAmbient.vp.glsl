#version 120

attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;
varying mat4 uProjectionMat;
varying vec4 uPos;

void main(void) {
    gl_Position = osg_Vertex;
    gl_Position.xy = gl_Position.xy*0.5;
    uPos = gl_Position;
    uProjectionMat = gl_ProjectionMatrix;
}
