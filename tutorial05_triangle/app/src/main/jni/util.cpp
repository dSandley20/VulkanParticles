//
// Created by Jason Carpenter on 2022/03/28.
//

// Define vector types to match iOS code
#include <ext/vector_float2.hpp>
#include <ext/vector_float3.hpp>
#include <ext/vector_float4.hpp>
#include <ext/vector_int2.hpp>
#include <ext/vector_int3.hpp>
#include <ext/vector_int4.hpp>
#include <ext/vector_uint2.hpp>
#include <ext/vector_uint3.hpp>
#include <ext/vector_uint4.hpp>
#include <ext/matrix_float4x3.hpp>
// Struct definitions
#define vector_float2 glm::vec2
#define vector_float3 glm::vec3
#define vector_float4 glm::vec4
#define vector_int2 glm::ivec2
#define vector_int3 glm::ivec3
#define vector_int4 glm::ivec4
#define vector_uint2 glm::uvec2
#define vector_uint3 glm::uvec3
#define vector_uint4 glm::uvec4

// Type definitions
#define float2 glm::vec2
#define float3 glm::vec3
#define float4 glm::vec4
#define int2 glm::ivec2
#define int3 glm::ivec3
#define int4 glm::ivec4
#define uint2 glm::uvec2
#define uint3 glm::uvec3
#define uint4 glm::uvec4
#define float4x3 glm::mat4x3
#define int4x3 glm::imat4x3
#define uint4x3 glm::umat4x3
#define half float
#define half4 glm::vec4

// Shader Definitions
struct ScreenPortion {
    vector_float2 topLeft;
    vector_float2 bottomRight;
    uint particle_count;
    uint id_num;
};

struct Vertex {
    vector_float4 color;
    vector_float2 pos;
    float speed;
    vector_float2 velocity;
    vector_float2 normalizedVelocity;
    vector_int3 rng;
    uint lifetime;
    uint kill;
    uint screenPortion_id;
    uint index;
};

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

#define pi_approx 3.1415926
#define fps_grad_avg 20
#define num_particles 1000
#define wind_scale 0.1

struct VertexOut {
    float4 color;
    float4 pos [[position]];
    float pointSize [[point_size]];
};

WindGridPoint getWindGridPointData(uint x, uint y, WindGridInfo windGridInfo, WindGridPoint* windGrid) {
    WindGridInfo info = windGridInfo;
    WindGridPoint wgp = windGrid[(info.width * y) + x];
    return wgp;
}

float degreeToRadians(float degrees) {
    return degrees * (pi_approx / 180);
}

float2 angleToHeadingVector(uint angle)
{
    float radians = angle * pi_approx / 180;
    return float2(cos(radians), sin(radians));
}

float2 velocityToUVComponents(int degrees, float speed) {
    // To receive interpolatable rotational degrees, we must seperate the
    // angle into it's U and V values. U and V are the values that would
    // be graphed as X and Y values respectively
    float u = sin(degrees * (pi_approx / 180));
    float v = cos(degrees * (pi_approx / 180));
    return float2(u,v);
}

float uvComponentstoVelocity(float3 uvs) {
    return (atan2(uvs.x, uvs.y) * 360 / 2 / pi_approx) + 180;
}

float uvComponentstoVelocity(float2 uv) {
    return (atan2(uv.x, uv.y) * 360 / 2 / pi_approx) + 180;
}

// pXX is the point near the given coordinates, where 0 = low and 1 = high
float3 bilinearInterpolateVector(float2 pos, float2 p00, float2 p10, float2 p01, float2 p11) {
    float rx = (1.0 - pos.x);
    float ry = (1.0 - pos.y);
    float a = rx * ry;
    float b = pos.x * ry;
    float c = rx * pos.y;
    float d = pos.x * pos.y;
    float u = (p00.x * a) + (p10.x * b) + (p01.x * c) + (p11.x * d);
    float v = (p00.y * a) + (p10.y * b) + (p01.y * c) + (p11.y * d);
    return float3(u, v, sqrt((u * u) + (v * v)));
}

float3 getUVsOfAbsolutePosition(int2 position, WindGridPoint* windGrid, WindGridInfo info) {
    // Get UV of a known point
    WindGridPoint data_point = getWindGridPointData(uint(position.x), uint(position.y), info, windGrid);
    float2 uv = angleToHeadingVector(data_point.angle + 270);
    return float3(uv.x, uv.y, data_point.speed);
}

float4x3 getUVsOfRelativePosition(float4 positions, WindGridPoint* windGrid, WindGridInfo info) {
    // Get the values of the surrounding 4 corner coordinates
    // Values are returned as UV values, described in uvComponents function
    int lowX = positions[0];
    int lowY = positions[1];
    int highX = positions[2];
    int highY = positions[3];
    float3 tl_uvs = getUVsOfAbsolutePosition(int2(lowX, lowY), windGrid, info);       // Top Left Coord;      Relative 0,0
    float3 tr_uvs = getUVsOfAbsolutePosition(int2(highX, lowY), windGrid, info);      // Top Right Coord;     Relative 1,0
    float3 bl_uvs = getUVsOfAbsolutePosition(int2(lowX, highY), windGrid, info);      // Bottom Left Coord;   Relative 0,1
    float3 br_uvs = getUVsOfAbsolutePosition(int2(highX, highY), windGrid, info);     // Bottom Right Coord;  Relative 1,1

    return float4x3(tl_uvs, tr_uvs, bl_uvs, br_uvs);
}

float3 interpolateVelocityToHeadingVector(float2 position, WindGridPoint* windGrid, WindGridInfo info) {
    // position is the adjusted position to fit the grid. X is 0-127, Y is 0-68

    float lowX = floor(position.x);
    float lowY = floor(position.y);
    float highX = ceil(position.x);
    float highY = ceil(position.y);

    float percentageX = position.x - lowX;
    float percentageY = position.y - lowY;

    float4x3 uvs = getUVsOfRelativePosition(float4(lowX, lowY, highX, highY), windGrid, info);

    float3 topLeftUVS = uvs[0];      // 0,0
    float3 topRightUVS = uvs[1];     // 1,0
    float3 bottomLeftUVS = uvs[2];   // 0,1
    float3 bottomRightUVS = uvs[3];  // 1,1
    const uint U = 0; // U value
    const uint V = 1; // V value
    const uint S = 2; // Speed value

    float interpolatedXU1 = (topLeftUVS[U] * (1.0 - percentageX)) + (topRightUVS[U] * percentageX);
    float interpolatedXU2 = (bottomLeftUVS[U] * (1.0 - percentageX)) + (bottomRightUVS[U] * percentageX);
    float interpolatedU = (interpolatedXU1 * (1.0 - percentageY)) + (interpolatedXU2 * percentageY);

    float interpolatedXV1 = (topLeftUVS[V] * (1.0 - percentageX)) + (topRightUVS[V] * percentageX);
    float interpolatedXV2 = (bottomLeftUVS[V] * (1.0 - percentageX)) + (bottomRightUVS[V] * percentageX);
    float interpolatedV = (interpolatedXV1 * (1.0 - percentageY)) + (interpolatedXV2 * percentageY);

    float interpolatedXS1 = (topLeftUVS[S] * (1.0 - percentageX)) + (topRightUVS[S] * percentageX);
    float interpolatedXS2 = (bottomLeftUVS[S] * (1.0 - percentageX)) + (bottomRightUVS[S] * percentageX);
    float interpolatedS = (interpolatedXS1 * (1.0 - percentageY)) + (interpolatedXS2 * percentageY);

    float3 resUVS = float3(interpolatedV, interpolatedU, interpolatedS);
    return resUVS;
}

float2 pointPositionInScreen(int width, int height) {
    float pointWidth = float(width) / 127;
    float pointHeight = float(height) / 68;
    return float2(pointWidth, pointHeight);
}

half4 hexToFloat(uint redHex, uint greenHex, uint blueHex, float alpha) {
    float red = (float(redHex) / float(0xFFu));
    float green = (float(greenHex) / float(0xFFu));
    float blue = (float(blueHex) / float(0xFFu));

    return half4(red, green, blue, alpha);
}

half4 speedColor(float speed)
{
    float low = 100.0;
    float high = 100.0;
    half4 lowColor = hexToFloat(0x2bu, 0x97u, 0x37u, 1.0);
    half4 highColor = hexToFloat(0x2bu, 0x97u, 0x37u, 1.0);
    if (speed <= 5.0) {
        low = 0.001;
        high = 5.0;
        lowColor = hexToFloat(0x2bu, 0x97u, 0x37u, 1.0);
        highColor = hexToFloat(0x2bu, 0x97u, 0x37u, 1.0);
    } else if (speed > 5.0 && speed <= 10.0) {
        low = 5.0;
        high = 10.0;
        lowColor = hexToFloat(0x2bu, 0x97u, 0x37u, 1.0);
        highColor = hexToFloat(0xadu, 0xe2u, 0x3eu, 1.0);
    } else if (speed > 10.0 && speed <= 20.0) {
        low = 10.0;
        high = 20.0;
        lowColor = hexToFloat(0xadu, 0xe2u, 0x3eu, 1.0);
        highColor = hexToFloat(0xf9u, 0xf4u, 0x38u, 1.0);
    } else if (speed > 20.0 && speed <= 30.0) {
        low = 20.0;
        high = 30.0;
        lowColor = hexToFloat(0xf9u, 0xf4u, 0x38u, 1.0);
        highColor = hexToFloat(0xf9u, 0xafu, 0x2cu, 1.0);
    } else if (speed > 30.0 && speed <= 40.0) {
        low = 30.0;
        high = 40.0;
        lowColor = hexToFloat(0xf9u, 0xafu, 0x2cu, 1.0);
        highColor = hexToFloat(0xfeu, 0x56u, 0x1fu, 1.0);
    } else if (speed > 40.0 && speed <= 60.0) {
        low = 40.0;
        high = 60.0;
        lowColor = hexToFloat(0xfeu, 0x56u, 0x1fu, 1.0);
        highColor = hexToFloat(0xe5u, 0x1du, 0x1du, 1.0);
    } else if (speed > 60.0) {
        low = 60.0;
        high = 100000.0;
        lowColor = hexToFloat(0xe5u, 0x1du, 0x1du, 1.0);
        highColor = hexToFloat(0xe5u, 0x1du, 0x1du, 1.0);
    }
    float adjustedHigh = high - low;
    float adjustedSpeed = high - speed;
    float ratio = adjustedSpeed / adjustedHigh;

    int lowRed = int(lowColor.r * 255);
    int lowGreen = int(lowColor.g * 255);
    int lowBlue = int(lowColor.b * 255);

    int highRed = int(highColor.r * 255);
    int highGreen = int(highColor.g * 255);
    int highBlue = int(highColor.b * 255);

    half red = ((highRed * ratio) + (lowRed * (1.0 - ratio))) / 255.0;
    half green = ((highGreen * ratio) + (lowGreen * (1.0 - ratio))) / 255.0;
    half blue = ((highBlue * ratio) + (lowBlue * (1.0 - ratio))) / 255.0;

    return half4(red, green, blue, 1.0);
}

float2 posToScreenWidth(float2 pos, WindGridInfo info)
{
    float x = (info.visibleWidth * ((pos.x + 1) / 2)) + (info.visiblePosInScreenX);
    float y = (((info.visibleHeight * ((pos.y + 1) / 2)) + (info.visiblePosInScreenY)) * -1) + info.screenHeight;
    return float2(x, y);
}

float2 screenWidthToPos(float2 pos, WindGridInfo info)
{
    float2 adjustedPos = pos - float2(info.visiblePosInScreenX, info.visiblePosInScreenY);
    float2 inverseViewSize = 1 / float2(info.visibleWidth, info.visibleHeight);
    float clipX = (2.0f * pos.x * inverseViewSize.x) - 1.0f;
    float clipY = ((2.0f * pos.y * inverseViewSize.y) - 1.0f) * -1;

    return float2(clipX, clipY);
}

float2 normalizeVelocity(float2 vel, WindGridInfo info)
{
    return vel / 350.0;
    //return float2(vel.x / info.width, vel.y / info.height);
}

// Conversion methods between the parent view, the visible subview, and the metal view coordinate systems
float2 normalizedPosToVisibleCoord(float2 pos, WindGridInfo info) {
    float x = pos.x;
    float y = pos.y;
    x *=  1.0;
    y *= -1.0;
    x += 1;
    y += 1;
    return float2((x / 2.0) * info.visibleWidth, (y / 2.0) * info.visibleHeight);
}

float2 visibleCoordToParentViewCoord(float2 pos, WindGridInfo info) {
    return float2(pos.x + info.visiblePosInScreenX, pos.y + info.visiblePosInScreenY);
}

float2 parentViewCoordToDataPoint(float2 pos, WindGridInfo info) {
    float x = pos.x;
    float y = pos.y;
    float scaledX = info.screenWidth / info.width;
    float scaledY = info.screenHeight / info.height;
    return float2(x / scaledX, y / scaledY);
}

float2 dataPointToParentViewCoord(float2 pos, WindGridInfo info) {
    float x = pos.x;
    float y = pos.y;
    float scaledX = info.screenWidth / info.width;
    float scaledY = info.screenHeight / info.height;
    return float2(x * scaledX, y * scaledY);
}

float2 parentViewCoordToVisibleCoord(float2 pos, WindGridInfo info) {
    return float2(pos.x - info.visiblePosInScreenX, pos.y - info.visiblePosInScreenY);
}

float2 visibleCoordToNormalizedCoord(float2 pos, WindGridInfo info) {
    float x = pos.x / (info.visibleWidth / 2.0);
    float y = pos.y / (info.visibleHeight / 2.0);
    x -=  1.0;
    y -=  1.0;
    x *=  1.0;
    y *= -1.0;
    return float2(x, y);
}

float2 normalizedCoordToGridPoint(float2 pos, WindGridInfo info) {
    float2 visiblePos = normalizedPosToVisibleCoord(pos, info);
    float2 parentPos = visibleCoordToParentViewCoord(visiblePos, info);
    float2 gridPoint = parentViewCoordToDataPoint(parentPos, info);
    return gridPoint;
}

float2 normalizedCoordToParentCoord(float2 pos, WindGridInfo info) {
    float2 visiblePos = normalizedPosToVisibleCoord(pos, info);
    float2 parentPos = visibleCoordToParentViewCoord(visiblePos, info);
    return parentPos;
}

float2 parentCoordToNormalizedCoord(float2 pos, WindGridInfo info) {
    float2 visiblePos = parentViewCoordToVisibleCoord(pos, info);
    float2 normalizedCoord = visibleCoordToNormalizedCoord(visiblePos, info);
    return normalizedCoord;
}