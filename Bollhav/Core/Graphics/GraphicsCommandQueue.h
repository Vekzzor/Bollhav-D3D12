#pragma once

class GraphicsCommandQueue
{
public:
	GraphicsCommandQueue(ID3D12Device4* _device);

	void SubmitList(ID3D12CommandList* _pCommandList);
	void Execute();
	ID3D12CommandQueue* GetCommandQueue() const;

private:
	std::vector<ID3D12CommandList*> m_pExecuteList;
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
};
