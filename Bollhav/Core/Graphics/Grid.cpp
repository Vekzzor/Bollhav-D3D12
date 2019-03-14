#include "Grid.h"
#include "pch.h"

Grid::Grid(ID3D12Device4* _pDevice, ID3D12RootSignature* _pSignature, UINT _Width, UINT _Spacing, DX12Heap* heap, UINT heapOffset, VERTEX_BUFFER_DESC vbDesc)
{
	// Create the vertices

	// Create the vertex buffer
	m_vbd.pData			  = vbDesc.pData;  //reinterpret_cast<LPVOID>(vertices.data());
	m_vbd.StrideInBytes = vbDesc.StrideInBytes;  //sizeof(vertices[0]);
	m_vbd.SizeInBytes	 = vbDesc.SizeInBytes;  //m_vbd.StrideInBytes * vertices.size();
	m_VertexBuffer.Create(_pDevice, &m_vbd,heap, heapOffset);

	//For use for upload heap creation later. 
	m_vertexSize = m_vbd.SizeInBytes;
	m_vertexData = m_vbd.pData; 

	// Create the pipelinestate
	m_Pipeline.SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	m_Pipeline.SetVertexShader(L"Shaders/LineVS.hlsl");
	m_Pipeline.SetPixelShader(L"Shaders/LinePS.hlsl");
	D3D12_RASTERIZER_DESC desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.AntialiasedLineEnable = true;
	m_Pipeline.SetRasterizerState(desc);
	m_Pipeline.Finalize(_pDevice, _pSignature);
}

void Grid::Draw(ID3D12GraphicsCommandList* _pCmdList)
{
	_pCmdList->SetPipelineState(m_Pipeline.GetPtr());
	_pCmdList->IASetVertexBuffers(0, 1, &m_VertexBuffer.GetVertexView());
	_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	_pCmdList->DrawInstanced(m_VertexBuffer.GetVertexCount(), 1, 0, 0);
}

VertexBuffer* Grid::GetVertexBuffer()
{
	return &m_VertexBuffer;
}

const UINT Grid::GetVertexSize() const
{
	return m_vertexSize; 
}

const LPVOID Grid::GetVertexData() const
{
	return m_vertexData;
}

const std::vector<XMFLOAT3>& Grid::GetDataVector() const
{
	return m_dataVector; 
}

const VERTEX_BUFFER_DESC& Grid::GetBufferDesc() const
{
	return m_vbd; 
}
