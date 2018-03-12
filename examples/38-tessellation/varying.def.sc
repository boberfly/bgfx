vec3 a_position : POSITION;
vec4 a_color0 : COLOR0;

vec4 v_position : POSITION = vec4(0.0, 0.0, 0.0, 0.0);
vec4 v_color0 : COLOR = vec4(0.0, 0.0, 0.0, 0.0);


vec4 hs_position : SV_POSITION = vec4(0.0, 0.0, 0.0, 0.0);
vec4 hs_color0 : COLOR0 = vec4(1.0, 0.0, 0.0, 1.0);

vec4 ds_color0   : COLOR0;
vec3 ds_facetNormal : TEXCOORD0 = vec3(0.0, 0.0, 0.0);
vec3 ds_patchDistance : TEXCOORD1 = vec3(0.0, 0.0, 0.0);