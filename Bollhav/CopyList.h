#pragma once
#include <Core/Graphics/DX12/VertexBuffer.h>
class CopyList
{
private:
	ComPtr<ID3D12GraphicsCommandList> m_pCopyList;

public:
	CopyList(ID3D12Device4* device, ID3D12CommandAllocator* commandAlloc);

	void Prepare(ID3D12CommandAllocator* pAllocator, ID3D12Resource* vertexBuffer);
	void Finish();

	ID3D12GraphicsCommandList* operator->(void);
	ID3D12GraphicsCommandList* GetPtr();

	HRESULT
	CreateUploadHeap(ID3D12Device4* pDevice, UINT dataSize, ID3D12Resource1* UploadHeapResource);

	void ScheduleCopy(VertexBuffer* vertexData);
};