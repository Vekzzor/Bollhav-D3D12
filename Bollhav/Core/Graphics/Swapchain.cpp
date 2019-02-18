#include "Swapchain.h"

Swapchain::Swapchain(ID3D12Device4* _device)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors				= NUM_BACK_BUFFERS;
	desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask					= 1;

	TIF(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pRTVDescHeap)));

	size_t rtvDescriptorSize =
		_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRTVDescHeap->GetCPUDescriptorHandleForHeapStart();
	for(UINT i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		m_RTDescriptor[i] = rtvHandle;
		rtvHandle.ptr += rtvDescriptorSize;
	}
}

void Swapchain::ResizeSwapchain(ID3D12CommandQueue* _pCommandQueue, UINT _width, UINT _height)
{

	// TODO(Henrik): Resizing
}

void Swapchain::Init(ID3D12Device4* _pDevice,
					 ID3D12CommandQueue* _pCommandQueue,
					 UINT _width,
					 UINT _height)
{
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.BufferCount			 = NUM_BACK_BUFFERS;
	sd.Width				 = _width;
	sd.Height				 = _height;
	sd.Format				 = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.Flags				 = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	sd.BufferUsage			 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count		 = 1;
	sd.SampleDesc.Quality	= 0;
	sd.SwapEffect			 = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode			 = DXGI_ALPHA_MODE_UNSPECIFIED;
	sd.Scaling				 = DXGI_SCALING_STRETCH;
	sd.Stereo				 = FALSE;

	ComPtr<IDXGIFactory4> factory;
	ComPtr<IDXGISwapChain1> swapChain1;
	TIF(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	TIF(factory->CreateSwapChainForHwnd(
		_pCommandQueue, GetActiveWindow(), &sd, nullptr, nullptr, &swapChain1));
	TIF(swapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapchain)));
	m_FrameIndex = m_pSwapchain->GetCurrentBackBufferIndex();

	m_pSwapchain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);

	m_hSwapChainWait = m_pSwapchain->GetFrameLatencyWaitableObject();
	assert(m_hSwapChainWait != nullptr);

	for(UINT i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		TIF(m_pSwapchain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTarget[i])));
		_pDevice->CreateRenderTargetView(m_pRenderTarget[i].Get(), nullptr, m_RTDescriptor[i]);
	}
}

void Swapchain::Present()
{
	TIF(m_pSwapchain->Present(1, 0));
	m_FrameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
}

HANDLE Swapchain::GetWaitObject() const
{
	return m_hSwapChainWait;
}

ID3D12Resource* Swapchain::GetCurrentRenderTarget() const
{
	return m_pRenderTarget[m_FrameIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetCurrentDescriptor() const
{
	return m_RTDescriptor[m_FrameIndex];
}
