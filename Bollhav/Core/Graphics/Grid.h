#pragma once
#include <Core/Graphics/DX12/GraphicsPipelineState.h>
#include <Core/Graphics/DX12/VertexBuffer.h>

class Grid
{
public:
	Grid(ID3D12Device4* _pDevice, ID3D12RootSignature* _pSignature, UINT _Width, UINT _Spacing, CopyList* copyList);

	void Draw(ID3D12GraphicsCommandList* _pCmdList);

	VertexBuffer* GetVertexBuffer(); 

private:
	VertexBuffer m_VertexBuffer;
	GraphicsPipelineState m_Pipeline;
};