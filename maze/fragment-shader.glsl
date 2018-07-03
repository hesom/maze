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

uniform vec3 color;

in vec3 vnormal;
in vec3 vview;
in vec3 vlight;

layout(location = 0) out vec4 fcolor;

const float kd = 0.5;
const float ks = 0.5;
const float shininess = 120.0;

void main(void)
{
    vec3 n = normalize(vnormal);
    vec3 v = normalize(vview);
    vec3 l = normalize(vlight);

    vec3 h = normalize(l + v);

    float diffuse = kd * max(dot(l, n), 0.0);

    float specular = ks * pow(max(dot(h, n), 0.0), shininess);

    fcolor = vec4(color * vec3(0.2 + diffuse + specular), 1.0);
}
