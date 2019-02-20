#pragma once

class GraphicsPipelineState
{
public:
	GraphicsPipelineState();

	void SetVertexShader(LPCWSTR _pFileName,
						 LPCSTR _pEntryPoint = "vs_main",
						 LPCSTR _pTarget	 = "vs_5_1");
	void
	SetPixelShader(LPCWSTR _pFileName, LPCSTR _pEntryPoint = "PS_main", LPCSTR _pTarget = "ps_5_0");

	void SetWireFrame(bool _val);

	void Finalize(ID3D12Device4* _pDevice, ID3D12RootSignature* _pRootSignature);
	ID3D12PipelineState* GetPtr() const;

private:
	void GetInputLayoutFromShader(std::vector<D3D12_INPUT_ELEMENT_DESC>* _pContainer,
								  std::vector<D3D12_SIGNATURE_PARAMETER_DESC>* _pContainer2);

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;

	ComPtr<ID3DBlob> m_pVertex;
	ComPtr<ID3DBlob> m_pPixel;

	ComPtr<ID3D12PipelineState> m_pPSO;
};