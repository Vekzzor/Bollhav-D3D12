#include "Renderer.h"
#include "d3dx12.h"

Renderer::Renderer() {}

Renderer::~Renderer()
{
	SafeRelease(&m_factory);
	SafeRelease(&m_device4);
	SafeRelease(&m_commandList);
	SafeRelease(&m_commandQueue);
	SafeRelease(&m_commandAllocator);
	SafeRelease(&m_fence1);
	SafeRelease(&m_renderTargetsHeap);

	for(auto rt : m_renderTargetViews)
	{
		SafeRelease(&rt);
	}

	SafeRelease(&m_rootSignature);
}

void Renderer::_init(const UINT wWidth, const UINT wHeight, HWND wndHandle)
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
	CreateObjectData();
	WaitForGpu(m_commandQueue);
	WaitForGpu(m_copyCommandQueue);
}

void Renderer::CreateDevice()
{
#ifdef _DEBUG
	//Enable the D3D12 debug layer.
	ID3D12Debug* debugController = nullptr;

#	ifdef STATIC_LINK_DEBUGSTUFF
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SafeRelease(debugController);
#	else
	HMODULE mD3D12 = GetModuleHandle("D3D12.dll");
	PFN_D3D12_GET_DEBUG_INTERFACE f =
		(PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(mD3D12, "D3D12GetDebugInterface");
	if(SUCCEEDED(f(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SafeRelease(&debugController);
#	endif
#endif

	//dxgi1_6 is only needed for initialization using an adapter.
	IDXGIAdapter1* adapter = nullptr;

	//The factory is used to create device which we connect to the graphics card (adapter)
	//Here, we want to find the adapter we want to use
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&m_factory));

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

void Renderer::CreateSwapChainAndCommandIterface(HWND whand)
{
	HRESULT hr;
	//Describe and create the command queue
	D3D12_COMMAND_QUEUE_DESC ccqd = {};
	ccqd.Type					  = D3D12_COMMAND_LIST_TYPE_COPY;
	ccqd.NodeMask				  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	ccqd.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ccqd.Priority				  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	TIF(m_device4->CreateCommandQueue(&ccqd, IID_PPV_ARGS(&m_copyCommandQueue)));

	D3D12_COMMAND_QUEUE_DESC cqd = {};
	TIF(m_device4->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_commandQueue)));

	//Create command allocator. The command allocator corresponds
	//to the underlying allocations in which GPU commands are stored.
	TIF(m_device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
										  IID_PPV_ARGS(&m_commandAllocator)));

	TIF(m_device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
										  IID_PPV_ARGS(&m_copyAllocator)));

	//Create command list.
	TIF(m_device4->CreateCommandList(0,
									 D3D12_COMMAND_LIST_TYPE_DIRECT,
									 m_commandAllocator,
									 nullptr,
									 IID_PPV_ARGS(&m_commandList)));

	CreateCopyStructure();

	//When creating a command list, it is originally open. The main loop expects
	//it to be closed and we do not wish to record anything right now,
	//so we close it.
	m_commandList->Close();

	//Create a swap chain
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.BufferCount			 = NUMOFBUFFERS;
	sd.Width				 = 0;
	sd.Height				 = 0;
	sd.Format				 = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.Flags				 = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	sd.BufferUsage			 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count		 = 1;
	sd.SampleDesc.Quality	= 0;
	sd.SwapEffect			 = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode			 = DXGI_ALPHA_MODE_UNSPECIFIED;
	sd.Scaling				 = DXGI_SCALING_STRETCH;
	sd.Stereo				 = FALSE;

	ComPtr<IDXGISwapChain1> swapChain1;

	TIF(m_factory->CreateSwapChainForHwnd(
		m_commandQueue, whand, &sd, nullptr, nullptr, &swapChain1));
	{

		//Make the swapchain act as if it were a swapChain4?
		//Presuambly because the wanted crate function is only possible
		//through a swapChain1.
		TIF(SUCCEEDED(swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain4))));
		{}
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
	//Create descriptor heap for render target views.
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors			   = NUMOFBUFFERS;
	dhd.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	HRESULT hr = m_device4->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_renderTargetsHeap));

	//Create resources for the render targets.
	m_renderTargetDescSize =
		m_device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	//One RTV for each frame.
	for(UINT n = 0; n < NUMOFBUFFERS; n++)
	{
		hr = m_swapChain4->GetBuffer(n, IID_PPV_ARGS(&m_renderTargetViews[n]));
		m_device4->CreateRenderTargetView(m_renderTargetViews[n], nullptr, cdh);
		cdh.ptr += m_renderTargetDescSize;
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

	HRESULT hr;

	TIF(hr = m_device4->CreateRootSignature(
			0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void Renderer::CreatePiplelineStateAndShaders()
{
	HRESULT hr;

	////// Shader Compiles //////
	ID3DBlob* vertexBlob;
	TIF(hr = D3DCompileFromFile(
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
			));

	ID3DBlob* pixelBlob;
	TIF(hr = D3DCompileFromFile(
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
			));

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

	TIF(hr = m_device4->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_graphPipelineState)));
}

void Renderer::CreateConstantBufferResources() {}

void Renderer::WaitForGpu(ID3D12CommandQueue* commandQueue)
{
	//WAITING FOR EACH FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	//This is code implemented as such for simplicity. The cpu could for example be used
	//for other tasks to prepare the next frame while the current one is being rendered.

	//Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	commandQueue->Signal(m_fence1, fence);
	m_fenceValue++;

	//Wait until command queue is done.
	if(m_fence1->GetCompletedValue() < fence)
	{
		m_fence1->SetEventOnCompletion(fence, m_eventHandle);
		WaitForSingleObject(m_eventHandle, INFINITE);
	}
}

//void Renderer::ExecuteCopy()
//{
//	ID3D12CommandList* list[] = {m_pCopyList};
//	m_copyCommandQueue->ExecuteCommandLists(1, list);
//}
//
//void Renderer::ExeuteRender()
//{
//	ID3D12CommandList* list[] = {m_commandList};
//	m_commandQueue->ExecuteCommandLists(1, list);
//}

void Renderer::CreateCopyStructure()
{
	HRESULT hr;
	//Create Copy List
	TIF(hr = m_device4->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_COPY, m_copyAllocator, nullptr, IID_PPV_ARGS(&m_pCopyList)));
}

void Renderer::CreateObjectData()
{
	/*Temporary since this is not the proper way to
	transfer vertex buffer data. The triangle is also temporary.*/

	HRESULT hr;

	Vertex triangleVertices[3] = {0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f,  0.5f,

								  0.5f,  -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f,  -0.5f,

								  -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f};

	//Create heap properties for the GPU vertex buffer
	D3D12_HEAP_PROPERTIES vbhp = {};
	vbhp.Type				   = D3D12_HEAP_TYPE_DEFAULT;
	vbhp.CreationNodeMask	  = 1;
	vbhp.VisibleNodeMask	   = 1;

	//Create Resource for the vertex buffer
	D3D12_RESOURCE_DESC vbrd = {};
	vbrd.Dimension			 = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbrd.Width				 = sizeof(triangleVertices);
	vbrd.Height				 = 1;
	vbrd.DepthOrArraySize	= 1;
	vbrd.MipLevels			 = 1;
	vbrd.SampleDesc.Count	= 1;
	vbrd.Layout				 = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//Temporary create the vertex buffer for this temporary triangle.
	TIF(hr = m_device4->CreateCommittedResource(&vbhp,
												D3D12_HEAP_FLAG_NONE,
												&vbrd,
												D3D12_RESOURCE_STATE_COPY_DEST,
												nullptr,
												IID_PPV_ARGS(&m_pVertexBufferResource)));

	//Create upload buffer in the same fashion but with different heap flags.

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	hp.CreationNodeMask		 = 1;
	hp.VisibleNodeMask		 = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width			   = sizeof(triangleVertices);
	rd.Height			   = 1;
	rd.DepthOrArraySize	= 1;
	rd.MipLevels		   = 1;
	rd.SampleDesc.Count	= 1;
	rd.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//Creates both a resource and an implicit heap, such that the heap is big enough
	//to contain the entire resource and the resource is mapped to the heap.
	TIF(hr = m_device4->CreateCommittedResource(&hp,
												D3D12_HEAP_FLAG_NONE,
												&rd,
												D3D12_RESOURCE_STATE_GENERIC_READ,
												nullptr,
												IID_PPV_ARGS(&m_pVertexBufferUploadHeap)));

	//This is a temp member variable, every object is to own its personal
	//vertex buffer later.
	m_pVertexBufferResource->SetName(L"vb heap");

	//Copy data to the upload heap, then schedule a copy
	//from the upload heap to the vertexBuffer on the GPU.
	D3D12_SUBRESOURCE_DATA vbData;
	vbData.pData	  = triangleVertices;
	vbData.RowPitch   = sizeof(triangleVertices);
	vbData.SlicePitch = vbData.RowPitch;

	//Schedule a copy
	UpdateSubresources<1>(
		m_pCopyList, m_pVertexBufferResource, m_pVertexBufferUploadHeap, 0, 0, 1, &vbData);

	//Create transistion barrier
	m_pCopyList->ResourceBarrier( //PROBLEM
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_pVertexBufferResource,
											  D3D12_RESOURCE_STATE_COPY_DEST,
											  D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
											  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
											  D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY
	));

	//Close the copyList, the data is now transfered and ready for use.
	m_pCopyList->Close();

	//Execute copy.
	ID3D12CommandList* copyListsToExecute[] = {m_pCopyList};
	m_copyCommandQueue->ExecuteCommandLists(ARRAYSIZE(copyListsToExecute), copyListsToExecute);

	WaitForGpu(m_copyCommandQueue);

	//Initialize the vertex buffer view
	m_vertexBufferView.BufferLocation = m_pVertexBufferResource->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes	= sizeof(triangleVertices);
}

void Renderer::Render()
{
	//Command list allocators can only be reset when the associated command lists have
	//finished execution on the GPU; fences are used to ensure this (See WaitForGpu method)
	//m_copyAllocator->Reset();
	m_commandAllocator->Reset();

	m_commandList->Reset(m_commandAllocator, m_graphPipelineState);
	//m_pCopyList->Reset(m_copyAllocator, nullptr);

	//Set root signature
	m_commandList->SetGraphicsRootSignature(m_rootSignature);

	//Set root descriptor table to index 0 in previously set root signature

	//Set necessary states.
	m_commandList->RSSetViewports(1, &m_viewPort);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_frameIndex = m_swapChain4->GetCurrentBackBufferIndex();
	//Indicate that the back buffer will be used as render target.
	m_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetViews[m_frameIndex],
											  D3D12_RESOURCE_STATE_PRESENT, //state before
											  D3D12_RESOURCE_STATE_RENDER_TARGET //state after
											  ));

	//Record commands.
	//Get the handle for the current render target used as back buffer.
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_renderTargetDescSize * m_frameIndex;

	m_commandList->OMSetRenderTargets(1, &cdh, true, nullptr);

	float clearColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
	m_commandList->ClearRenderTargetView(cdh, clearColor, 0, nullptr);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

	m_commandList->DrawInstanced(3, 1, 0, 0);

	//Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetViews[m_frameIndex],
											  D3D12_RESOURCE_STATE_RENDER_TARGET, //state before
											  D3D12_RESOURCE_STATE_PRESENT //state after
											  ));

	//Close the list to prepare it for execution.
	m_commandList->Close();
	//m_pCopyList->Close();

	//Execute the command list.
	ID3D12CommandList* listsToExecute[] = {m_commandList};
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	//Present the frame.
	DXGI_PRESENT_PARAMETERS pp = {};
	m_swapChain4->Present1(0, 0, &pp);

	WaitForGpu(m_commandQueue);
	//WaitForGpu(m_copyCommandQueue);
	//Wait for GPU to finish.
	//NOT BEST PRACTICE, only used as such for simplicity.
}
