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


struct WindGridInfo {
   uint visiblePosInScreenX;
   uint visiblePosInScreenY;
   uint width;
   uint height;
   uint screenWidth;
   uint screenHeight;
   uint visibleWidth;
   uint visibleHeight;
};

struct WindGridPoint {
   float speed;
   uint angle;
};

struct ScreenPortion {
   vec2 topLeft;
   vec2 bottomRight;
   uint particle_count;
   uint id_num;
};

struct Vertex {
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
};


//layout(binding = 4) in uint vid;

layout (location = 0) in vec4 pos;


layout (binding = 1) uniform Vertex2 {
   Vertex list[2];
} vertexArray;

layout(binding = 2) uniform WindGridPoint2 {
   WindGridPoint list[2];
} windGridPoints;

layout (binding = 3) uniform WindGridInfo2 {
   WindGridInfo list[2];
} windGridInfo;

layout(binding =4) uniform ScreenPortion2{
   ScreenPortion list[2];
}portions;

vec2 normalizedPosToVisibleCoord(vec2 pos, WindGridInfo info) {
   float x = pos.x;
   float y = pos.y;
   x *=  1.0;
   y *= -1.0;
   x += 1;
   y += 1;
   return vec2((x / 2.0) * info.visibleWidth, (y / 2.0) * info.visibleHeight);
}

vec2 visibleCoordToParentViewCoord(vec2 pos, WindGridInfo info) {
   return vec2(pos.x + info.visiblePosInScreenX, pos.y + info.visiblePosInScreenY);
}

vec2 parentViewCoordToDataPoint(vec2 pos, WindGridInfo info) {
   float x = pos.x;
   float y = pos.y;
   float scaledX = info.screenWidth / info.width;
   float scaledY = info.screenHeight / info.height;
   return vec2(x / scaledX, y / scaledY);
}

//vec2 normalizedCoordToGridPoint(vec2 pos, WindGridInfo info) {
//   vec2 visiblePos = normalizedPosToVisibleCoord(pos, info);
//   vec2 parentPos = visibleCoordToParentViewCoord(visiblePos, info);
//   vec2 gridPoint = parentViewCoordToDataPoint(parentPos, info);
//   return gridPoint;
//}

void main() {
   gl_Position = pos;
   gl_PointSize = 10.0;
}
