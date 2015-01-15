#version 120

//----------------------------------------------------------------------------------------------MAIN--FP

uniform sampler2D texture;

in vec2 texCoord;
in float light;

void main( void )
{
   gl_FragColor = texture2D(texture, texCoord)*light;
   gl_FragColor.a = 1.0;
}
