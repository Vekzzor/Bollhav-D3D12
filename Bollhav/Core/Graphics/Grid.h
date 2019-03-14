#pragma once
#include <Core/Graphics/DX12/GraphicsPipelineState.h>
#include <Core/Graphics/DX12/VertexBuffer.h>

class Grid
{
public:
	Grid(ID3D12Device4* _pDevice, ID3D12RootSignature* _pSignature, UINT _Width, UINT _Spacing, DX12Heap* heap, UINT heapOffset, VERTEX_BUFFER_DESC vbDesc);

	void Draw(ID3D12GraphicsCommandList* _pCmdList);

	VertexBuffer* GetVertexBuffer(); 

	const UINT GetVertexSize() const; 

	const LPVOID GetVertexData() const; 

	const std::vector<XMFLOAT3>& GetDataVector() const; 

	const VERTEX_BUFFER_DESC& GetBufferDesc() const; 

private:
	VertexBuffer m_VertexBuffer;
	GraphicsPipelineState m_Pipeline;

	UINT m_vertexSize; 
	LPVOID m_vertexData; 

	VERTEX_BUFFER_DESC m_vbd;

	std::vector<XMFLOAT3> m_dataVector; 
};