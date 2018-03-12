$input ds_facetNormal, ds_patchDistance

#define LightPosition vec3(0.25, 0.25, 1)
#define DiffuseMaterial vec3(0, 0.75, 0.75)
#define AmbientMaterial vec3(0.04, 0.04, 0.04)

/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

float amplify(float d, float scale, float offset)
{
	d = scale * d + offset;
	d = clamp(d, 0, 1);
	d = 1 - exp2(-2 * d*d);
	return d;
}

void main()
{
	vec3 N = normalize(ds_facetNormal);
	vec3 L = LightPosition;
	float df = abs(dot(N, L));
	vec3 color = AmbientMaterial + df * DiffuseMaterial;

	float d2 = min(min(ds_patchDistance.x, ds_patchDistance.y), ds_patchDistance.z);
	color = amplify(d2, 60, -0.5) * color;    

	gl_FragColor = vec4(color, 1.0);
}
