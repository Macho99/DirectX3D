#ifndef _GRASS_FX_
#define _GRASS_FX_

struct GrassPosition
{
    float3 position;
    float padding;
};

struct NearbyGrassData
{
    float4x4 worldMatrix;
};

struct DistantGrassData
{
    float3 position;
};
#endif