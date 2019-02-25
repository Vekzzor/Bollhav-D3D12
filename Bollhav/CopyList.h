#pragma once
#include "CopyList.h"
class CopyList
{
private:
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCopyList;
	ComPtr<ID3D12Resource> m_pUploadHeapResource;
	std::vector<ID3D12CommandList*> executeList;

public:
	CopyList(ID3D12Device4* device);

	void Prepare(ID3D12Resource* vertexBuffer);
	void Finish(ID3D12Resource* VertexData);

	ID3D12GraphicsCommandList* operator->(void);
	ID3D12GraphicsCommandList* GetPtr();

	ComPtr<ID3D12GraphicsCommandList> GetList();
	ComPtr<ID3D12Resource> GetUploadHeap(); 
	
	HRESULT CreateUploadHeap(ID3D12Device4* pDevice, UINT dataSize);

	//Non-Vertex buffer solution. 
	void ScheduleCopy(ID3D12Resource* copyDest, ID3D12Resource* uploadHeapRes, D3D12_SUBRESOURCE_DATA copyData);

	void ExecuteCopy(ID3D12CommandQueue* commandQueue);

	void ResetCopyAllocator();
	void ResetCopyList();
};