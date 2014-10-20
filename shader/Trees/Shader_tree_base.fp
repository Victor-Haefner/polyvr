//----------------------------------------------------------------------------------------------MAIN--FP

vec4 fvAmbient  = vec4(0.36, 0.36, 0.36, 1.0);
//vec4 fvSpecular = vec4(0.7,  0.7,  0.7,  1.0);
vec4 fvSpecular = vec4(0.3,  0.3,  0.3,  1.0);
vec4 fvDiffuse  = vec4(0.5,  0.5,  0.5,  1.0);
//float fSpecularPower = 25.0;
float fSpecularPower = 10.0;

uniform sampler2D texture;

varying in vec3 ViewDirection;
varying in vec3 fvObjectPosition;
varying in vec3 Normal;
varying in vec2 texCoord;

void main( void )
{
   //gl_FragColor = gl_Color;
   //Normal = vec3(0,0,1.0);


   vec3  fvLightDirection = normalize( gl_LightSource[0].position.xyz - fvObjectPosition.xyz);
   vec3  fvNormal         = normalize( Normal );
   float fNDotL           = dot( fvNormal, fvLightDirection );

   vec3  fvReflection     = normalize( ( ( 2.0 * fvNormal ) * fNDotL ) - fvLightDirection );
   vec3  fvViewDirection  = normalize( ViewDirection );
   float fRDotV           = max( 0.0, dot( fvReflection, fvViewDirection ) );

   vec4  fvBaseColor      = texture2D(texture, texCoord);

   vec4  fvTotalAmbient   = fvAmbient * fvBaseColor;
   vec4  fvTotalDiffuse   = fvDiffuse * fNDotL * fvBaseColor;
   vec4  fvTotalSpecular  = fvSpecular * ( pow( fRDotV, fSpecularPower ) );

   gl_FragColor = ( fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular );

   //gl_FragColor = vec4(fvNormal[0], fvNormal[1], fvNormal[2], 1.0);
}
