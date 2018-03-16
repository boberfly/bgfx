$input hs_color0
$output ds_facetNormal, ds_patchDistance
$layout triangles, 3, equal_spacing, cw

#include <bgfx_shader.sh>

void main()
{
	vec3 p0 = gl_TessCoord.x * getHullPosition(0).xyz;
	vec3 p1 = gl_TessCoord.y * getHullPosition(1).xyz;
	vec3 p2 = gl_TessCoord.z * getHullPosition(2).xyz;

	vec3 tePosition = normalize(p0 + p1 + p2);

	ds_patchDistance = gl_TessCoord;
	ds_facetNormal = tePosition;

	gl_Position = mul(u_modelViewProj, vec4(tePosition, 1.0) );
}
