#version 120

attribute vec4 osg_Vertex;
varying vec2 coords;

void main(void) {
    gl_FrontColor = gl_Color;
    gl_Position = osg_Vertex;
    gl_Position.z = -1.0;
    coords = 0.5*osg_Vertex.xy/osg_Vertex.w + 0.5;
}
