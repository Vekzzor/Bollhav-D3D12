#include "FrameManager.h"

FrameManager::FrameManager(ID3D12Device4* _pDevice)
	: m_iFrameIndex(0)
	, m_fenceLastSignaledValue(0)
{
	for(int i = 0; i < NUM_BACKBUFFERS; i++)
		m_Frames[i] = Frame(_pDevice);
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

void FrameManager::WaitForLastSubmittedFrame()
{
	Frame* frameCtxt = &m_Frames[m_iFrameIndex % NUM_BACKBUFFERS];

	UINT64 fenceValue = frameCtxt->GetFenceValue();
	if(fenceValue == 0)
		return; // No fence was signaled

	frameCtxt->SetFenceValue( 0);
	if(m_pFence->GetCompletedValue() >= fenceValue)
		return;

	m_pFence->SetEventOnCompletion(fenceValue, m_hFenceEvent);
	WaitForSingleObject(m_hFenceEvent, INFINITE);
}

void FrameManager::SyncCommandQueue(Frame* _pFrame, ID3D12CommandQueue* _pQueue)
{

	UINT64 fenceValue = m_fenceLastSignaledValue + 1;
	TIF(_pQueue->Signal(m_pFence.Get(), fenceValue));
	m_fenceLastSignaledValue = fenceValue;
	_pFrame->SetFenceValue(fenceValue);
}
