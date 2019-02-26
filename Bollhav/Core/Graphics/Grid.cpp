#include "Grid.h"
#include "pch.h"

Grid::Grid(ID3D12Device4* _pDevice, ID3D12RootSignature* _pSignature, UINT _Width, UINT _Spacing)
{
	// Create the vertices
	std::vector<XMFLOAT3> vertices;
	int ww = _Width;
	for(int i = -ww; i <= ww; i += _Spacing)
	{
		float hwf = static_cast<float>(ww);
		float ii  = static_cast<float>(i);

		XMFLOAT3 p0{ii, 0, -hwf};
		XMFLOAT3 p1{ii, 0, hwf};

		XMFLOAT3 p2{-hwf, 0, ii};
		XMFLOAT3 p3{hwf, 0, ii};

		vertices.push_back(p0);
		vertices.push_back(p1);
		vertices.push_back(p2);
		vertices.push_back(p3);
	}
	// Create the vertex buffer
	VERTEX_BUFFER_DESC vbd;
	vbd.pData		  = reinterpret_cast<LPVOID>(vertices.data());
	vbd.StrideInBytes = sizeof(vertices[0]);
	vbd.SizeInBytes   = vbd.StrideInBytes * vertices.size();
	m_VertexBuffer.Create(_pDevice, &vbd);

	//For use for upload heap creation later. 
	m_vertexSize = vbd.SizeInBytes; 

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
