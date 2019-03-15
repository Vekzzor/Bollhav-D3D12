#pragma once

class ComputeQueue
{
public:
	ComputeQueue(ID3D12Device4* _pDevice);

	void SubmitList(ID3D12GraphicsCommandList* _pCmdList);
	void Execute();

	void WaitForGPU();
	void GetTimestampFrequency(UINT64* qFrec);

private:
	std::vector<ID3D12CommandList*> m_pCommandListsToExecute;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_FenceSignal;
	HANDLE m_hFenceEvent;
};