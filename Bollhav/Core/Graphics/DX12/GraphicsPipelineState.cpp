#include "GraphicsPipelineState.h"
#include "pch.h"

GraphicsPipelineState::GraphicsPipelineState()

{
	m_PSODesc = {};
	//NOTE(Henrik): Make this declariations into functions when needed

	m_PSODesc.RasterizerState				   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	m_PSODesc.BlendState					   = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	m_PSODesc.DepthStencilState.DepthEnable	= TRUE;
	m_PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	m_PSODesc.DepthStencilState.DepthFunc	  = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	m_PSODesc.DepthStencilState.StencilEnable  = FALSE;
	m_PSODesc.DSVFormat						   = DXGI_FORMAT_D32_FLOAT;
	m_PSODesc.SampleMask					   = UINT_MAX;
	m_PSODesc.PrimitiveTopologyType			   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_PSODesc.NumRenderTargets				   = 1;
	m_PSODesc.RTVFormats[0]					   = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_PSODesc.SampleDesc.Count				   = 1;
}

void GraphicsPipelineState::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC _Desc)
{
	m_PSODesc.DepthStencilState = _Desc;
}

void GraphicsPipelineState::SetRasterizerState(D3D12_RASTERIZER_DESC _Desc)
{
	m_PSODesc.RasterizerState = _Desc;
}

void GraphicsPipelineState::SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE _Topology)
{
	m_PSODesc.PrimitiveTopologyType = _Topology;
}

void GraphicsPipelineState::SetVertexShader(LPCWSTR _pFileName,
											LPCSTR _pEntryPoint,
											LPCSTR _pTarget)
{
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

	ComPtr<ID3DBlob> error;
	TIF(D3DCompileFromFile(
		_pFileName, nullptr, nullptr, _pEntryPoint, _pTarget, compileFlags, 0, &m_pVertex, &error));
	if(error)
	{
		std::cerr << "Error: " << (char*)error->GetBufferPointer() << std::endl;
	}
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

void GraphicsPipelineState::SetWireFrame(bool _val)
{
	m_PSODesc.RasterizerState.FillMode = _val ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

void GraphicsPipelineState::Finalize(ID3D12Device4* _pDevice, ID3D12RootSignature* _pRootSignature)
{
	// Fill in the rest of the desc
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayouts;
	ComPtr<ID3D12ShaderReflection> pShaderReflection;
	TIF(D3DReflect(m_pVertex->GetBufferPointer(),
				   m_pVertex->GetBufferSize(),
				   IID_PPV_ARGS(&pShaderReflection)));

	D3D12_SHADER_DESC shaderDesc;
	pShaderReflection->GetDesc(&shaderDesc);

	inputLayouts.resize(shaderDesc.InputParameters);

	for(unsigned int i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC desc;
		pShaderReflection->GetInputParameterDesc(i, &desc);

		inputLayouts[i].SemanticName		 = desc.SemanticName;
		inputLayouts[i].SemanticIndex		 = desc.SemanticIndex;
		inputLayouts[i].InputSlot			 = 0;
		inputLayouts[i].AlignedByteOffset	= 0;
		inputLayouts[i].InputSlotClass		 = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputLayouts[i].InstanceDataStepRate = 0;

		std::cout << "Name: " << inputLayouts[i].SemanticName << std::endl;

		// determine DXGI format
		if(desc.Mask == 1)
		{
			if(desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32_UINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32_SINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if(desc.Mask <= 3)
		{
			if(desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32_UINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32_SINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if(desc.Mask <= 7)
		{
			if(desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32B32_UINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32B32_SINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if(desc.Mask <= 15)
		{
			if(desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if(desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				inputLayouts[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}

	m_PSODesc.InputLayout.NumElements		 = static_cast<UINT>(inputLayouts.size());
	m_PSODesc.InputLayout.pInputElementDescs = inputLayouts.data();
	m_PSODesc.pRootSignature				 = _pRootSignature;
	std::cout << (m_PSODesc.RasterizerState.FillMode == D3D12_FILL_MODE_WIREFRAME) << std::endl;
	TIF(_pDevice->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_pPSO)));

	NAME_D3D12_OBJECT(m_pPSO);
}

ID3D12PipelineState* GraphicsPipelineState::GetPtr() const
{
	return m_pPSO.Get();
}

void GraphicsPipelineState::GetInputLayoutFromShader(
	std::vector<D3D12_INPUT_ELEMENT_DESC>* _pContainer,
	std::vector<D3D12_SIGNATURE_PARAMETER_DESC>* _pContainer2)
{}
