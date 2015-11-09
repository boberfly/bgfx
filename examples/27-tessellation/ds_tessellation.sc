$input hs_position, hs_color0
$output ds_color0, ds_facetNormal, ds_patchDistance

layout(triangles, equal_spacing, cw) in;

#include "../common/common.sh"

void main()
{
    vec3 p0 = gl_TessCoord.x * hs_position[0].xyz;
    vec3 p1 = gl_TessCoord.y * hs_position[1].xyz;
    vec3 p2 = gl_TessCoord.z * hs_position[2].xyz;    

    ds_color0 = gl_TessCoord.x * hs_color0[0] 
              + gl_TessCoord.y * hs_color0[1] 
              + gl_TessCoord.z * hs_color0[2];

    vec3 tePosition = normalize(p0 + p1 + p2);
    
    ds_patchDistance = gl_TessCoord;    
    ds_facetNormal = tePosition;
    gl_Position = mul(u_modelViewProj, vec4(tePosition, 1.0) );
}