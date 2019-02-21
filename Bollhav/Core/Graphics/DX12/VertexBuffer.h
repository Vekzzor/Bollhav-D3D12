#pragma once

struct VERTEX_BUFFER_DESC
{
	void* pData;
	UINT SizeInBytes;
	UINT StrideInBytes;

};

class VertexBuffer
{
public:
	VertexBuffer(ID3D12Device4* _pDevice, VERTEX_BUFFER_DESC * _pDesc);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexView() const;

private:
	D3D12_VERTEX_BUFFER_VIEW m_BufferView;
	ComPtr<ID3D12Resource> m_pVertexData;
};