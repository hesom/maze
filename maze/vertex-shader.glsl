/*
 * Copyright (C) 2016 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#version 330

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 view_matrix;
uniform mat3 normal_matrix;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

out vec3 vnormal;
out vec3 vview;
out vec3 vlight;

const vec4 wlight = vec4(-10.0, -30.0, -10.0, 1.0);

void main(void)
{
    vec4 position = vec4(pos, 1.0);
    vnormal = normal_matrix * normal;
    vview = -(modelview_matrix * position).xyz;
    vlight = -(view_matrix * wlight).xyz;
    gl_Position = projection_matrix * modelview_matrix * position;
}
