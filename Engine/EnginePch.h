#pragma once

#define WIN32_LEAN_AND_MEAN

#include "Types.h"
#include "Define.h"

// STL
#include <memory>
#include <iostream>
#include <array>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

// WIN
#include <windows.h>
#include <assert.h>
#include <optional>

// DX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXTex/DirectXTex.h>
#include <DirectXTex/DirectXTex.inl>
using namespace DirectX;
using namespace Microsoft::WRL;

#include <FX11/d3dx11effect.h>

// Assimp
#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

// ImGUI
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

// Libs
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "DirectXTex/DirectXTex_debug.lib")
#pragma comment(lib, "FX11/Effects11d.lib")
#pragma comment(lib, "Assimp/assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "DirectXTex/DirectXTex.lib")
#pragma comment(lib, "FX11/Effects11.lib")
#pragma comment(lib, "Assimp/assimp-vc143-mt.lib")
#endif

// Managers
#include "Game.h"
#include "Texture.h"
#include "Graphics.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "SceneManager.h"

// Engine
#include "VertexData.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstantBuffer.h"
#include "Shader.h"
#include "IExecute.h"

#include "GameObject.h"
#include "Transform.h"
#include "Mesh.h"

#if defined(_DEBUG)

inline void DX_SetDebugName(
    ID3D11DeviceChild* obj,
    const char* file,
    int line
)
{
    if (!obj) return;

    std::string name;
    name += file;
    name += ":";
    name += std::to_string(line);

    obj->SetPrivateData(
        WKPDID_D3DDebugObjectName,
        (UINT)name.size(),
        name.c_str()
    );
}

#define DX_INTERNAL_CREATE(call, outObj) \
    CHECK(call); \
    DX_SetDebugName( \
        reinterpret_cast<ID3D11DeviceChild*>(outObj), \
        __FILE__, \
        __LINE__ \
    );

#else

#define DX_INTERNAL_CREATE(call, outObj) \
    CHECK(call);

#endif

#define DX_CREATE_BUFFER(desc, data, outBuffer) \
    outBuffer.Reset();\
    DX_INTERNAL_CREATE( \
        DEVICE->CreateBuffer( \
            (desc), (data), (outBuffer).GetAddressOf() \
        ), \
        (outBuffer).Get() \
    )

#define DX_CREATE_TEXTURE2D(desc, data, outTex) \
    outTex.Reset();\
    DX_INTERNAL_CREATE( \
        DEVICE->CreateTexture2D( \
            (desc), (data), (outTex).GetAddressOf() \
        ), \
        (outTex).Get() \
    )

#define DX_CREATE_SRV(resource, desc, outView) \
    outView.Reset();\
    DX_INTERNAL_CREATE( \
        DEVICE->CreateShaderResourceView( \
            (resource), (desc), (outView).GetAddressOf() \
        ), \
        (outView).Get() \
    )

#define DX_CREATE_RTV(resource, desc, outView) \
    outView.Reset();\
    DX_INTERNAL_CREATE( \
        DEVICE->CreateRenderTargetView( \
            (resource), (desc), (outView).GetAddressOf() \
        ), \
        (outView).Get() \
    )

#define DX_CREATE_DSV(resource, desc, outView) \
    outView.Reset();\
    DX_INTERNAL_CREATE( \
        DEVICE->CreateDepthStencilView( \
            (resource), (desc), (outView).GetAddressOf() \
        ), \
        (outView).Get() \
    )
#define DX_CREATE_UAV(resource, desc, outView) \
    outView.Reset();\
    DX_INTERNAL_CREATE( \
        DEVICE->CreateUnorderedAccessView( \
            (resource), (desc), (outView).GetAddressOf() \
        ), \
        (outView).Get() \
    )


//#if defined(_DEBUG)
//#define CreateBuffer                DO_NOT_USE_CreateBuffer
//#define CreateTexture2D             DO_NOT_USE_CreateTexture2D
//#define CreateShaderResourceView    DO_NOT_USE_CreateSRV
//#define CreateRenderTargetView      DO_NOT_USE_CreateRTV
//#define CreateDepthStencilView      DO_NOT_USE_CreateDSV
//#define CreateUnorderedAccessView   DO_NOT_USE_CreateUAV
//#endif
