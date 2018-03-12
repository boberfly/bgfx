$input a_position, a_color0
$output v_position, v_color0


/*
 * Copyright 2011-2018 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

void main()
{
	v_color0 = a_color0;
	v_position = vec4(a_position, 1.0);	
}
