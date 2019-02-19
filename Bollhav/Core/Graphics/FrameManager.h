#pragma once
#include <Core/Graphics/Frame.h>
#include <Core/Graphics/Swapchain.h>

class FrameManager
{
public:
	FrameManager(ID3D12Device4* _device);
	Frame* GetReadyFrame(Swapchain* _pSwapChain);

	void SyncCommandQueue(Frame* _pFrame, ID3D12CommandQueue* _pQueue);
private:
	Frame m_Frames[NUM_BACKBUFFERS];
	UINT m_iFrameIndex;
	UINT64 m_fenceLastSignaledValue;

	HANDLE m_hFenceEvent;
	ComPtr<ID3D12Fence> m_pFence;
};
