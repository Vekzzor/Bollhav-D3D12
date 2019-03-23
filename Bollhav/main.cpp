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

#include <Utility/ObjLoader.h>

#include <Core/Camera/FPSCamera.h>

#include <Core/Input/Input.h>

ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
void ImguiSetup(ID3D12Device4* _pDevice, HWND _hWindowHandle);
void CreateRootGraphics(Device& device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& pRootGraphics);
void CreateRootCompute(Device& device);
void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList);

ComPtr<ID3D12DescriptorHeap> g_Heap;

ComPtr<ID3D12RootSignature> gRootCompute;
ComPtr<ID3D12PipelineState> gComputePipeline;

UINT gDescriptorSize;
// Buffer containing positions
constexpr UINT gTotalNumCubes = 128;
UINT gNumCubesCount			  = 1;

struct DATA
{
	float x, y, z; // Position
	float vx, vy, vz; // Velocity
};
DATA gPositions[gTotalNumCubes];

// Thread
constexpr UINT gThreadCount = 1;
enum Thread_Type : unsigned char
{
	COMPUTE = 0,
	COPY	= 1,
	THREAD_TYPE_COUNT
};
HANDLE gThreadHandles[THREAD_TYPE_COUNT][gThreadCount];
UINT gThreadIndexes[THREAD_TYPE_COUNT][gThreadCount];

void CreateThreads(ID3D12Device* _pDevice);

// Per Compute Thread
DWORD WINAPI ComputeThreadProc(LPVOID _pThreadData);

ComPtr<ID3D12CommandQueue> gTComputeQueue[gThreadCount];
ComPtr<ID3D12CommandAllocator> gTComputeAllocator[gThreadCount];
ComPtr<ID3D12GraphicsCommandList> gTComputeList[gThreadCount];

ComPtr<ID3D12Fence> gTComputeFence[gThreadCount];
volatile UINT64 gTComputeFenceValue[gThreadCount];

// Per Copy Queue

DWORD WINAPI CopyThreadProc(LPVOID _pThreadData);
ComPtr<ID3D12CommandQueue> gTCopyQueue[gThreadCount];
ComPtr<ID3D12CommandAllocator> gTCopyAllocator[gThreadCount];
ComPtr<ID3D12GraphicsCommandList> gTCopyList[gThreadCount];

D3D12_PLACED_SUBRESOURCE_FOOTPRINT gTLayouts[gThreadCount];
ComPtr<ID3D12Resource> gTCopyUpload[gThreadCount];
ComPtr<ID3D12Resource> gTPosBuffer[gThreadCount];

ComPtr<ID3D12Fence> gTCopyFence[gThreadCount];
volatile UINT64 gTCopyFenceValue[gThreadCount];

HANDLE gTEvent[THREAD_TYPE_COUNT][gThreadCount];

ComPtr<ID3D12Fence> gTRenderFence[gThreadCount];
volatile UINT64 gTRenderFenceValue[gThreadCount];

volatile LONG gThreadsRunning = 1L;

int main(int, char**)
{
	Window window(VideoMode(1280, 720), L"Hejsan");

	Device device;

	Swapchain sc(device.GetDevice());
	GraphicsCommandQueue CommandQueue(device.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	//Copy Command Queue
	GraphicsCommandQueue copyCommandQueue(device.GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);

	sc.Init(device.GetDevice(), CommandQueue.GetCommandQueue());
	DepthStencil ds(device.GetDevice(), window.GetWidth(), window.GetHeight());

	ComPtr<ID3D12RootSignature> pRootGraphics;
	CreateRootGraphics(device, pRootGraphics);
	FrameManager fm(device.GetDevice());
	Frame* currentFrame = fm.GetReadyFrame(&sc);
	CommandList cl(device.GetDevice(), currentFrame->GetDirectAllocator());

	CopyList copyList(device.GetDevice());

	ImguiSetup(device.GetDevice(), window.getHandle());

	OBJLoader obj;

	CURRENT_VALUES ret = obj.loadObj("Assets/cube.obj");

	std::vector<XMFLOAT3> v;
	for(int i = 0; i < ret.vertexIndices.size(); i++)
	{
		v.push_back(ret.out_vertices[ret.vertexIndices[i]]);
		v.push_back(ret.out_normals[ret.normalIndices[i]]);
	}

	size_t size = v.size() * sizeof(XMFLOAT3);
	VERTEX_BUFFER_DESC vbDesc;
	vbDesc.pData		 = v.data();
	vbDesc.SizeInBytes   = v.size() * sizeof(XMFLOAT3);
	vbDesc.StrideInBytes = sizeof(XMFLOAT3) * 2;
	VertexBuffer boxBuffer(device.GetDevice(), &vbDesc);

	GraphicsPipelineState gps;
	auto desc	 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.CullMode = D3D12_CULL_MODE_FRONT;
	gps.SetRasterizerState(desc);
	gps.SetPixelShader(L"Shaders/CubePS.hlsl");
	gps.SetVertexShader(L"Shaders/CubeVS.hlsl");
	gps.Finalize(device.GetDevice(), pRootGraphics.Get());

	// Generate grid
	Grid grid(device.GetDevice(), pRootGraphics.Get(), 100, 1);

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

	CreateRootCompute(device);

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
	cpsd.pRootSignature					   = gRootCompute.Get();
	cpsd.CS.pShaderBytecode				   = computeBlob->GetBufferPointer();
	cpsd.CS.BytecodeLength				   = computeBlob->GetBufferSize();
	cpsd.NodeMask						   = 0;

	TIF(device->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&gComputePipeline)));

	ComPtr<ID3D12GraphicsCommandList> pComputeList;

	ComputeQueue computeQueue(device.GetDevice());

	device->CreateCommandList(0,
							  D3D12_COMMAND_LIST_TYPE_COMPUTE,
							  currentFrame->GetComputeAllocator(),
							  nullptr,
							  IID_PPV_ARGS(&pComputeList));
	pComputeList->Close();

	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
	srvUavHeapDesc.NumDescriptors			  = 2;
	srvUavHeapDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvUavHeapDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TIF(device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&g_Heap)));
	NAME_D3D12_OBJECT(g_Heap);

	for(int i = 0; i < gTotalNumCubes; i++)
	{
		float rand_y = rand() / RAND_MAX;
		float rand_x = rand() / RAND_MAX;

		float ii		= static_cast<float>(i);
		gPositions[i].x = -ii;

		gPositions[i].y = 0;
		gPositions[i].z = 0;

		gPositions[i].vx = rand_y * 0.001f;
		gPositions[i].vy = -rand_x * 0.001f;
		gPositions[i].vz = 0.0f;
	}

	D3D12_HEAP_PROPERTIES defaultHeap = {};
	defaultHeap.CPUPageProperty		  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeap.CreationNodeMask	  = 1;
	defaultHeap.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeap.Type				  = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeap.VisibleNodeMask		  = 1;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Alignment		   = 0;
	bufferDesc.DepthOrArraySize	= 1;
	bufferDesc.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Flags			   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	bufferDesc.Format			   = DXGI_FORMAT_UNKNOWN;
	bufferDesc.Height			   = 1;
	bufferDesc.Width			   = sizeof(gPositions); // Creates a fullsize buffer
	bufferDesc.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels		   = 1;
	bufferDesc.SampleDesc.Count	= 1;

	/*TIF(device->CreateCommittedResource(&defaultHeap,
										D3D12_HEAP_FLAG_NONE,
										&bufferDesc,
										D3D12_RESOURCE_STATE_COPY_DEST,
										nullptr,
										IID_PPV_ARGS(&gPosbuffer)));
	NAME_D3D12_OBJECT(gPosbuffer);*/

	//Create Upload Heap for all the transfers

	UINT vertexSize = vbDesc.SizeInBytes;
	UINT gridSize   = grid.GetVertexSize();
	UINT posSize	= bufferDesc.Width;

	UINT transferSize = (vertexSize + gridSize + posSize);

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
	subResources[2].pData	  = gPositions;
	subResources[2].RowPitch   = gNumCubesCount * sizeof(DATA);
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

	//// Get the placed subresource footprint
	//D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts;
	//UINT nRows;
	//UINT64 rowSizeBytes;
	//UINT64 requiredSize = 0;
	//device->GetCopyableFootprints(
	//	&bufferDesc, 0, 1, vertexSize + gridSize, &layouts, &nRows, &rowSizeBytes, &requiredSize);
	//UINT dataSize = sizeof(DATA);
	//// Copy the data to the resoruce
	//{
	//	BYTE* pData;
	//	copyList.GetUploadHeap()->Map(0, nullptr, reinterpret_cast<LPVOID*>(&pData));
	//	// Offset the pointer
	//	pData += layouts.Offset;
	//	memcpy(pData, gPositions, dataSize);
	//	copyList.GetUploadHeap()->Unmap(0, nullptr);
	//}

	//copyList->CopyBufferRegion(
	//	gPosbuffer.Get(), 0, copyList.GetUploadHeap().Get(), layouts.Offset, dataSize);

	copyList.GetList().Get()->Close();

	//Submit and execute
	copyCommandQueue.SubmitList(copyList.GetList().Get());

	copyCommandQueue.Execute();
	copyCommandQueue.WaitForGPU();

	ID3D12Resource* vResources[2];
	vResources[0] = boxBuffer.GetBufferResource();
	vResources[1] = grid.GetVertexBuffer()->GetBufferResource();

	for(UINT i = 0; i < gThreadCount; i++)
	{
		gTRenderFence[i] = fm.GetFencePtr();
	}
	CreateThreads(device.GetDevice());
	currentFrame = fm.GetReadyFrame(&sc);
	while(Input::IsKeyPressed(VK_ESCAPE) == false && window.isOpen())
	{

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

			float speed = Input::IsKeyPressed(VK_SHIFT) ? 10 : 5;
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

			camera.move(camMovement);
			camera.update(deltaTime);
		}
		// Rendering
		PIXBeginEvent(CommandQueue.GetCommandQueue(), 0, L"DirectQueue");
		// Before we can render we need to instruct the compute thread to start its queues
		for(UINT threadID = 0; threadID < gThreadCount; threadID++)
		{
			// This makes the threads pass Wait point 1
			// We need to pass the actual fence value
			UINT64 val = currentFrame->GetFenceValue();
			//std::cout << "Value: " << fm.m_fenceLastSignaledValue << '\n';
			InterlockedIncrement(&gTRenderFenceValue[threadID]);
		}

		// Now lets wait for the compute to finish
		for(UINT threadID = 0; threadID < gThreadCount; threadID++)
		{
			UINT64 fenceVal = InterlockedCompareExchange(&gTComputeFenceValue[threadID], 0, 0);
			if(gTComputeFence[threadID]->GetCompletedValue() < fenceVal)
			{
				// Instruct the queue to wait for the current compute work to finish
				CommandQueue->Wait(gTComputeFence[threadID].Get(), fenceVal);
			}
		}

		TIF(currentFrame->GetDirectAllocator()->Reset());

		cl.Prepare(currentFrame->GetDirectAllocator(), sc.GetCurrentRenderTarget());

		cl->OMSetRenderTargets(
			1, &sc.GetCurrentDescriptor(), true, &ds.GetCPUDescriptorHandleForHeapStart());
		ds.Clear(cl.GetPtr());

		cl->ClearRenderTargetView(sc.GetCurrentDescriptor(), (float*)&clear_color, 0, NULL);

		cl->SetGraphicsRootSignature(pRootGraphics.Get());
		cl->SetGraphicsRoot32BitConstants(
			1, 16, reinterpret_cast<LPCVOID>(&(camera.getView() * camera.getProjection())), 0);
		cl->SetGraphicsRootShaderResourceView(2, gTPosBuffer[0]->GetGPUVirtualAddress());

		cl->RSSetViewports(1, &vp);
		cl->RSSetScissorRects(1, &scissor);

		// Draw cube
		cl->SetPipelineState(gps.GetPtr());
		cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cl->IASetVertexBuffers(0, 1, &boxBuffer.GetVertexView());
		cl->DrawInstanced(boxBuffer.GetVertexCount(), gNumCubesCount, 0, 0);

		// Draw Grid
		//grid.Draw(cl.GetPtr());

		ImguiDraw(cl.GetPtr());

		ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
		cl->SetDescriptorHeaps(1, ppHeaps);

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cl.GetPtr());

		cl.Finish();

		CommandQueue.SubmitList(cl.GetPtr());
		CommandQueue.Execute();
		PIXEndEvent(CommandQueue.GetCommandQueue());

		sc.Present();

		fm.SyncCommandQueue(currentFrame, CommandQueue.GetCommandQueue());
		currentFrame = fm.GetReadyFrame(&sc);
	}
	InterlockedExchange(&gThreadsRunning, 0L);
	WaitForMultipleObjects(gThreadCount, gThreadHandles[COMPUTE], TRUE, INFINITE);
	WaitForMultipleObjects(gThreadCount, gThreadHandles[COPY], TRUE, INFINITE);

	fm.WaitForLastSubmittedFrame();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CloseHandle(gThreadHandles[COPY][0]);
	CloseHandle(gThreadHandles[COMPUTE][0]);
	return 0;
}

void CreateRootCompute(Device& device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion					  = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if(FAILED(device->CheckFeatureSupport(
		   D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_DESCRIPTOR_RANGE1 ranges[2];
	ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[0].NumDescriptors					= 1;
	ranges[0].BaseShaderRegister				= 0;
	ranges[0].RegisterSpace						= 0;
	ranges[0].Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[1].NumDescriptors					= 1;
	ranges[1].BaseShaderRegister				= 0;
	ranges[1].RegisterSpace						= 0;
	ranges[1].Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges   = &ranges[0];
	rootParameters[0].ShaderVisibility					  = D3D12_SHADER_VISIBILITY_ALL;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges   = &ranges[1];
	rootParameters[1].ShaderVisibility					  = D3D12_SHADER_VISIBILITY_ALL;

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
	TIF(device->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&gRootCompute)));

	NAME_D3D12_OBJECT(gRootCompute);
}

void CreateRootGraphics(Device& device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& pRootGraphics)
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
		ImGui::End();
	}
}

void CreateThreads(ID3D12Device* _pDevice)
{
	D3D12_HEAP_PROPERTIES uploadHeap = {};
	uploadHeap.CPUPageProperty		 = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeap.CreationNodeMask		 = 1;
	uploadHeap.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeap.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeap.VisibleNodeMask		 = 1;

	D3D12_HEAP_PROPERTIES defaultHeap = {};
	defaultHeap.CPUPageProperty		  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeap.CreationNodeMask	  = 1;
	defaultHeap.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeap.Type				  = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeap.VisibleNodeMask		  = 1;

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Alignment			 = 0;
	uploadBufferDesc.DepthOrArraySize	= 1;
	uploadBufferDesc.Dimension			 = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Flags				 = D3D12_RESOURCE_FLAG_NONE;
	uploadBufferDesc.Format				 = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.Height				 = 1;
	uploadBufferDesc.Width				 = sizeof(gPositions); // Creates a fullsize buffer
	uploadBufferDesc.Layout				 = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.MipLevels			 = 1;
	uploadBufferDesc.SampleDesc.Count	= 1;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Alignment		   = 0;
	bufferDesc.DepthOrArraySize	= 1;
	bufferDesc.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Flags			   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	bufferDesc.Format			   = DXGI_FORMAT_UNKNOWN;
	bufferDesc.Height			   = 1;
	bufferDesc.Width			   = sizeof(gPositions); // Creates a fullsize buffer
	bufferDesc.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels		   = 1;
	bufferDesc.SampleDesc.Count	= 1;

	// Create the copy threads
	for(UINT threadIndex = 0; threadIndex < gThreadCount; threadIndex++)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type					  = D3D12_COMMAND_LIST_TYPE_COPY;
		desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask				  = 0;
		desc.Priority				  = 0;

		TIF(_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gTCopyQueue[threadIndex])));
		NAME_D3D12_OBJECT_INDEXED(gTCopyQueue, threadIndex);
		TIF(_pDevice->CreateCommandAllocator(desc.Type,
											 IID_PPV_ARGS(&gTCopyAllocator[threadIndex])));
		NAME_D3D12_OBJECT_INDEXED(gTCopyAllocator, threadIndex);
		TIF(_pDevice->CreateCommandList(desc.NodeMask,
										desc.Type,
										gTCopyAllocator[threadIndex].Get(),
										nullptr,
										IID_PPV_ARGS(&gTCopyList[threadIndex])));
		NAME_D3D12_OBJECT_INDEXED(gTCopyList, threadIndex);

		// NOTE(Henrik): Check on the shared flag
		gTComputeFenceValue[threadIndex] = 0LL;
		TIF(_pDevice->CreateFence(gTCopyFenceValue[threadIndex],
								  D3D12_FENCE_FLAG_SHARED,
								  IID_PPV_ARGS(&gTCopyFence[threadIndex])));
		NAME_D3D12_OBJECT_INDEXED(gTCopyFence, threadIndex);

		// Create upload buffer
		TIF(_pDevice->CreateCommittedResource(&uploadHeap,
											  D3D12_HEAP_FLAG_NONE,
											  &uploadBufferDesc,
											  D3D12_RESOURCE_STATE_GENERIC_READ,
											  nullptr,
											  IID_PPV_ARGS(&gTCopyUpload[threadIndex])));
		NAME_D3D12_OBJECT_INDEXED(gTCopyUpload, threadIndex);

		TIF(_pDevice->CreateCommittedResource(&defaultHeap,
											  D3D12_HEAP_FLAG_NONE,
											  &bufferDesc,
											  D3D12_RESOURCE_STATE_COPY_DEST,
											  nullptr,
											  IID_PPV_ARGS(&gTPosBuffer[threadIndex])));
		NAME_D3D12_OBJECT_INDEXED(gTPosBuffer, threadIndex);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Buffer.FirstElement				= 0;
		srvDesc.Buffer.NumElements				= gTotalNumCubes;
		srvDesc.Buffer.StructureByteStride		= sizeof(DATA);
		srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;

		_pDevice->CreateShaderResourceView(
			gTPosBuffer[threadIndex].Get(), &srvDesc, g_Heap->GetCPUDescriptorHandleForHeapStart());

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement				 = 0;
		uavDesc.Buffer.NumElements				 = gTotalNumCubes;
		uavDesc.Buffer.StructureByteStride		 = sizeof(DATA);
		uavDesc.Buffer.CounterOffsetInBytes		 = 0;
		uavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

		auto offHeap = g_Heap->GetCPUDescriptorHandleForHeapStart();
		gDescriptorSize =
			_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		offHeap.ptr += gDescriptorSize;
		_pDevice->CreateUnorderedAccessView(
			gTPosBuffer[threadIndex].Get(), nullptr, &uavDesc, offHeap);

		// Get the layouts
		UINT nRows;
		UINT64 rowSizeBytes;
		UINT64 requiredSize;
		_pDevice->GetCopyableFootprints(
			&bufferDesc, 0, 1, 0, &gTLayouts[threadIndex], &nRows, &rowSizeBytes, &requiredSize);

		gTEvent[COPY][threadIndex] = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		gThreadIndexes[COPY][threadIndex] = threadIndex;
		gThreadHandles[COPY][threadIndex] =
			CreateThread(nullptr,
						 0,
						 CopyThreadProc,
						 reinterpret_cast<LPVOID>(&gThreadIndexes[COPY][threadIndex]),
						 CREATE_SUSPENDED,
						 nullptr);

		ResumeThread(gThreadHandles[COPY][threadIndex]);
	}

	// Create the Compute threads
	for(UINT threadIndex = 0; threadIndex < gThreadCount; threadIndex++)
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type					  = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask				  = 0;
		desc.Priority				  = 0;

		_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gTComputeQueue[threadIndex]));
		_pDevice->CreateCommandAllocator(desc.Type, IID_PPV_ARGS(&gTComputeAllocator[threadIndex]));
		_pDevice->CreateCommandList(desc.NodeMask,
									desc.Type,
									gTComputeAllocator[threadIndex].Get(),
									nullptr,
									IID_PPV_ARGS(&gTComputeList[threadIndex]));

		// NOTE(Henrik): Check on the shared flag
		gTComputeFenceValue[threadIndex] = 0LL;
		_pDevice->CreateFence(gTComputeFenceValue[threadIndex],
							  D3D12_FENCE_FLAG_SHARED,
							  IID_PPV_ARGS(&gTComputeFence[threadIndex]));

		gTEvent[COMPUTE][threadIndex] = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		gThreadIndexes[COMPUTE][threadIndex] = threadIndex;
		gThreadHandles[COMPUTE][threadIndex] =
			CreateThread(nullptr,
						 0,
						 ComputeThreadProc,
						 reinterpret_cast<LPVOID>(&gThreadIndexes[COMPUTE][threadIndex]),
						 CREATE_SUSPENDED,
						 nullptr);

		ResumeThread(gThreadHandles[COMPUTE][threadIndex]);
	}
}

DWORD WINAPI ComputeThreadProc(LPVOID _pThreadData)
{
	UINT threadID = *reinterpret_cast<UINT*>(_pThreadData);

	ID3D12CommandQueue* pComputeQueue		  = gTComputeQueue[threadID].Get();
	ID3D12CommandAllocator* pComputeAllocator = gTComputeAllocator[threadID].Get();
	ID3D12GraphicsCommandList* pComputeList   = gTComputeList[threadID].Get();
	ID3D12Fence* pComputeFence				  = gTComputeFence[threadID].Get();
	unsigned int backbufferIndex			  = 0;
	while(InterlockedCompareExchange(&gThreadsRunning, 0, 0))
	{

		PIXBeginEvent(pComputeQueue, 0, L"Compute");

		// Wait for Copy to finish
		UINT64 fenceVal = InterlockedCompareExchange(&gTCopyFenceValue[threadID], 0, 0);
		if(gTCopyFence[threadID]->GetCompletedValue() < fenceVal)
		{
			// Instruct the queue to wait for the current compute work to finish
			pComputeQueue->Wait(gTCopyFence[threadID].Get(), fenceVal);
			std::cout << "Compute: " << fenceVal << '\n';
			//InterlockedExchange(&gTCopyFenceValue[threadID], 0);
		}

		// Run the compute shader
		D3D12_RESOURCE_BARRIER srvToUav = {};
		srvToUav.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		srvToUav.Transition.pResource   = gTPosBuffer[threadID].Get();
		srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		srvToUav.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		pComputeList->ResourceBarrier(1, &srvToUav);

		pComputeList->SetComputeRootSignature(gRootCompute.Get());
		ID3D12DescriptorHeap* ppHeaps2[] = {g_Heap.Get()};
		pComputeList->SetDescriptorHeaps(_countof(ppHeaps2), ppHeaps2);

		auto SrvHandle = g_Heap->GetGPUDescriptorHandleForHeapStart();
		auto UavHandle = g_Heap->GetGPUDescriptorHandleForHeapStart();
		UavHandle.ptr += gDescriptorSize;

		pComputeList->SetComputeRootDescriptorTable(0, SrvHandle);
		pComputeList->SetComputeRootDescriptorTable(1, UavHandle);

		pComputeList->Dispatch(gNumCubesCount, 1, 1);

		srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		pComputeList->ResourceBarrier(1, &srvToUav);

		TIF(pComputeList->Close());

		// Execute
		ID3D12CommandList* Lists[] = {pComputeList};
		pComputeQueue->ExecuteCommandLists(ARRAYSIZE(Lists), Lists);

		// Wait for the compute to finish.
		UINT64 threadFenceValue = InterlockedIncrement(&gTComputeFenceValue[threadID]);
		TIF(pComputeQueue->Signal(gTComputeFence[threadID].Get(), threadFenceValue));

		TIF(gTComputeFence[threadID]->SetEventOnCompletion(threadFenceValue,
														   gTEvent[COMPUTE][threadID]));
		WaitForSingleObject(gTEvent[COMPUTE][threadID], INFINITE);
		PIXEndEvent(pComputeQueue);

		// Här är compute klar

		TIF(pComputeAllocator->Reset());
		TIF(pComputeList->Reset(pComputeAllocator, gComputePipeline.Get()));
	}

	return 0;
}

DWORD WINAPI CopyThreadProc(LPVOID _pThreadData)
{
	UINT threadID = *reinterpret_cast<UINT*>(_pThreadData);

	ID3D12CommandQueue* pCopyQueue		   = gTCopyQueue[threadID].Get();
	ID3D12CommandAllocator* pCopyAllocator = gTCopyAllocator[threadID].Get();
	ID3D12GraphicsCommandList* pCopyList   = gTCopyList[threadID].Get();
	ID3D12Fence* pCopyFence				   = gTCopyFence[threadID].Get();

	while(InterlockedCompareExchange(&gThreadsRunning, 0, 0))
	{
		PIXBeginEvent(pCopyQueue, 0, L"CopyQueue");
		// Do the copying!
		UINT dataSizeInBytes = sizeof(DATA);
		UINT64 inArrayOffset = (gNumCubesCount - 1) * dataSizeInBytes;
		UINT GPUBufferOffset = gTLayouts[threadID].Offset + inArrayOffset;

		BYTE* pData;
		TIF(gTCopyUpload[threadID]->Map(0, nullptr, reinterpret_cast<LPVOID*>(&pData)));

		// Offset the pointer
		pData += GPUBufferOffset;

		BYTE* inArrayStart = reinterpret_cast<BYTE*>(&gPositions) + inArrayOffset;

		memcpy(pData, inArrayStart, dataSizeInBytes);

		gTCopyUpload[threadID]->Unmap(0, nullptr);

		pCopyList->CopyBufferRegion(gTPosBuffer[threadID].Get(),
									inArrayOffset,
									gTCopyUpload[threadID].Get(),
									GPUBufferOffset,
									dataSizeInBytes);

		TIF(pCopyList->Close());

		// Execute
		ID3D12CommandList* Lists[] = {pCopyList};
		pCopyQueue->ExecuteCommandLists(ARRAYSIZE(Lists), Lists);

		// Wait for the copy to finish
		UINT64 threadFenceValue = InterlockedIncrement(&gTCopyFenceValue[threadID]);
		TIF(pCopyQueue->Signal(gTCopyFence[threadID].Get(), threadFenceValue));
		
		TIF(gTCopyFence[threadID]->SetEventOnCompletion(threadFenceValue, gTEvent[COPY][threadID]));
		WaitForSingleObject(gTEvent[COPY][threadID], INFINITE);

		// Now is the Copy finished
		gNumCubesCount++;
		gNumCubesCount = (gNumCubesCount > (gTotalNumCubes - 1)) ? 1 : gNumCubesCount;
		// Now we need to wait for the directQueue to finish
		UINT64 renderContextFenceValue =
			InterlockedCompareExchange(&gTRenderFenceValue[threadID], 0, 0);
		if(gTRenderFence[threadID]->GetCompletedValue() < renderContextFenceValue)
		{
			TIF(pCopyQueue->Wait(gTRenderFence[threadID].Get(), renderContextFenceValue));

			//InterlockedExchange(&gTRenderFenceValue[threadID], 0);
		}
	//	std::cout << "Copy: " << renderContextFenceValue << '\n';
		PIXEndEvent(pCopyQueue);
		TIF(pCopyAllocator->Reset());
		TIF(pCopyList->Reset(pCopyAllocator, nullptr));
	}

	return 0;
}
