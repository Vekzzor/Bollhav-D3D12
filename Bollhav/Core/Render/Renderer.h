#pragma once

static const UINT NUMOFBUFFERS = 2; 

class Renderer
{
private:

	template<class Interface>
	inline void SafeRelease(
		Interface **ppInterfaceToRelease)
	{
		if (*ppInterfaceToRelease != NULL)
		{
			(*ppInterfaceToRelease)->Release();

			(*ppInterfaceToRelease) = NULL;
		}
	}

	IDXGIFactory6* m_factory = nullptr;
	ID3D12Device4* m_device4 = nullptr; 
	ID3D12CommandQueue* m_commandQueue = nullptr; 
	ID3D12CommandAllocator* m_commandAllocator = nullptr;
	ID3D12GraphicsCommandList3* m_commandList = nullptr;
	IDXGISwapChain1* m_swapChain1 = nullptr; 
	IDXGISwapChain4* m_swapChain4 = nullptr;
	ID3D12DescriptorHeap* m_renderTargetsHeap = nullptr; 
	ID3D12Resource1* m_renderTargetViews[NUMOFBUFFERS] = {};
	ID3D12RootSignature* m_rootSignature; 
	UINT m_renderTargetDescSize = 0;
	ID3D12PipelineState* m_graphPipelineState = nullptr; 

	//Make use of this in the right way, the way it is used 
	//now is not correct. 
	ID3D12Fence1* m_fence1 = nullptr; 
	UINT m_fenceValue; 
	HANDLE m_eventHandle; 

	D3D12_VIEWPORT m_viewPort; 
	D3D12_RECT m_scissorRect; 

	UINT m_width; 
	UINT m_height;
	

public:
	Renderer(); 
	~Renderer(); 
	
	void _init(const UINT wWidth, const UINT wHeight, const HWND& wndHandle);

	void CreateDevice(); 
	void CreateSwapChainAndCommandIterface(const HWND& whand); 
	void CreateFenceAndEventHadle(); 
	void CreateRenderTarget(); 
	void CreateViewportAndScissorRect(); 
	void CreateRootSignature(); 
	void CreatePiplelineStateAndShaders(); 

};