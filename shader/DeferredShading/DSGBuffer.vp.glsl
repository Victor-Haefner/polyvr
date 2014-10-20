#version 120

varying vec4 vertPos;
varying vec3 vertNorm;

void main(void)
{
    vertPos        = gl_ModelViewMatrix * gl_Vertex;
    vertNorm       = gl_NormalMatrix    * gl_Normal;

    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FrontColor  = gl_Color;
    gl_Position    = ftransform();
}
