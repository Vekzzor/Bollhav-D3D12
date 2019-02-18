#include "GraphicsCore.h"

GraphicsCore::GraphicsCore()
	: m_Device()
	, m_Swapchain(m_Device.GetDevice())
	, m_CommandQueue(m_Device.GetDevice())
{
	m_Swapchain.Init(m_Device.GetDevice(), m_CommandQueue.GetCommandQueue());
}

GraphicsCore::~GraphicsCore() {}

ID3D12Device4* GraphicsCore::GetDevice() const
{
	return m_Device.GetDevice();
}

