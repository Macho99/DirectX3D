#pragma once
#include <windows.h>
#include "DirectXMath.h"
#include "SimpleMath.h"

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

using Color = DirectX::XMFLOAT4;
namespace Colors
{
	XMGLOBALCONST Color White = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMGLOBALCONST Color Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMGLOBALCONST Color Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	XMGLOBALCONST Color Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	XMGLOBALCONST Color Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	XMGLOBALCONST Color Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	XMGLOBALCONST Color Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	XMGLOBALCONST Color Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	XMGLOBALCONST Color Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
	XMGLOBALCONST Color LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
}

using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;
using Quaternion = DirectX::SimpleMath::Quaternion;
using Ray = DirectX::SimpleMath::Ray;

// MeshID / MaterialID
using InstanceID = std::pair<uint64, uint64>;
//using TransformID = int64;