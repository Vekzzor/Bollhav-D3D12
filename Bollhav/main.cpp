#include <CopyList.h>
#include <Core/Compute/ComputeQueue.h>
#include <Core/Compute/GPUComputing.h>
#include <Core/Graphics/DX12/CommandList.h>
#include <Core/Graphics/DX12/ConstantBuffer.h>
#include <Core/Graphics/DX12/DepthStencil.h>
#include <Core/Graphics/DX12/Device.h>
#include <Core/Graphics/DX12/GraphicsCommandQueue.h>
#include <Core/Graphics/DX12/GraphicsPipelineState.h>
#include <Core/Graphics/DX12/Swapchain.h>
#include <Core/Graphics/DX12/VertexBuffer.h>
#include <Core/Graphics/FrameManager.h>
#include <Core/Graphics/Grid.h>
#include <Core/Window/Window.h>

#include <chrono>

#include <DX12Heap.h>
#include <Utility/ObjLoader.h>

#include <Core/Camera/FPSCamera.h>

#include <Core/Input/Input.h>

enum ParticleFormation
{
	Sphere,
	Wormhole,
	Something
};

struct DATA
{
	float x, y, z; // Position
	float vx, vy, vz; // Velocity
};

struct CBData
{
	UINT numCubes  = 128;
	UINT numBlocks = 1;

	float dt		 = 0.001f;
	float damping	= 1.0f;
	float centerMass = 10000.0f * 10000.0f;
} cbData{};

static double computeTimeInMs	= 0;
static double renderTimeInMs	 = 0;
static double totalFrameTimeInMs = 0;
ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;
ComPtr<ID3D12DescriptorHeap> g_Heap;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void ImguiSetup(ID3D12Device4* _pDevice, HWND _hWindowHandle);
void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList);
void CreateParticleFormation(ParticleFormation formation,
							 DATA* particles,
							 const UINT& numCubes,
							 const UINT& blockSize);

int main(int, char**)
{
	ID3D12Debug* debugController;

	HMODULE mD3D12 = GetModuleHandle("D3D12.dll");
	PFN_D3D12_GET_DEBUG_INTERFACE f =
		(PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(mD3D12, "D3D12GetDebugInterface");
	if(SUCCEEDED(f(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	Window window(VideoMode(1280, 720), L"Hejsan");

	Device device;
	Swapchain sc(device.GetDevice());
	GraphicsCommandQueue CommandQueue(device.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	//Copy Command Queue
	GraphicsCommandQueue copyCommandQueue(device.GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);

	sc.Init(device.GetDevice(), CommandQueue.GetCommandQueue());
	DepthStencil ds(device.GetDevice(), window.GetWidth(), window.GetHeight());

	ComPtr<ID3D12RootSignature> pRootGraphics;
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion					  = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if(FAILED(device->CheckFeatureSupport(
			   D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		D3D12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors					= 1;
		ranges[0].BaseShaderRegister				= 0;
		ranges[0].RegisterSpace						= 0;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[0].Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;

		D3D12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.ShaderRegister = 0;
		rootParameters[0].Descriptor.RegisterSpace  = 0;
		rootParameters[0].Descriptor.Flags			= D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

		rootParameters[1].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameters[1].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Constants.Num32BitValues = 16;
		rootParameters[1].Constants.ShaderRegister = 1;
		rootParameters[1].Constants.RegisterSpace  = 0;

		rootParameters[2].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[2].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[2].Descriptor.Flags			= D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		rootParameters[2].Descriptor.RegisterSpace  = 0;
		rootParameters[2].Descriptor.ShaderRegister = 0;

		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter					  = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU				  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV				  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW				  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSig;
		rootSig.Version					   = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSig.Desc_1_1.NumParameters	 = _countof(rootParameters);
		rootSig.Desc_1_1.pParameters	   = rootParameters;
		rootSig.Desc_1_1.NumStaticSamplers = 1;
		rootSig.Desc_1_1.pStaticSamplers   = &samplerDesc;
		rootSig.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		TIF(D3DX12SerializeVersionedRootSignature(
			&rootSig, featureData.HighestVersion, &signature, &error));
		TIF(device->CreateRootSignature(0,
										signature->GetBufferPointer(),
										signature->GetBufferSize(),
										IID_PPV_ARGS(&pRootGraphics)));

		NAME_D3D12_OBJECT(pRootGraphics);
	}

	FrameManager fm(device.GetDevice());
	CommandList cl(device.GetDevice(), fm.GetReadyFrame(&sc)->GetCommandAllocator());

	CopyList copyList = CopyList(device.GetDevice());

	ImguiSetup(device.GetDevice(), window.getHandle());

	OBJLoader obj;

	CURRENT_VALUES ret = obj.loadObj("Assets/cube.obj");

	std::vector<XMFLOAT3> v;
	for(int i = 0; i < ret.vertexIndices.size(); i++)
	{
		v.push_back(ret.out_vertices[ret.vertexIndices[i]]);
		v.push_back(ret.out_normals[ret.normalIndices[i]]);
	}

	/*Before creating a buffer of any kind, a heap has to be created since 
	that is what we will store the buffer resource in.*/
	DX12Heap dynamicHeap;

	dynamicHeap.SetProperties(
		D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1);

	//Create Vertex
	//size_t size = v.size() * sizeof(XMFLOAT3);
	VERTEX_BUFFER_DESC vbDesc;
	vbDesc.pData		 = v.data();
	vbDesc.SizeInBytes   = v.size() * sizeof(XMFLOAT3);
	vbDesc.StrideInBytes = sizeof(XMFLOAT3) * 2;

	//Create Positions
	constexpr UINT blockSize = 128;
	UINT nCubes				 = (blockSize * blockSize) * 3;

	DATA* positions = new DATA[nCubes];
	CreateParticleFormation(ParticleFormation::Wormhole, positions, nCubes, blockSize);

	//D3D12_HEAP_PROPERTIES defaultHeap = {};
	//defaultHeap.CPUPageProperty		  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//defaultHeap.CreationNodeMask	  = 1;
	//defaultHeap.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	//defaultHeap.Type				  = D3D12_HEAP_TYPE_DEFAULT;
	//defaultHeap.VisibleNodeMask		  = 1;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Alignment		   = 0;
	bufferDesc.DepthOrArraySize	= 1;
	bufferDesc.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Flags			   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	bufferDesc.Format			   = DXGI_FORMAT_UNKNOWN;
	bufferDesc.Height			   = 1;
	bufferDesc.Width			   = sizeof(DATA) * nCubes;
	bufferDesc.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels		   = 1;
	bufferDesc.SampleDesc.Count	= 1;

	VERTEX_BUFFER_DESC gridVBDesc;
	gridVBDesc.pData		 = v.data();
	gridVBDesc.SizeInBytes   = v.size() * sizeof(XMFLOAT3);
	gridVBDesc.StrideInBytes = sizeof(XMFLOAT3) * 2;

	UINT vertexSize   = (vbDesc.SizeInBytes + 65535) & ~65535;
	UINT gridSize	 = (gridVBDesc.SizeInBytes + 65535) & ~65535;
	UINT posSize	  = (bufferDesc.Width + 65535) & ~65535;
	UINT transferSize = vertexSize + gridSize + posSize;

	D3D12_HEAP_PROPERTIES heapProperties = dynamicHeap.GetProperties();

	ComPtr<ID3D12Resource> posResource;
	dynamicHeap.SetDesc(
		UINT64(transferSize), heapProperties, 0, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	dynamicHeap.CreateWithCurrentSettings(device.GetDevice());

	//Create Vertex
	VertexBuffer boxBuffer(device.GetDevice(), &vbDesc, &dynamicHeap, 0);

	//Create Grid
	Grid grid(device.GetDevice(), pRootGraphics.Get(), gridVBDesc, 10, 1, &dynamicHeap, vertexSize);

	ID3D12Resource* resource = dynamicHeap.InsertResource(device.GetDevice(),
														  vertexSize + gridSize,
														  bufferDesc,
														  D3D12_RESOURCE_STATE_COPY_DEST,
														  posResource.Get());
	posResource.Attach(resource);

	//Create Upload Heap for all the transfers
	copyList.CreateUploadHeap(device.GetDevice(), transferSize);

	D3D12_SUBRESOURCE_DATA subResources[3];

	//Vertex Transfer Data
	subResources[0].pData	  = vbDesc.pData;
	subResources[0].RowPitch   = vbDesc.SizeInBytes;
	subResources[0].SlicePitch = subResources[0].RowPitch;

	//Grid Transfer Data
	subResources[1].pData	  = grid.GetDataVector().data();
	subResources[1].RowPitch   = grid.GetVertexBuffer()->GetBufferData().RowPitch;
	subResources[1].SlicePitch = subResources[1].RowPitch;

	//Position Transfer Data
	subResources[2].pData	  = positions;
	subResources[2].RowPitch   = bufferDesc.Width;
	subResources[2].SlicePitch = subResources[2].RowPitch;

	//Schedule the Vertex transfer
	copyList.ScheduleCopy(
		boxBuffer.GetBufferResource(), copyList.GetUploadHeap().Get(), subResources[0], 0);

	//Schedule the Grid transfer.
	copyList.ScheduleCopy(grid.GetVertexBuffer()->GetBufferResource(),
						  copyList.GetUploadHeap().Get(),
						  subResources[1],
						  vertexSize);

	//Schedule the Position transfer.
	copyList.ScheduleCopy(
		posResource.Get(), copyList.GetUploadHeap().Get(), subResources[2], vertexSize + gridSize);

	//Submit and execute
	copyCommandQueue.SubmitList(copyList.GetList().Get());

	copyList.GetList().Get()->Close();

	copyCommandQueue.Execute();
	copyCommandQueue.WaitForGPU();

	GraphicsPipelineState gps;
	auto desc	 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.CullMode = D3D12_CULL_MODE_FRONT;
	gps.SetRasterizerState(desc);
	gps.SetPixelShader(L"Shaders/CubePS.hlsl");
	gps.SetVertexShader(L"Shaders/CubeVS.hlsl");
	gps.Finalize(device.GetDevice(), pRootGraphics.Get());

	FPSCamera camera;
	camera.setPosition({0, 5, 0});

	D3D12_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width	= window.getSize().x;
	vp.Height   = window.getSize().y;
	vp.MinDepth = D3D12_MIN_DEPTH;
	vp.MaxDepth = D3D12_MAX_DEPTH;

	D3D12_RECT scissor;
	scissor.left   = 0;
	scissor.right  = vp.Width;
	scissor.top	= 0;
	scissor.bottom = vp.Height;

	ComPtr<ID3D12RootSignature> pRootCompute;
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion					  = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if(FAILED(device->CheckFeatureSupport(
			   D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		D3D12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].RangeType			 = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].NumDescriptors	 = 1;
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RegisterSpace		 = 0;
		ranges[0].Flags				 = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[1].NumDescriptors					= 1;
		ranges[1].BaseShaderRegister				= 0;
		ranges[1].RegisterSpace						= 0;
		ranges[1].Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges   = &ranges[0];
		rootParameters[0].ShaderVisibility					  = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges   = &ranges[1];
		rootParameters[1].ShaderVisibility					  = D3D12_SHADER_VISIBILITY_ALL;

		rootParameters[2].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameters[2].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[2].Constants.Num32BitValues = 5;
		rootParameters[2].Constants.ShaderRegister = 0;
		rootParameters[2].Constants.RegisterSpace  = 0;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSig;
		rootSig.Version					   = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSig.Desc_1_1.NumParameters	 = _countof(rootParameters);
		rootSig.Desc_1_1.pParameters	   = rootParameters;
		rootSig.Desc_1_1.NumStaticSamplers = 0;
		rootSig.Desc_1_1.pStaticSamplers   = nullptr;
		rootSig.Desc_1_1.Flags			   = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		TIF(D3DX12SerializeVersionedRootSignature(
			&rootSig, featureData.HighestVersion, &signature, &error));
		if(error)
		{
			std::cout << (char*)error->GetBufferPointer() << std::endl;
		}
		TIF(device->CreateRootSignature(0,
										signature->GetBufferPointer(),
										signature->GetBufferSize(),
										IID_PPV_ARGS(&pRootCompute)));

		NAME_D3D12_OBJECT(pRootCompute);
	}

	ComPtr<ID3DBlob> computeBlob;
	TIF(D3DCompileFromFile(L"Shaders/ComputeShader.hlsl",
						   nullptr,
						   nullptr,
						   "CS_main",
						   "cs_5_1",
						   0,
						   0,
						   &computeBlob,
						   nullptr));

	D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd = {};
	cpsd.pRootSignature					   = pRootCompute.Get();
	cpsd.CS.pShaderBytecode				   = computeBlob->GetBufferPointer();
	cpsd.CS.BytecodeLength				   = computeBlob->GetBufferSize();
	cpsd.NodeMask						   = 0;
	ComPtr<ID3D12PipelineState> pComputePipeline;
	TIF(device->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&pComputePipeline)));

	ComPtr<ID3D12CommandAllocator> pComputeAllocator;
	ComPtr<ID3D12GraphicsCommandList> pComputeList;

	ComputeQueue computeQueue(device.GetDevice());
	TIF(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
									   IID_PPV_ARGS(&pComputeAllocator)));
	NAME_D3D12_OBJECT(pComputeAllocator);
	device->CreateCommandList(0,
							  D3D12_COMMAND_LIST_TYPE_COMPUTE,
							  pComputeAllocator.Get(),
							  nullptr,
							  IID_PPV_ARGS(&pComputeList));
	pComputeList->Close();

	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
	srvUavHeapDesc.NumDescriptors			  = 2;
	srvUavHeapDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvUavHeapDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TIF(device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&g_Heap)));
	NAME_D3D12_OBJECT(g_Heap);

#if 0
	//Name the Resources
	/*NAME_D3D12_OBJECT(boxBuffer.GetBufferResourceComPtr()); 
	NAME_D3D12_OBJECT(grid.GetVertexBuffer()->GetBufferResourceComPtr());
	NAME_D3D12_OBJECT(posbuffer);*/



	/*ID3D12Resource* vResources[2];
	vResources[0] = boxBuffer.GetBufferResource();
	vResources[1] = grid.GetVertexBuffer()->GetBufferResource();*/

	/*D3D12_RESOURCE_BARRIER vertexBarrier;
	vertexBarrier.Transition.pResource   = *vResources;
	vertexBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	vertexBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	vertexBarrier.Transition.Subresource = 0;
	vertexBarrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	vertexBarrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
	copyList.GetList().Get()->ResourceBarrier(2, &vertexBarrier);

	D3D12_RESOURCE_BARRIER posBarrier;
	posBarrier.Transition.pResource   = posbuffer.Get();
	posBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	posBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	posBarrier.Transition.Subresource = 0;
	posBarrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	posBarrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
	copyList.GetList().Get()->ResourceBarrier(1, &posBarrier);*/
#endif

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Buffer.FirstElement				= 0;
	srvDesc.Buffer.NumElements				= nCubes;
	srvDesc.Buffer.StructureByteStride		= sizeof(DATA);
	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;

	device->CreateShaderResourceView(
		posResource.Get(), &srvDesc, g_Heap->GetCPUDescriptorHandleForHeapStart());

	HRESULT hr = device->GetDeviceRemovedReason();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement				 = 0;
	uavDesc.Buffer.NumElements				 = nCubes;
	uavDesc.Buffer.StructureByteStride		 = sizeof(DATA);
	uavDesc.Buffer.CounterOffsetInBytes		 = 0;
	uavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

	auto offHeap = g_Heap->GetCPUDescriptorHandleForHeapStart();
	UINT descSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	offHeap.ptr += descSize;
	device->CreateUnorderedAccessView(posResource.Get(), nullptr, &uavDesc, offHeap);

	static float m_timer									   = 0;
	static float m_particleGenTimer							   = 0;
	const double UPDATE_TIME								   = 1.0 / 60.0;
	static float dt											   = 1 / 60;
	std::chrono::time_point<std::chrono::steady_clock> preTime = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> currentTime =
		std::chrono::steady_clock::now();

	D3D12::D3D12Timer computeTimer;
	computeTimer.init(device.GetDevice(), 1);
	D3D12::D3D12Timer renderTimer;
	renderTimer.init(device.GetDevice(), 1);

	while(Input::IsKeyPressed(VK_ESCAPE) == false && window.isOpen())
	{
		m_timer += dt;
		m_particleGenTimer += dt;
		if(m_particleGenTimer > 0.1 && !(cbData.numCubes >= nCubes))
		{
			m_particleGenTimer = 0;
			cbData.numCubes += 128;
			cbData.numBlocks = ceil(cbData.numCubes / blockSize);
		}
		while(m_timer >= UPDATE_TIME)
		{
			m_timer -= UPDATE_TIME;

			window.pollEvents();

			// Start the Dear ImGui frame
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			static bool wireframe = false;

			if(Input::IsKeyTyped('F'))
			{
				wireframe = !wireframe;
				fm.WaitForLastSubmittedFrame();
			}

			// Camera stuff
			{
				float deltaTime		 = 0.01f;
				XMVECTOR camRot		 = camera.getRotationQuat();
				XMMATRIX camRotMat   = XMMatrixRotationQuaternion(camRot);
				XMVECTOR camMovement = XMVectorSet(0, 0, 0, 0);

				static POINT prevMouse = {0, 0};
				if(prevMouse.x == 0 && prevMouse.y == 0)
					GetCursorPos(&prevMouse);

				POINT currMousePos;
				GetCursorPos(&currMousePos);

				float dx  = (currMousePos.x - prevMouse.x) * 0.6f;
				float dy  = (currMousePos.y - prevMouse.y) * 0.6f;
				prevMouse = currMousePos;
				if(Input::IsKeyPressed(VK_LBUTTON))
				{
					camera.rotate(-dx * deltaTime, -dy * deltaTime);
				}

				float speed = Input::IsKeyPressed(VK_SHIFT) ? 100 : 10;
				if(GetAsyncKeyState('W'))
					camMovement -= camRotMat.r[2] * deltaTime * speed;
				if(GetAsyncKeyState('S'))
					camMovement += camRotMat.r[2] * deltaTime * speed;
				if(GetAsyncKeyState('A'))
					camMovement -= camRotMat.r[0] * deltaTime * speed;
				if(GetAsyncKeyState('D'))
					camMovement += camRotMat.r[0] * deltaTime * speed;
				if(GetAsyncKeyState('Q'))
					camMovement -= camRotMat.r[1] * deltaTime * speed;
				if(GetAsyncKeyState('E'))
					camMovement += camRotMat.r[1] * deltaTime * speed;

#if 0
				//if(GetAsyncKeyState('C'))
				//{
				//	//Resize the position buffer and its SRV and copy the
				//	//new information to them.
				//	nCubes++;

				//	UINT offset = sizeof(positions);

				//	Resize::ResizeArray(positions, nCubes - 1, nCubes);

				//	float rand_y = rand() / RAND_MAX;
				//	float rand_x = rand() / RAND_MAX;

				//	float ii			= static_cast<float>(nCubes);
				//	positions[nCubes].x = (rand() % 10) - 5;

				//	positions[nCubes].y = (rand() % 10) - 5;
				//	positions[nCubes].z = (rand() % 10) - 5;

				//	positions[nCubes].vx = rand_y * 0.001f;
				//	positions[nCubes].vy = -rand_x * 0.001f;
				//	positions[nCubes].vz = 0.0f;

				//	//Create a new upload heap with enough space
				//	UINT size = sizeof(positions);
				//	copyList.CreateUploadHeap(device.GetDevice(), size);

				//	D3D12_HEAP_PROPERTIES heapProperties = dynamicHeap.GetProperties();

				//	//Create a heap with the new position data
				//	dynamicHeap.SetDesc(size, heapProperties, 0, D3D12_HEAP_FLAG_NONE);
				//	dynamicHeap.CreateWithCurrentSettings(device.GetDevice());

				//	bufferDesc.Alignment		= 0;
				//	bufferDesc.DepthOrArraySize = 1;
				//	bufferDesc.Dimension		= D3D12_RESOURCE_DIMENSION_BUFFER;
				//	bufferDesc.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				//	bufferDesc.Format			= DXGI_FORMAT_UNKNOWN;
				//	bufferDesc.Height			= 1;
				//	bufferDesc.Width			= size;
				//	bufferDesc.Layout			= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				//	bufferDesc.MipLevels		= 1;
				//	bufferDesc.SampleDesc.Count = 1;

				//	//Set the resource (in this case the resorce with positions)
				//	dynamicHeap.InsertResource(device.GetDevice(),
				//							   offset,
				//							   bufferDesc,
				//							   D3D12_RESOURCE_STATE_COPY_DEST,
				//							   posbuffer.Get());

				//				srvDesc.Buffer.FirstElement				= 0;
				//	srvDesc.Buffer.NumElements				= nCubes;
				//	srvDesc.Buffer.StructureByteStride		= sizeof(DATA);
				//	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
				//	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				//	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
				//	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;

				//	device->CreateShaderResourceView(
				//		posbuffer.Get(), &srvDesc, g_Heap->GetCPUDescriptorHandleForHeapStart());

				//	uavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
				//	uavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
				//	uavDesc.Buffer.FirstElement				 = 0;
				//	uavDesc.Buffer.NumElements				 = nCubes;
				//	uavDesc.Buffer.StructureByteStride		 = sizeof(DATA);
				//	uavDesc.Buffer.CounterOffsetInBytes		 = 0;
				//	uavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

				//	auto offHeap  = g_Heap->GetCPUDescriptorHandleForHeapStart();
				//	UINT descSize = device->GetDescriptorHandleIncrementSize(
				//		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				//	offHeap.ptr += descSize;
				//	device->CreateUnorderedAccessView(posbuffer.Get(), nullptr, &uavDesc, offHeap);
				//}
#endif

				camera.move(camMovement);
				camera.update(deltaTime);
			}

			// Compute
			TIF(pComputeAllocator->Reset());
			TIF(pComputeList->Reset(pComputeAllocator.Get(), pComputePipeline.Get()));

			D3D12_RESOURCE_BARRIER srvToUav = {};
			srvToUav.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			srvToUav.Transition.pResource   = posResource.Get();
			srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			srvToUav.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
			pComputeList->ResourceBarrier(1, &srvToUav);

			pComputeList->SetComputeRootSignature(pRootCompute.Get());

			pComputeList->SetComputeRoot32BitConstants(2, 5, reinterpret_cast<LPCVOID>(&cbData), 0);

			ID3D12DescriptorHeap* ppHeaps2[] = {g_Heap.Get()};
			pComputeList->SetDescriptorHeaps(_countof(ppHeaps2), ppHeaps2);

			auto SrvHandle = g_Heap->GetGPUDescriptorHandleForHeapStart();
			auto UavHandle = g_Heap->GetGPUDescriptorHandleForHeapStart();
			UavHandle.ptr += descSize;

			pComputeList->SetComputeRootDescriptorTable(0, SrvHandle);
			pComputeList->SetComputeRootDescriptorTable(1, UavHandle);

			computeTimer.start(pComputeList.Get(), 0);
			pComputeList->Dispatch(cbData.numBlocks, 1, 1);

			srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			pComputeList->ResourceBarrier(1, &srvToUav);

			computeTimer.stop(pComputeList.Get(), 0);
			computeTimer.resolveQueryToCPU(pComputeList.Get(), 0);

			TIF(pComputeList->Close());
			computeQueue.SubmitList(pComputeList.Get());
			computeQueue.Execute();
			computeQueue.WaitForGPU();

			// Rendering
			Frame* frameCtxt = fm.GetReadyFrame(&sc);
			TIF(frameCtxt->GetCommandAllocator()->Reset());

			cl.Prepare(frameCtxt->GetCommandAllocator(), sc.GetCurrentRenderTarget());

			cl->OMSetRenderTargets(
				1, &sc.GetCurrentDescriptor(), true, &ds.GetCPUDescriptorHandleForHeapStart());
			ds.Clear(cl.GetPtr());
			cl->ClearRenderTargetView(sc.GetCurrentDescriptor(), (float*)&clear_color, 0, NULL);

			cl->SetGraphicsRootSignature(pRootGraphics.Get());
			cl->SetGraphicsRoot32BitConstants(
				1, 16, reinterpret_cast<LPCVOID>(&(camera.getView() * camera.getProjection())), 0);
			cl->SetGraphicsRootShaderResourceView(2, posResource->GetGPUVirtualAddress());

			cl->RSSetViewports(1, &vp);
			cl->RSSetScissorRects(1, &scissor);

			// Draw cube
			cl->SetPipelineState(gps.GetPtr());
			cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cl->IASetVertexBuffers(0, 1, &boxBuffer.GetVertexView());

			renderTimer.start(cl.GetPtr(), 0);
			cl->DrawInstanced(boxBuffer.GetVertexCount(), cbData.numCubes, 0, 0);

			// Draw Grid
			grid.Draw(cl.GetPtr());

			ImguiDraw(cl.GetPtr());

			ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
			cl->SetDescriptorHeaps(1, ppHeaps);

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cl.GetPtr());

			renderTimer.stop(cl.GetPtr(), 0);
			renderTimer.resolveQueryToCPU(cl.GetPtr(), 0);

			cl.Finish();

			CommandQueue.SubmitList(cl.GetPtr());
			CommandQueue.Execute();

			sc.Present();

			fm.SyncCommandQueue(frameCtxt, CommandQueue.GetCommandQueue());

			//get compute time in ms
			UINT64 queueFreq;
			computeQueue.GetTimestampFrequency(&queueFreq);
			double timestampToMs = (1.0 / queueFreq) * 1000.0;

			D3D12::GPUTimestampPair computeTime = computeTimer.getTimestampPair(0);

			UINT64 dtQ		= computeTime.Stop - computeTime.Start;
			computeTimeInMs = dtQ * timestampToMs;

			//get render time in ms
			CommandQueue.GetTimestampFrequency(&queueFreq);
			timestampToMs					 = (1.0 / queueFreq) * 1000.0;
			D3D12::GPUTimestampPair drawTime = renderTimer.getTimestampPair(0);
			drawTime						 = renderTimer.getTimestampPair(0);

			dtQ	 = drawTime.Stop - drawTime.Start;
			renderTimeInMs = dtQ * timestampToMs;
		}

		currentTime = std::chrono::steady_clock::now();
		dt = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - preTime).count() /
			 1000000000.0f;
		preTime = currentTime;
	}
	fm.WaitForLastSubmittedFrame();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}

void ImguiSetup(ID3D12Device4* _pDevice, HWND _hWindowHandle)
{

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors				= 1;
	desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TIF(_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_imguiSRVHeap)));

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(_hWindowHandle);
	ImGui_ImplDX12_Init(_pDevice,
						NUM_BACKBUFFERS,
						DXGI_FORMAT_R8G8B8A8_UNORM,
						g_imguiSRVHeap->GetCPUDescriptorHandleForHeapStart(),
						g_imguiSRVHeap->GetGPUDescriptorHandleForHeapStart());
}

void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList)
{

	{

		ImGui::Begin("FPS"); // Create a window called "Hello, world!" and append into it.

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
					1000.0f / ImGui::GetIO().Framerate,
					ImGui::GetIO().Framerate);
		ImGui::Text("Compute average %.6f ms/frame", computeTimeInMs);
		ImGui::Text("Render average %.6f ms/frame", renderTimeInMs);
		ImGui::Text("C&R average %.1f ms/frame", renderTimeInMs + computeTimeInMs);
		ImGui::Text("Particle ammount: %.iK", cbData.numCubes/1000);
		ImGui::End();
	}
}

#define M_PI 3.14159265358979323846
#define DEGTORAD(d) d*(M_PI / 180)
void CreateParticleFormation(ParticleFormation formation,
							 DATA* particles,
							 const UINT& numCubes,
							 const UINT& blockSize)
{
	int cubeCount = 0;
	switch(formation)
	{
	case ParticleFormation::Sphere:
		for(int i = 0; i < ceil(numCubes / blockSize); i++)
		{
			for(int k = 0; k < blockSize; k++)
			{
				particles[cubeCount].x = 0;
				particles[cubeCount].y = 0;
				particles[cubeCount].z = -150;

				particles[cubeCount].vx = sinf(DEGTORAD(k * (360.0f / blockSize))) * 500.0f;
				particles[cubeCount].vy = cosf(DEGTORAD(k * (360.0f / blockSize))) * 500.0f;
				particles[cubeCount].vz = 0;

				cubeCount++;
			}
		}
		break;

	case ParticleFormation::Wormhole:
		for(int i = 0; i < ceil(numCubes / blockSize); i++)
		{
			for(int k = 0; k < blockSize; k++)
			{
				particles[cubeCount].x = sinf(DEGTORAD(k * (360.0f / blockSize))) * -100;
				particles[cubeCount].y = cosf(DEGTORAD(k * (360.0f / blockSize))) * -100;
				particles[cubeCount].z = -100;

				particles[cubeCount].vx = sinf(DEGTORAD(k * (360.0f / blockSize))) * 500.0f;
				particles[cubeCount].vy = cosf(DEGTORAD(k * (360.0f / blockSize))) * 500.0f;
				particles[cubeCount].vz = 0;

				cubeCount++;
			}
		}
		break;
	case ParticleFormation::Something:
		UINT nCubesSqrt = sqrt(numCubes);
		for(int i = 0; i < ceil(numCubes / blockSize); i++)
		{
			for(int k = 0; k < blockSize; k++)
			{
				float rand_x = (0.5f - (cubeCount % nCubesSqrt) / (float)nCubesSqrt) * 16.0f;
				float rand_y = (0.5f - (cubeCount / nCubesSqrt) / (float)nCubesSqrt) * 16.0f;

				//positions[cubeCount].x = rand_x + (rand() % 10) - 5;
				//positions[cubeCount].y = rand_y + (rand() % 10) - 5;
				particles[cubeCount].x = i * 2 + k - 200;
				particles[cubeCount].y = 0;
				particles[cubeCount].z = -200;

				particles[cubeCount].vx = rand_y * 50.0f;
				particles[cubeCount].vy = -rand_x * 50.0f;
				particles[cubeCount].vz = 0;

				cubeCount++;
			}
		}
		break;
	}
}