#pragma once

class Swapchain
{
public:
	Swapchain(ID3D12Device4* _pDevice);

	void ResizeSwapchain(ID3D12CommandQueue* _pCommandQueue, UINT _width = 0, UINT _height = 0);
	void Init(ID3D12Device4* _pDevice,
			  ID3D12CommandQueue* _pCommandQueue,
			  UINT _width  = 0,
			  UINT _height = 0);

	void Present();

	HANDLE GetWaitObject() const;
	ID3D12Resource* GetCurrentRenderTarget() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDescriptor() const;

private:
	static const int NUM_BACK_BUFFERS = 3;

	UINT m_FrameIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTDescriptor[NUM_BACK_BUFFERS] = {};
	ComPtr<ID3D12Resource> m_pRenderTarget[NUM_BACK_BUFFERS];
	ComPtr<ID3D12DescriptorHeap> m_pRTVDescHeap;

	ComPtr<IDXGISwapChain4> m_pSwapchain;
	HANDLE m_hSwapChainWait;
};