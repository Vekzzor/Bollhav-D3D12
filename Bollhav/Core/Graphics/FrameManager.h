#pragma once
#include <Core/Graphics/Frame.h>
#include <Core/Graphics/DX12/Swapchain.h>

class FrameManager
{
public:
	FrameManager(ID3D12Device4* _device);
	Frame* GetReadyFrame(Swapchain* _pSwapChain);
	void WaitForLastSubmittedFrame();
	void SyncCommandQueue(Frame* _pFrame, ID3D12CommandQueue* _pQueue);
	ID3D12Fence* GetFencePtr();

	UINT64 m_fenceLastSignaledValue;
	UINT m_iFrameIndex;
private:
	Frame m_Frames[NUM_BACKBUFFERS];

	HANDLE m_hFenceEvent;
	ComPtr<ID3D12Fence> m_pFence;
};
