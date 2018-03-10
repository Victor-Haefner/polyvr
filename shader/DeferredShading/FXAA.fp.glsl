#version 430 core
layout (location = 0) out vec4 color;
layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform sampler2D tex;
uniform vec2 OSGViewportSize;
in vec2 tcs;

void main( void ) {
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0/8.0;
    float FXAA_REDUCE_MIN = 1.0/128.0;

    vec3 rgbNW=texture2D(tex, tcs+(vec2(-1.0,-1.0)/OSGViewportSize)).xyz;
    vec3 rgbNE=texture2D(tex, tcs+(vec2(1.0,-1.0)/OSGViewportSize)).xyz;
    vec3 rgbSW=texture2D(tex, tcs+(vec2(-1.0,1.0)/OSGViewportSize)).xyz;
    vec3 rgbSE=texture2D(tex, tcs+(vec2(1.0,1.0)/OSGViewportSize)).xyz;
    vec3 rgbM =texture2D(tex, tcs).xyz;

    vec3 luma=vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max( (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2( -FXAA_SPAN_MAX,  -FXAA_SPAN_MAX), vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX)) / OSGViewportSize;

    vec3 rgbA = mix( texture2D(tex, tcs - dir * 0.17).xyz, texture2D(tex, tcs + dir * 0.17).xyz, 0.5);
    vec3 rgbB = mix( texture2D(tex, tcs - dir * 0.5 ).xyz, texture2D(tex, tcs + dir * 0.5 ).xyz, 0.5) * 0.5 + rgbA * 0.5;
    float lumaB = dot(rgbB, luma);

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) color.xyz = rgbA;
    else color.xyz = rgbB;
}


