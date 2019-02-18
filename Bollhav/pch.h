#ifndef PCH_H
#define PCH_H

// DirectX
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "d3dcompiler.lib")

constexpr int NUM_BACKBUFFERS = 3;

#include <wrl.h>
using namespace Microsoft::WRL;

// Window
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#	define NOMINMAX
#endif
#include <windows.h>

// Utility
#include <assert.h>
#include <comdef.h>
#include <functional> // For std::function
#include <iostream>
#include <string>

// ImGui
#include <Utility/imgui.h>
#include <Utility/imgui_impl_dx12.h>
#include <Utility/imgui_impl_win32.h>

inline void TIF(HRESULT hr)
{
#if _DEBUG
	if(FAILED(hr))
	{
		_com_error err(hr);
		char s_str[256] = {};
		sprintf_s(s_str, "%s", err.ErrorMessage());
		throw std::runtime_error(s_str);
	}
#endif
}

#endif