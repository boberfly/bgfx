$input v_position, v_color0
$output hs_position, hs_color0
$layout triangles, 3, equal_spacing, cw

uniform vec4 TessLevel;

#define ID gl_InvocationID

void main_perpatch()
{
    gl_TessLevelInner[0] = TessLevel.x;
    gl_TessLevelOuter[0] = TessLevel.y;
    gl_TessLevelOuter[1] = TessLevel.y;
    gl_TessLevelOuter[2] = TessLevel.y;
}

void main()
{
    hs_position[ID] = v_position[ID];
    hs_color0[ID] = v_color0[ID];
}