#pragma once

#include <Core/Graphics/Device.h>
#include <Core/Graphics/GraphicsCommandQueue.h>
#include <Core/Graphics/Swapchain.h>

class GraphicsCore
{
public:
	GraphicsCore();
	~GraphicsCore();

	ID3D12Device4* GetDevice() const;

private:
	Device m_Device;
	Swapchain m_Swapchain;
	GraphicsCommandQueue m_CommandQueue;
};
