#version 400 compatibility

in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec2 osg_MultiTexCoord0;
in vec2 osg_MultiTexCoord1;

out vec3 v_Position;
out vec3 v_Normal;

out vec4 v_Color;
out vec2 v_UVCoord1;
out vec2 v_UVCoord2;

void main()
{
    vec4 pos = gl_ModelViewMatrix * osg_Vertex;
    v_Position = vec3(pos.xyz) / pos.w;

    v_Normal = normalize(gl_NormalMatrix * osg_Normal);

    v_UVCoord1 = osg_MultiTexCoord0;
    v_UVCoord2 = osg_MultiTexCoord1;
    v_Normal = gl_NormalMatrix * osg_Normal;
    v_Color = gl_Color;
    gl_Position    = gl_ModelViewProjectionMatrix*osg_Vertex;
}
