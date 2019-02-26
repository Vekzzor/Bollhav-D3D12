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
	VertexBuffer(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc);

	void Create(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC* _pDesc);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexView() const;
	UINT GetVertexCount() const;
	ID3D12Resource* GetBufferResource();
	const D3D12_SUBRESOURCE_DATA& GetBufferData() const; 

private:
	UINT m_VertexCount;
	D3D12_VERTEX_BUFFER_VIEW m_BufferView;
	ComPtr<ID3D12Resource> m_pVertexData;
	D3D12_SUBRESOURCE_DATA m_vbData;
};