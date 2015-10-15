#version 120

varying vec4      vertPos;
varying vec3      vertNorm;

uniform sampler2D tex0;

float luminance(vec4 color) { return dot(color, vec4(0.3, 0.59, 0.11, 0.0)); }

void main(void) {
    vec3 pos = vertPos.xyz / vertPos.w;
//     vec3 pos = vertPos.xyz;

    float ambVal  = luminance(gl_Color);
    //vec3  diffCol = texture2D(tex0, gl_TexCoord[0].xy).xyz;
    //vec3  diffCol = gl_FrontMaterial.diffuse.rgb;
    vec3  diffCol = gl_Color.rgb;

    gl_FragData[0] = vec4(pos, ambVal);
    gl_FragData[1] = vec4(normalize(vertNorm), 0);
    gl_FragData[2] = vec4(diffCol, 0);
}
