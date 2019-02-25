#pragma once
#include <Core/Graphics/DX12/VertexBuffer.h>
class CopyList
{
private:
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCopyList;
	ComPtr<ID3D12Resource> m_pUploadHeapResource;
	std::vector<ID3D12CommandList*> executeList;

public:
	CopyList(ID3D12Device4* device, ID3D12CommandAllocator* commandAlloc);

	void Prepare(ID3D12Resource* vertexBuffer);
	void Finish(ID3D12Resource* VertexData);

	ID3D12GraphicsCommandList* operator->(void);
	ID3D12GraphicsCommandList* GetPtr();

	HRESULT
	CreateUploadHeap(ID3D12Device4* pDevice, UINT dataSize);

	void ScheduleCopy(VertexBuffer* vertexData);
	void ExecuteCopy(ID3D12CommandQueue* commandQueue);

	void ResetCopyAllocator();
	void ResetCopyList();

	//void WaitForGPU();
};