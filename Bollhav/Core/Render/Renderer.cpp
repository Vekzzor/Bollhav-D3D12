#include "Renderer.h"

Renderer::Renderer() {}

Renderer::~Renderer()
{
	SafeRelease(&m_factory);
	SafeRelease(&m_device4);
	SafeRelease(&m_commandList);
	SafeRelease(&m_commandQueue);
	SafeRelease(&m_commandAllocator);
	SafeRelease(&m_swapChain1);
	SafeRelease(&m_fence1);
	SafeRelease(&m_renderTargetsHeap);

	for(auto rt : m_renderTargetViews)
	{
		SafeRelease(&rt);
	}

	SafeRelease(&m_rootSignature);
}

void Renderer::_init(const UINT wWidth, const UINT wHeight, const HWND& wndHandle)
{
	m_width  = wWidth;
	m_height = wHeight;

	CreateDevice();
	CreateSwapChainAndCommandIterface(wndHandle);
	CreateFenceAndEventHadle();
	CreateRenderTarget();
	CreateViewportAndScissorRect();
	CreateRootSignature();
	CreatePiplelineStateAndShaders(); 
}

void Renderer::CreateDevice()
{
	//Enable the debug layer
	ID3D12Debug* debugController = nullptr;

	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SafeRelease(&debugController);

	//dxgi1_6 is only needed for initialization using an adapter.
	IDXGIAdapter1* adapter = nullptr;

	//The factory is used to create device which we connect to the graphics card (adapter)
	//Here, we want to find the adapter we want to use
	CreateDXGIFactory(IID_PPV_ARGS(&m_factory));

	for(UINT adapterIndex = 0;; ++adapterIndex)
	{
		adapter = nullptr;

		if(DXGI_ERROR_NOT_FOUND == m_factory->EnumAdapters1(adapterIndex, &adapter))
		{
			break; //No more adapters to enumerate.
		}

		//Now we have found an adapter (hopefully) we now need to check if it supports DX12.
		//Note that we pass nullptr in this create function, which means we are only testing
		//if it has support, and not actually creating a device yet.

		if(SUCCEEDED(
			   D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(m_device4), nullptr)))
		{
			break;
		}
		SafeRelease(&adapter);
	}
	if(adapter)
	{
		HRESULT hr = S_OK;
		//Create the actual device.
		if(SUCCEEDED(
			   hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device4))))
		{
		}

		SafeRelease(&adapter);
	}
	//Here we could implement an option to use a software if no adapyter was found
	//We dont want this right now though.
}

void Renderer::CreateSwapChainAndCommandIterface(const HWND& whand)
{
	HRESULT hr;
	//Describe and create the command queue
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	hr = m_device4->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_commandQueue));

	//Create command allocator. The command allocator corresponds
	//to the underlying allocations in which GPU commands are stored.
	hr = m_device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
										   IID_PPV_ARGS(&m_commandAllocator));

	//Create command list.
	hr = m_device4->CreateCommandList(0,
									  D3D12_COMMAND_LIST_TYPE_DIRECT,
									  m_commandAllocator,
									  nullptr,
									  IID_PPV_ARGS(&m_commandList));

	//When creating a command list, it is originally open. The main loop expects
	//it to be closed and we do not wish to record anything right now,
	//so we close it.
	m_commandList->Close();

	//Create a swap chain
	DXGI_SWAP_CHAIN_DESC1 swapDesc;
	swapDesc.Width				= 0;
	swapDesc.Height				= 0;
	swapDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.Stereo				= FALSE;
	swapDesc.SampleDesc.Count   = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.BufferCount		= NUMOFBUFFERS;
	swapDesc.Scaling			= DXGI_SCALING_NONE;
	swapDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Flags				= 0;
	swapDesc.AlphaMode			= DXGI_ALPHA_MODE_UNSPECIFIED;

	if(SUCCEEDED(hr = m_factory->CreateSwapChainForHwnd(
					 m_commandQueue, whand, &swapDesc, nullptr, nullptr, &m_swapChain1)))

		//Make the swapchain act as if it were a swapChain4?
		//Presuambly because the wanted crate function is only possible
		//through a swapChain1.
		if(SUCCEEDED(m_swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain4))))
		{
			m_swapChain4->Release();
		}
}

void Renderer::CreateFenceAndEventHadle()
{
	HRESULT hr;

	hr			 = m_device4->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence1));
	m_fenceValue = 1;

	//Create an event handle to use for GPU syncrhonization.
	//(Research this)////////////////////////////////////////////////////
	m_eventHandle = CreateEvent(0, false, false, 0);
}

void Renderer::CreateRenderTarget()
{
	//Create the descriptor heap neccessary for the render target views
	D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
	dhDesc.NumDescriptors			  = NUMOFBUFFERS;
	dhDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	HRESULT hr = m_device4->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_renderTargetsHeap));

	//Get the increment size of an RTV.
	m_renderTargetDescSize =
		m_device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//The pointer starts at the start of the heap for the render target views.
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHand =
		m_renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	//One Render Target View for each frame.
	for(UINT n = 0; n < NUMOFBUFFERS; n++)
	{
		hr = m_swapChain4->GetBuffer(n, IID_PPV_ARGS(&m_renderTargetViews[n]));
		m_device4->CreateRenderTargetView(m_renderTargetViews[n], nullptr, cpuDescHand);

		//Offset pointer so that it points at the place where the next RTV is to be in the heap.
		cpuDescHand.ptr += m_renderTargetDescSize;
	}
}

void Renderer::CreateViewportAndScissorRect()
{
	m_viewPort.TopLeftX = 0.0f;
	m_viewPort.TopLeftY = 0.0f;
	m_viewPort.Width	= (float)m_width;
	m_viewPort.Height   = (float)m_height;
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	m_scissorRect.left   = (long)0;
	m_scissorRect.right  = (long)m_width;
	m_scissorRect.top	= (long)0;
	m_scissorRect.bottom = (long)m_height;
}

void Renderer::CreateRootSignature()
{
	//Define descriptor range(s)
	D3D12_DESCRIPTOR_RANGE dtRanges[1];
	dtRanges[0].RangeType		   = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	dtRanges[0].NumDescriptors	 = 1; //Only one CB
	dtRanges[0].BaseShaderRegister = 0; //This means register b0
	dtRanges[0].RegisterSpace	  = 0; //b0, space0
	dtRanges[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; //Automatic append between slots in the heap.

	//Create a descriptor table to store the CB in.
	//This could be stored directly in the root, but for practice,
	//we will store it in a descriptor table.
	D3D12_ROOT_DESCRIPTOR_TABLE dt;
	dt.NumDescriptorRanges = ARRAYSIZE(dtRanges);
	dt.pDescriptorRanges   = dtRanges;

	//Create root parameter
	D3D12_ROOT_PARAMETER rootParam[1] = {};
	rootParam[0].ParameterType		  = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable	  = dt;
	rootParam[0].ShaderVisibility	 = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags			 = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters	 = ARRAYSIZE(rootParam);
	rsDesc.pParameters		 = rootParam;
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers   = nullptr;

	//Packaging
	ID3DBlob* sBlob;
	D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sBlob, nullptr);

	HRESULT hr = m_device4->CreateRootSignature(
		0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
}

void Renderer::CreatePiplelineStateAndShaders() 
{
	HRESULT hr;

	////// Shader Compiles //////
	ID3DBlob* vertexBlob;
	hr = D3DCompileFromFile(
		L"Shaders/VertexShader.hlsl", // filename
		nullptr, // optional macros
		nullptr, // optional include files
		"VS_main", // entry point
		"vs_5_0", // shader model (target)
		0, // shader compile options			// here DEBUGGING OPTIONS
		0, // effect compile options
		&vertexBlob, // double pointer to ID3DBlob
		nullptr // pointer for Error Blob messages.
		// how to use the Error blob, see here
		// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);

	ID3DBlob* pixelBlob;
	hr = D3DCompileFromFile(
		L"Shaders/PixelShader.hlsl", // filename
		nullptr, // optional macros
		nullptr, // optional include files
		"PS_main", // entry point
		"ps_5_0", // shader model (target)
		0, // shader compile options			// here DEBUGGING OPTIONS
		0, // effect compile options
		&pixelBlob, // double pointer to ID3DBlob
		nullptr // pointer for Error Blob messages.
		// how to use the Error blob, see here
		// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);

	////// Input Layout //////
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {{"POSITION",
													0,
													DXGI_FORMAT_R32G32B32_FLOAT,
													0,
													0,
													D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
													0},
												   {"NORMAL",
													0,
													DXGI_FORMAT_R32G32B32_FLOAT,
													0,
													12,
													D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
													0},
												   {"TEXCOORD",
													0,
													DXGI_FORMAT_R32G32_FLOAT,
													0,
													24,
													D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
													0}};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDesc;
	inputLayoutDesc.NumElements		   = ARRAYSIZE(inputElementDesc);

	////// Pipline State //////
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	//Specify pipeline stages:
	gpsd.pRootSignature		   = m_rootSignature; 
	gpsd.InputLayout		   = inputLayoutDesc;
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.VS.pShaderBytecode	= reinterpret_cast<void*>(vertexBlob->GetBufferPointer());
	gpsd.VS.BytecodeLength	 = vertexBlob->GetBufferSize();
	gpsd.PS.pShaderBytecode	= reinterpret_cast<void*>(pixelBlob->GetBufferPointer());
	gpsd.PS.BytecodeLength	 = pixelBlob->GetBufferSize();

	//Specify render target and depthstencil usage.
	gpsd.RTVFormats[0]	= DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask		  = UINT_MAX;

	//Specify rasterizer behaviour.
	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	//Specify blend descriptions.
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {false,
													false,
													D3D12_BLEND_ONE,
													D3D12_BLEND_ZERO,
													D3D12_BLEND_OP_ADD,
													D3D12_BLEND_ONE,
													D3D12_BLEND_ZERO,
													D3D12_BLEND_OP_ADD,
													D3D12_LOGIC_OP_NOOP,
													D3D12_COLOR_WRITE_ENABLE_ALL};
	for(UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	hr = m_device4->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_graphPipelineState));
}
