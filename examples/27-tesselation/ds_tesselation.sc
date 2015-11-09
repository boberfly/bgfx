layout(triangles, equal_spacing, cw) in;

in vec3 hs_position[];
in vec4 hs_color0[];

out vec4 ds_color0;
out vec3 ds_facetNormal;
out vec3 ds_patchDistance;

#include "../common/common.sh"

void main()
{
    vec3 p0 = gl_TessCoord.x * hs_position[0];
    vec3 p1 = gl_TessCoord.y * hs_position[1];
    vec3 p2 = gl_TessCoord.z * hs_position[2];    

    ds_color0 = gl_TessCoord.x * hs_color0[0] 
              + gl_TessCoord.y * hs_color0[1] 
              + gl_TessCoord.z * hs_color0[2];

    vec3 tePosition = normalize(p0 + p1 + p2);
    
    ds_patchDistance = gl_TessCoord;
    ds_facetNormal = tePosition;
    gl_Position = mul(u_modelViewProj, vec4(tePosition, 1.0) );
}