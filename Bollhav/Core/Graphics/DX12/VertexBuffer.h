#pragma once
#include "CopyList.h"
#include <pch.h>
struct VERTEX_BUFFER_DESC
{
	LPVOID pData;
	UINT SizeInBytes;
	UINT StrideInBytes;
};

class VertexBuffer
{
public:
	VertexBuffer() = default;
	VertexBuffer(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc, DX12Heap * heap, UINT heapOffset);

	void Create(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc, DX12Heap* heap, UINT heapOffset);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexView() const;
	UINT GetVertexCount() const;
	ID3D12Resource* GetBufferResource();
	ComPtr<ID3D12Resource> GetBufferResourceComPtr();
	const D3D12_SUBRESOURCE_DATA& GetBufferData() const; 
	D3D12_RESOURCE_DESC& GetResourceDesc(); 

private:
	D3D12_RESOURCE_DESC m_desc; 

	UINT m_VertexCount;
	D3D12_VERTEX_BUFFER_VIEW m_BufferView;
	ComPtr<ID3D12Resource> m_pVertexData;
	D3D12_SUBRESOURCE_DATA m_vbData;
};