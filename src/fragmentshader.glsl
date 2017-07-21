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

smooth in vec3 vnormal;
smooth in vec3 vlight;
smooth in vec3 vview;

layout(location = 0) out vec4 fcolor;

void main(void)
{
    vec3 normal = normalize(vnormal);
    vec3 light = normalize(vlight);
    vec3 view = normalize(vview);
    vec3 halfv = normalize(light + view);
    float d = max(dot(light, normal), 0.0);
    vec3 diffuse = 0.5 * d * vec3(1.0);
    vec3 specular = vec3(0.0);
    if (d > 0.0)
        specular = 0.5 * pow(max(dot(halfv, normal), 0.0), 100.0) * vec3(1.0);
    fcolor = vec4(color * (0.2 + diffuse + specular), 1.0);
}
