#include "GraphicsPipelineState.h"
#include "pch.h"

GraphicsPipelineState::GraphicsPipelineState()
	
{
	m_PSODesc = {};
	//NOTE(Henrik): Make this declariations into functions when needed


	m_PSODesc.RasterizerState					   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	m_PSODesc.BlendState						   = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	m_PSODesc.DepthStencilState.DepthEnable	  = FALSE;
	m_PSODesc.DepthStencilState.StencilEnable	= FALSE;
	m_PSODesc.SampleMask						   = UINT_MAX;
	m_PSODesc.PrimitiveTopologyType			   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_PSODesc.NumRenderTargets				   = 1;
	m_PSODesc.RTVFormats[0]					   = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_PSODesc.SampleDesc.Count				   = 1;
	
}

void GraphicsPipelineState::SetVertexShader(LPCWSTR _pFileName,
											LPCSTR _pEntryPoint,
											LPCSTR _pTarget)
{
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	ComPtr<ID3DBlob> error;
	TIF(D3DCompileFromFile(
		_pFileName, nullptr, nullptr, _pEntryPoint, _pTarget, compileFlags, 0, &m_pVertex, &error));

	m_PSODesc.VS.pShaderBytecode = m_pVertex->GetBufferPointer();
	m_PSODesc.VS.BytecodeLength  = m_pVertex->GetBufferSize();
}

void GraphicsPipelineState::SetPixelShader(LPCWSTR _pFileName, LPCSTR _pEntryPoint, LPCSTR _pTarget)
{
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	ComPtr<ID3DBlob> error;
	TIF(D3DCompileFromFile(
		_pFileName, nullptr, nullptr, _pEntryPoint, _pTarget, compileFlags, 0, &m_pPixel, &error));

	m_PSODesc.PS.pShaderBytecode = m_pPixel->GetBufferPointer();
	m_PSODesc.PS.BytecodeLength  = m_pPixel->GetBufferSize();
}

void GraphicsPipelineState::Finalize(ID3D12Device4* _pDevice, ID3D12RootSignature* _pRootSignature)
{
	// Fill in the rest of the desc
	
	GetInputLayoutFromShader(&inputLayouts);
	
	m_PSODesc.InputLayout.NumElements = inputLayouts.size();
	m_PSODesc.InputLayout.pInputElementDescs= inputLayouts.data();

	m_PSODesc.pRootSignature = _pRootSignature;

	TIF(_pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pPSO)));
	NAME_D3D12_OBJECT(m_pPSO);
}

ID3D12PipelineState* GraphicsPipelineState::GetPtr() const
{
	return m_pPSO.Get();
}

void GraphicsPipelineState::GetInputLayoutFromShader(
	std::vector<D3D12_INPUT_ELEMENT_DESC>* _pContainer)
{
	ComPtr<ID3D12ShaderReflection> pShaderReflection;
	TIF(D3DReflect(m_pVertex->GetBufferPointer(),
				   m_pVertex->GetBufferSize(),
				   IID_PPV_ARGS(&pShaderReflection)));

	D3D12_SHADER_DESC shaderDesc;
	pShaderReflection->GetDesc(&shaderDesc);

	_pContainer->resize(shaderDesc.InputParameters);

	for(unsigned int i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
		pShaderReflection->GetInputParameterDesc(i, &paramDesc);

		D3D12_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName		 = paramDesc.SemanticName;
		elementDesc.SemanticIndex		 = paramDesc.SemanticIndex;
		elementDesc.InputSlot			 = 0;
		elementDesc.AlignedByteOffset	= 0;
		elementDesc.InputSlotClass		 = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if(paramDesc.Mask == 1)
		{
			if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if(paramDesc.Mask <= 3)
		{
			if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if(paramDesc.Mask <= 7)
		{
			if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if(paramDesc.Mask <= 15)
		{
			if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if(paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		_pContainer->at(i) = elementDesc;
	}
}
