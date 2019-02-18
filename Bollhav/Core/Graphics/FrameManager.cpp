#include "FrameManager.h"

FrameManager::FrameManager(ID3D12Device4* _pDevice)
	: m_Frames{_pDevice, _pDevice, _pDevice}
	, m_iFrameIndex(0)
	, m_fenceLastSignaledValue(0)
{
	TIF(_pDevice->CreateCommandList(0,
									D3D12_COMMAND_LIST_TYPE_DIRECT,
									m_Frames[m_iFrameIndex].GetCommandAllocator(),
									nullptr,
									IID_PPV_ARGS(&m_pCommandList)));
	TIF(m_pCommandList->Close());

	TIF(_pDevice->CreateFence(m_iFrameIndex, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
	m_hFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

Frame* FrameManager::GetReadyFrame(Swapchain* _pSwapChain)
{
	UINT nextFrameIndex		 = m_iFrameIndex + 1;
	m_iFrameIndex			 = nextFrameIndex;
	HANDLE waitableObjects[] = {_pSwapChain->GetWaitObject(), NULL};
	DWORD numWaitableObjects = 1;

	Frame* currentFrame = &m_Frames[nextFrameIndex % NUM_BACKBUFFERS];
	UINT64 fenceValue   = currentFrame->GetFenceValue();
	if(fenceValue != 0) // Means no fence was signaled
	{
		currentFrame->SetFenceValue(0);
		TIF(m_pFence->SetEventOnCompletion(fenceValue, m_hFenceEvent));
		waitableObjects[1] = m_hFenceEvent;
		numWaitableObjects = 2;
	}

	WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);
	return currentFrame;
}

void FrameManager::SyncCommandQueue(Frame* _pFrame, ID3D12CommandQueue* _pQueue)
{

	UINT64 fenceValue = m_fenceLastSignaledValue + 1;
	TIF(_pQueue->Signal(m_pFence.Get(), fenceValue));
	m_fenceLastSignaledValue = fenceValue;
	_pFrame->SetFenceValue(fenceValue);
}
