#pragma once

class VertexBuffer
{
public:
	VertexBuffer(ID3D12Device4* _pDevice,void* _pData, size_t _dataSize);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexView() const;

private:
	D3D12_VERTEX_BUFFER_VIEW m_BufferView;
	ComPtr<ID3D12Resource> m_pVertexData;
};