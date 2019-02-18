#pragma once
class GPUComputing
{
public:
	GPUComputing();
	~GPUComputing();
	int init(ID3D12Device4* device, ID3DBlob* computeBlob, ID3D12RootSignature* rootSignature);
	ID3D12CommandQueue* CommandQueue()
	{
		return m_pComputeQueue.Get();
	}
	ID3D12CommandAllocator* CommandAllocator()
	{
		return m_pComputeAllocator.Get();
	}
	ID3D12GraphicsCommandList* CommandList()
	{
		return m_pComputeList.Get();
	}
	ID3D12PipelineState* PipelineState()
	{
		return m_pPipelineState.Get();
	}

private:
	void m_CreateCommandInterface(ID3D12Device4* device);
	void m_CreatePipeLineState(ID3D12Device4* device,
							   ID3DBlob* computeBlob,
							   ID3D12RootSignature* rootSignature);

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pComputeQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pComputeAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pComputeList;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
};
