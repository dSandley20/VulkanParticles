// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform  Vertex {
   vec4 color;
   vec2 pos;
   float speed;
   vec2 velocity;
   vec2 normalizedVelocity;
   ivec3 rng;
   uint lifetime;
   uint kill;
   uint screenPortion_id;
   uint index;
}
vertexArray;

layout(binding = 1) uniform  WindGridPoint {
   float speed;
   uint angle;
} windGridPoints;

layout(binding = 2) uniform WindGridInfo {
   uint visiblePosInScreenX;
   uint visiblePosInScreenY;
   uint width;
   uint height;
   uint screenWidth;
   uint screenHeight;
   uint visibleWidth;
   uint visibleHeight;
} windGridInfo;

layout(binding = 3) uniform ScreenPortion {
   vec2 topLeft;
   vec2 bottomRight;
   uint particle_count;
   uint id_num;
} portions;


//layout(binding = 4) in uint vid;

layout (location = 0) in vec4 pos;

void main() {
   gl_Position = pos;
   gl_PointSize = 10.0;
}
