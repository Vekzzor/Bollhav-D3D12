#pragma once

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
#include <vector>

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

// Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
	WCHAR fullName[50];
	if(swprintf_s(fullName, L"%s[%u]", name, index) > 0)
	{
		pObject->SetName(fullName);
	}
}
#else
inline void SetName(ID3D12Object*, LPCWSTR) {}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT) {}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x,n)
