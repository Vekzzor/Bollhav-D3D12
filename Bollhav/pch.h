#pragma once

// DirectX
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "d3dcompiler.lib")

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

inline void ThrowIfFailed(HRESULT hr);
