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
#include <crtdbg.h>
#include <strsafe.h>

#define MULTI

ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;
ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
void ImguiSetup(ID3D12Device4* _pDevice, HWND _hWindowHandle);
void CreateRootGraphics(Device& device, Microsoft::WRL::ComPtr<ID3D12RootSignature>& pRootGraphics);
void CreateRootCompute(Device& device);
void updateCamera(FPSCamera& camera, float dt);
void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList);

CRITICAL_SECTION mutex;

void SafePrint(const char* _pFormat, ...)
{
	//EnterCriticalSection(&mutex);

	int nLen = 0;
	int nRet = 0;
	char cBuffer[512];
	va_list arglist;
	HANDLE hOut = NULL;
	HRESULT hRet;

	ZeroMemory(cBuffer, sizeof(cBuffer));

	va_start(arglist, _pFormat);

	nLen = lstrlenA(_pFormat);
	hRet = StringCchVPrintfA(cBuffer, 512, _pFormat, arglist);

	if(nRet >= nLen || GetLastError() == 0)
	{
		hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if(hOut != INVALID_HANDLE_VALUE)
		{
			WriteConsoleA(hOut, cBuffer, lstrlenA(cBuffer), (LPDWORD)&nLen, NULL);
		}
	}
	//LeaveCriticalSection(&mutex);
}

enum ParticleFormation
{
	Sphere,
	Wormhole,
	Something
};

struct PerformanceData
{
	UINT particleCount		= 0;
	double currentTime		= 0;
	double FrameTime		= 0;
	double DispatchDuration = 0;
	double DrawDuration		= 0;
} perfData[1000]{};

UINT perfDataIndex				 = 0;
UINT particleAmount				 = 0;
float m_time					 = 0;
static double computeTimeInMs	= 0;
static double renderTimeInMs	 = 0;
static double totalFrameTimeInMs = 0;
static float vRam				 = 0;

D3D12::D3D12Timer gComputeTimer;
D3D12::D3D12Timer gRenderTimer;

constexpr UINT gThreadCount			= 1;
constexpr UINT gThreadPerBackBuffer = NUM_BACKBUFFERS;
// Buffer containing positions
constexpr UINT blockSize		  = 128;
constexpr UINT gTotalNumCubes	 = blockSize * blockSize * 3;
UINT gCopiedData[NUM_BACKBUFFERS] = {};

struct DATA
{
	float x, y, z; // Position
	float vx, vy, vz; // Velocity
};
DATA gPositions[gTotalNumCubes];

void CreateParticleFormation(ParticleFormation formation,
							 DATA particles[gTotalNumCubes],
							 const UINT& numCubes,
							 const UINT& blockSize);
float VRamUsage();
void WriteToFile();
ComPtr<ID3D12DescriptorHeap> g_Heap;

ComPtr<ID3D12RootSignature> gRootCompute;
ComPtr<ID3D12PipelineState> gComputePipeline;

UINT gDescriptorSize;
UINT gDescriptorStride;
enum ROOT_TABLE : unsigned char
{
	ROOT_TABLE_SRV = 0,
	ROOT_TABLE_UAV,
	ROOT_TABLE_COUNT,
};

struct CBData
{
	volatile ULONG numCubes = 0;
	UINT numBlocks			= 0;

	float dt		 = 0.0001f;
	float damping	= 1.0f;
	float centerMass = 10000.0f * 10000.0f;
} cbData[gThreadPerBackBuffer]{};

// Thread
enum Thread_Type : unsigned char
{
	COMPUTE = 0,
	COPY	= 1,
	THREAD_TYPE_COUNT
};
HANDLE gThreadHandles[THREAD_TYPE_COUNT];

void CreateThreads(ID3D12Device* _pDevice);

// Per Compute Thread
DWORD WINAPI ComputeThreadProc(LPVOID _pThreadData);

ComPtr<ID3D12CommandQueue> gTComputeQueue;
ComPtr<ID3D12CommandAllocator> gTComputeAllocator[gThreadPerBackBuffer];
ComPtr<ID3D12GraphicsCommandList> gTComputeList[gThreadPerBackBuffer];

ComPtr<ID3D12Fence> gTComputeFence;
UINT64 gTComputeFenceValue;

// Per Copy Queue

DWORD WINAPI CopyThreadProc(LPVOID _pThreadData);

ComPtr<ID3D12CommandQueue> gTCopyQueue;
ComPtr<ID3D12CommandAllocator> gTCopyAllocator[gThreadPerBackBuffer];
ComPtr<ID3D12GraphicsCommandList> gTCopyList[gThreadPerBackBuffer];

D3D12_PLACED_SUBRESOURCE_FOOTPRINT gTLayouts;
ComPtr<ID3D12Resource> gTCopyUpload;
ComPtr<ID3D12Resource> gTPosBuffer[gThreadPerBackBuffer];

ComPtr<ID3D12Fence> gTCopyFence;
UINT64 gTCopyFenceValue;

HANDLE gTStartComputeEvent;
HANDLE gTWaitComputeEvent;

HANDLE gTWaitCopyEvent;
HANDLE gTStartCopyEvent;

volatile LONG gThreadsRunning = 1L;

int main(int, char**)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	InitializeCriticalSection(&mutex);

	Window window(VideoMode(1280, 720), L"Hejsan");

	Device device;
	//Init Timers to collect timestamps from GPU
	gComputeTimer.init(device.GetDevice(), 1);
	gRenderTimer.init(device.GetDevice(), 1);

	Swapchain sc(device.GetDevice());
	GraphicsCommandQueue CommandQueue(device.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	//Copy Command Queue
	GraphicsCommandQueue copyCommandQueue(device.GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);

	sc.Init(device.GetDevice(), CommandQueue.GetCommandQueue());
	DepthStencil ds(device.GetDevice(), window.GetWidth(), window.GetHeight());

	ComPtr<ID3D12RootSignature> pRootGraphics;
	CreateRootGraphics(device, pRootGraphics);

	FrameManager fm(device.GetDevice());
	Frame* currentFrame				 = fm.GetReadyFrame(&sc);
	CommandList cls[NUM_BACKBUFFERS] = {};
	for(auto& cl : cls)
	{
		cl = CommandList(device.GetDevice(), currentFrame->GetDirectAllocator());
		TIF(cl->Close());
	}

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
	NAME_D3D12_OBJECT(gComputePipeline);

	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
	srvUavHeapDesc.NumDescriptors			  = ROOT_TABLE_COUNT * gThreadPerBackBuffer;
	srvUavHeapDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvUavHeapDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TIF(device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&g_Heap)));
	NAME_D3D12_OBJECT(g_Heap);

	CreateParticleFormation(ParticleFormation::Wormhole, gPositions, gTotalNumCubes, blockSize);

	//Create Upload Heap for all the transfers

	UINT vertexSize = vbDesc.SizeInBytes;
	UINT gridSize   = grid.GetVertexSize();

	UINT transferSize = (vertexSize + gridSize);

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

	//Schedule the Vertex transfer
	copyList.ScheduleCopy(
		boxBuffer.GetBufferResource(), copyList.GetUploadHeap().Get(), subResources[0], 0);

	//Schedule the Grid transfer.
	copyList.ScheduleCopy(grid.GetVertexBuffer()->GetBufferResource(),
						  copyList.GetUploadHeap().Get(),
						  subResources[1],
						  vertexSize);

	copyList.GetList().Get()->Close();

	//Submit and execute
	copyCommandQueue.SubmitList(copyList.GetList().Get());

	copyCommandQueue.Execute();
	copyCommandQueue.WaitForGPU();

	ID3D12Resource* vResources[2];
	vResources[0] = boxBuffer.GetBufferResource();
	vResources[1] = grid.GetVertexBuffer()->GetBufferResource();

	static float m_particleGenTimer[NUM_BACKBUFFERS] = {};
	static float dt									 = 1 / 60;
	constexpr float particleInterval				 = 0.1f;

	std::chrono::time_point<std::chrono::steady_clock> preTime = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> currentTime =
		std::chrono::steady_clock::now();

	perfData[perfDataIndex].currentTime		 = 0;
	perfData[perfDataIndex].particleCount	= 128;
	perfData[perfDataIndex].DrawDuration	 = 0;
	perfData[perfDataIndex].DispatchDuration = 0;
	perfData[perfDataIndex].FrameTime		 = 0;
	perfDataIndex++;

	CreateThreads(device.GetDevice());

	while(Input::IsKeyPressed(VK_ESCAPE) == false && window.isOpen())
	{
		vRam = VRamUsage();

		const UINT frameIndex = (fm.m_fenceLastSignaledValue + 1) % NUM_BACKBUFFERS;
		m_particleGenTimer[frameIndex] += dt;
		window.pollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Camera stuff
		updateCamera(camera, dt);

		for(auto& cb : cbData)
			cb.dt = dt / 10;

		if(m_particleGenTimer[frameIndex] > particleInterval &&
		   cbData[frameIndex].numCubes < gTotalNumCubes)
		{
			m_time += m_particleGenTimer[frameIndex];
			cbData[frameIndex].numCubes += 128;
			cbData[frameIndex].numBlocks = ceil(cbData[frameIndex].numCubes / 128);
			m_particleGenTimer[frameIndex] -= particleInterval;

			particleAmount								 = cbData[0].numCubes;
			perfData[perfDataIndex].currentTime			 = m_time;
			perfData[perfDataIndex].particleCount		 = cbData[0].numCubes;
			perfData[perfDataIndex - 1].DrawDuration	 = renderTimeInMs;
			perfData[perfDataIndex - 1].DispatchDuration = computeTimeInMs;
			perfData[perfDataIndex - 1].FrameTime		 = 1000.0f / ImGui::GetIO().Framerate;
			perfDataIndex++;
		}
		//Fill command list
		{
			TIF(currentFrame->GetDirectAllocator()->Reset());

			cls[frameIndex].Prepare(currentFrame->GetDirectAllocator(),
									sc.GetCurrentRenderTarget());

			cls[frameIndex]->OMSetRenderTargets(
				1, &sc.GetCurrentDescriptor(), true, &ds.GetCPUDescriptorHandleForHeapStart());
			ds.Clear(cls[frameIndex].GetPtr());

			cls[frameIndex]->ClearRenderTargetView(
				sc.GetCurrentDescriptor(), (float*)&clear_color, 0, NULL);

			cls[frameIndex]->SetGraphicsRootSignature(pRootGraphics.Get());

			cls[frameIndex]->SetGraphicsRoot32BitConstants(
				1, 16, reinterpret_cast<LPCVOID>(&(camera.getView() * camera.getProjection())), 0);

			cls[frameIndex]->SetGraphicsRootShaderResourceView(
				2, gTPosBuffer[frameIndex]->GetGPUVirtualAddress());

			cls[frameIndex]->RSSetViewports(1, &vp);
			cls[frameIndex]->RSSetScissorRects(1, &scissor);

			// Draw cube
			cls[frameIndex]->SetPipelineState(gps.GetPtr());
			cls[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cls[frameIndex]->IASetVertexBuffers(0, 1, &boxBuffer.GetVertexView());

			gRenderTimer.start(cls[frameIndex].GetPtr(), 0);
			cls[frameIndex]->DrawInstanced(
				boxBuffer.GetVertexCount(), cbData[frameIndex].numCubes, 0, 0);

			gRenderTimer.stop(cls[frameIndex].GetPtr(), 0);
			gRenderTimer.resolveQueryToCPU(cls[frameIndex].GetPtr(), 0);

			// Draw Grid
			//grid.Draw(cl.GetPtr());

			ImguiDraw(cls[frameIndex].GetPtr());

			ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
			cls[frameIndex]->SetDescriptorHeaps(1, ppHeaps);

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cls[frameIndex].GetPtr());

			D3D12_RESOURCE_BARRIER copyToSrv = {};
			copyToSrv.Type					 = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			copyToSrv.Transition.pResource   = gTPosBuffer[frameIndex].Get();
			copyToSrv.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			copyToSrv.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
			copyToSrv.Flags					 = D3D12_RESOURCE_BARRIER_FLAG_NONE;

			cls[frameIndex]->ResourceBarrier(1, &copyToSrv);
			cls[frameIndex].Finish();
		}
#ifdef MULTI
		// Start the compute thread
		SetEvent(gTWaitComputeEvent);
#else
		ComputeThreadProc(nullptr);
#endif
		// Start Copy Thread
		if(gCopiedData[frameIndex] < gTotalNumCubes)
		{
#ifdef MULTI
			SetEvent(gTWaitCopyEvent);
			// Wait until command recording is done.
			WaitForSingleObject(gTStartCopyEvent, INFINITE);
			ResetEvent(gTStartCopyEvent);
#else

			CopyThreadProc(nullptr);
#endif

			// Execute
			ID3D12CommandList* CLists[] = {gTCopyList[frameIndex].Get()};
			gTCopyQueue->ExecuteCommandLists(ARRAYSIZE(CLists), CLists);
			UINT64 copyFenceValue = ++gTCopyFenceValue;
			gTCopyQueue->Signal(gTCopyFence.Get(), copyFenceValue);
		}

#ifdef MULTI
		// Wait for the compute thread to finish recording
		WaitForSingleObject(gTStartComputeEvent, INFINITE);

		ResetEvent(gTStartComputeEvent);
#endif
		gTComputeQueue->Wait(gTCopyFence.Get(), gTCopyFenceValue);
		// Execute
		ID3D12CommandList* Lists[] = {gTComputeList[frameIndex].Get()};
		gTComputeQueue->ExecuteCommandLists(ARRAYSIZE(Lists), Lists);

		// Wait for the compute to finish.
		{
			UINT64 threadFenceValue = ++gTComputeFenceValue;
			TIF(gTComputeQueue->Signal(gTComputeFence.Get(), threadFenceValue));
		}

		// Rendering
		PIXBeginEvent(CommandQueue.GetCommandQueue(), 0, L"DirectQueue");

		CommandQueue.SubmitList(cls[frameIndex].GetPtr());
		CommandQueue->Wait(gTComputeFence.Get(), gTComputeFenceValue);
		CommandQueue.Execute();
		PIXEndEvent(CommandQueue.GetCommandQueue());

		sc.Present();

		fm.SyncCommandQueue(currentFrame, CommandQueue.GetCommandQueue());
		currentFrame = fm.GetReadyFrame(&sc);

		//Timestamps
		{
			UINT64 queueFreq	 = 0;
			UINT64 dtQ			 = 0;
			double timestampToMs = 0;

			//get draw time in ms
			CommandQueue.GetTimestampFrequency(&queueFreq);
			timestampToMs					 = (1.0 / queueFreq) * 1000.0;
			D3D12::GPUTimestampPair drawTime = gRenderTimer.getTimestampPair(0);

			dtQ			   = drawTime.Stop - drawTime.Start;
			renderTimeInMs = dtQ * timestampToMs;

			//get compute time in ms
			queueFreq = 0;
			gTComputeQueue->GetTimestampFrequency(&queueFreq);
			timestampToMs = (1.0 / queueFreq) * 1000.0;

			D3D12::GPUTimestampPair computeTime = gComputeTimer.getTimestampPair(0);

			dtQ				= computeTime.Stop - computeTime.Start;
			computeTimeInMs = dtQ * timestampToMs;
		}

		currentTime = std::chrono::steady_clock::now();
		dt = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - preTime).count() /
			 1000000000.0f;
		preTime = currentTime;
	}

	SetEvent(gTWaitComputeEvent);
	SetEvent(gTWaitCopyEvent);

	InterlockedExchange(&gThreadsRunning, 0L);
	WaitForSingleObject(gThreadHandles[COMPUTE], INFINITE);
	WaitForSingleObject(gThreadHandles[COPY], INFINITE);

	fm.WaitForLastSubmittedFrame();
	WriteToFile();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CloseHandle(gThreadHandles[COPY]);
	CloseHandle(gThreadHandles[COMPUTE]);
	return 0;
}

void updateCamera(FPSCamera& camera, float dt)
{
	float deltaTime		 = dt;
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
		camera.rotate(-dx * deltaTime * 10, -dy * deltaTime * 10);
	}

	float speed = Input::IsKeyPressed(VK_SHIFT) ? 400 : 200;
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
	ranges[1].Flags								= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
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
		ImGui::Text("Compute average %.6f ms/frame", computeTimeInMs);
		ImGui::Text("Render average %.6f ms/frame", renderTimeInMs);
		ImGui::Text("C&R average %.1f ms/frame", renderTimeInMs + computeTimeInMs);
		ImGui::Text("Particle ammount: %.iK", particleAmount / 1000);
		ImGui::Text("VRAM Usage: %.0fMB", vRam);
		ImGui::Text("Time: %.1fs", m_time);
		ImGui::End();
	}
}

void CreateThreads(ID3D12Device* _pDevice)
{
	gDescriptorSize =
		_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gDescriptorStride = gDescriptorSize * 2;

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
	uploadBufferDesc.Width				 = sizeof(DATA) * blockSize; // Creates a fullsize buffer
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
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type					  = D3D12_COMMAND_LIST_TYPE_COPY;
		desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask				  = 0;
		desc.Priority				  = 0;

		TIF(_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gTCopyQueue)));
		NAME_D3D12_OBJECT(gTCopyQueue);

		gTCopyFenceValue = 0;

		TIF(_pDevice->CreateFence(
			gTCopyFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gTCopyFence)));

		NAME_D3D12_OBJECT(gTCopyFence);

		for(UINT frameIndex = 0; frameIndex < gThreadPerBackBuffer; frameIndex++)
		{
			TIF(_pDevice->CreateCommandAllocator(desc.Type,
												 IID_PPV_ARGS(&gTCopyAllocator[frameIndex])));
			NAME_D3D12_OBJECT_INDEXED(gTCopyAllocator, frameIndex);

			TIF(_pDevice->CreateCommandList(desc.NodeMask,
											desc.Type,
											gTCopyAllocator[frameIndex].Get(),
											nullptr,
											IID_PPV_ARGS(&gTCopyList[frameIndex])));
			NAME_D3D12_OBJECT_INDEXED(gTCopyList, frameIndex);

			gTCopyList[frameIndex]->Close();

			// Create upload buffer
			TIF(_pDevice->CreateCommittedResource(&uploadHeap,
												  D3D12_HEAP_FLAG_NONE,
												  &uploadBufferDesc,
												  D3D12_RESOURCE_STATE_GENERIC_READ,
												  nullptr,
												  IID_PPV_ARGS(&gTCopyUpload)));
			NAME_D3D12_OBJECT(gTCopyUpload);

			TIF(_pDevice->CreateCommittedResource(&defaultHeap,
												  D3D12_HEAP_FLAG_NONE,
												  &bufferDesc,
												  D3D12_RESOURCE_STATE_COMMON,
												  nullptr,
												  IID_PPV_ARGS(&gTPosBuffer[frameIndex])));
			NAME_D3D12_OBJECT_INDEXED(gTPosBuffer, frameIndex);

			D3D12_CPU_DESCRIPTOR_HANDLE heapPointer = g_Heap->GetCPUDescriptorHandleForHeapStart();
			heapPointer.ptr += frameIndex * gDescriptorStride;

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Buffer.FirstElement				= 0;
			srvDesc.Buffer.NumElements				= gTotalNumCubes;
			srvDesc.Buffer.StructureByteStride		= sizeof(DATA);
			srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;

			_pDevice->CreateShaderResourceView(
				gTPosBuffer[frameIndex].Get(), &srvDesc, heapPointer);

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement				 = 0;
			uavDesc.Buffer.NumElements				 = gTotalNumCubes;
			uavDesc.Buffer.StructureByteStride		 = sizeof(DATA);
			uavDesc.Buffer.CounterOffsetInBytes		 = 0;
			uavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

			heapPointer.ptr += gDescriptorSize;
			_pDevice->CreateUnorderedAccessView(
				gTPosBuffer[frameIndex].Get(), nullptr, &uavDesc, heapPointer);

			// Get the layouts
			UINT nRows;
			UINT64 rowSizeBytes;
			UINT64 requiredSize;
			_pDevice->GetCopyableFootprints(
				&bufferDesc, 0, 1, 0, &gTLayouts, &nRows, &rowSizeBytes, &requiredSize);
		}

		gTWaitCopyEvent  = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		gTStartCopyEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
#ifdef MULTI
		gThreadHandles[COPY] = CreateThread(nullptr, 0, CopyThreadProc, nullptr, 0, nullptr);
#endif
	}

	// Create the Compute threads
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type					  = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask				  = 0;
	desc.Priority				  = 0;

	_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gTComputeQueue));
	NAME_D3D12_OBJECT(gTComputeQueue);

	for(UINT threadIndex = 0; threadIndex < gThreadPerBackBuffer; threadIndex++)
	{
		_pDevice->CreateCommandAllocator(desc.Type, IID_PPV_ARGS(&gTComputeAllocator[threadIndex]));
		NAME_D3D12_OBJECT_INDEXED(gTComputeAllocator, threadIndex);

		_pDevice->CreateCommandList(desc.NodeMask,
									desc.Type,
									gTComputeAllocator[threadIndex].Get(),
									gComputePipeline.Get(),
									IID_PPV_ARGS(&gTComputeList[threadIndex]));

		NAME_D3D12_OBJECT_INDEXED(gTComputeList, threadIndex);

		gTComputeList[threadIndex]->Close();
		gTComputeFenceValue = 0;
		_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gTComputeFence));
		NAME_D3D12_OBJECT(gTComputeFence);
	}

	gTWaitComputeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	gTStartComputeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
#ifdef MULTI
	gThreadHandles[COMPUTE] = CreateThread(nullptr, 0, ComputeThreadProc, nullptr, 0, nullptr);
#endif
}

DWORD WINAPI ComputeThreadProc(LPVOID _pThreadData)
{
	UNREFERENCED_PARAMETER(_pThreadData);
	static unsigned int backbufferIndex = 1;
	static UINT lastIndex				 = 0;
#ifdef MULTI
	do
	{
#endif
		ID3D12CommandAllocator* pComputeAllocator = gTComputeAllocator[backbufferIndex].Get();
		ID3D12GraphicsCommandList* pComputeList   = gTComputeList[backbufferIndex].Get();

#ifdef MULTI
		// wait until render is finished
		WaitForSingleObject(gTWaitComputeEvent, INFINITE);

		// When waiting done, we set this to waiting state
		ResetEvent(gTWaitComputeEvent);
#endif

		// Fill compute list
		{
			TIF(pComputeAllocator->Reset());
			TIF(pComputeList->Reset(pComputeAllocator, gComputePipeline.Get()));

			D3D12_RESOURCE_BARRIER srvToUav = {};
			srvToUav.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			srvToUav.Transition.pResource   = gTPosBuffer[backbufferIndex].Get();
			srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			srvToUav.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;

			pComputeList->ResourceBarrier(1, &srvToUav);

			pComputeList->SetComputeRootSignature(gRootCompute.Get());
			ID3D12DescriptorHeap* ppHeaps2[] = {g_Heap.Get()};
			pComputeList->SetDescriptorHeaps(_countof(ppHeaps2), ppHeaps2);

			D3D12_GPU_DESCRIPTOR_HANDLE heapPtr = g_Heap->GetGPUDescriptorHandleForHeapStart();

			heapPtr.ptr += lastIndex * gDescriptorStride;
			pComputeList->SetComputeRootDescriptorTable(ROOT_TABLE_SRV, heapPtr);

			heapPtr = g_Heap->GetGPUDescriptorHandleForHeapStart();
			heapPtr.ptr += (backbufferIndex * gDescriptorStride) + gDescriptorSize;
			pComputeList->SetComputeRootDescriptorTable(ROOT_TABLE_UAV, heapPtr);

			pComputeList->SetComputeRoot32BitConstants(2, 5, reinterpret_cast<LPCVOID>(&cbData), 0);
			gComputeTimer.start(pComputeList, 0);
			pComputeList->Dispatch(cbData[lastIndex].numBlocks, 1, 1);

			srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			pComputeList->ResourceBarrier(1, &srvToUav);

			gComputeTimer.stop(pComputeList, 0);
			gComputeTimer.resolveQueryToCPU(pComputeList, 0);

			TIF(pComputeList->Close());
		}
#ifdef MULTI
		// Message that the recording is finished
		SetEvent(gTStartComputeEvent);
#endif
		backbufferIndex++;
		lastIndex++;
		lastIndex %= NUM_BACKBUFFERS;
		backbufferIndex %= NUM_BACKBUFFERS;
#ifdef MULTI
	} while(InterlockedCompareExchange(&gThreadsRunning, 0, 0));
#endif

	return 0;
}

DWORD WINAPI CopyThreadProc(LPVOID _pThreadData)
{
	UNREFERENCED_PARAMETER(_pThreadData);
	static unsigned int backbufferIndex = 0;
#ifdef MULTI
	do
	{
#endif
		ID3D12CommandAllocator* pCopyAllocator = gTCopyAllocator[backbufferIndex].Get();
		ID3D12GraphicsCommandList* pCopyList   = gTCopyList[backbufferIndex].Get();
#ifdef MULTI
		// wait until render is finished
		WaitForSingleObject(gTWaitCopyEvent, INFINITE);

		// When waiting done, we set this to waiting state
		ResetEvent(gTWaitCopyEvent);
#endif
		// Fill copy list
		{
			TIF(pCopyAllocator->Reset());
			TIF(pCopyList->Reset(pCopyAllocator, nullptr));
			// Do the copying!

			if(gCopiedData[backbufferIndex] < gTotalNumCubes)
			{
				UINT dataSizeInBytes = sizeof(DATA) * blockSize;
				// TODO(Henrik): fixa numcubes
				UINT64 inArrayOffset = (gCopiedData[backbufferIndex]) * sizeof(DATA);

				BYTE* pData;
				TIF(gTCopyUpload->Map(0, nullptr, reinterpret_cast<LPVOID*>(&pData)));

				BYTE* inArrayStart = reinterpret_cast<BYTE*>(&gPositions) + inArrayOffset;

				memcpy(pData, inArrayStart, dataSizeInBytes);

				gTCopyUpload->Unmap(0, nullptr);

				pCopyList->CopyBufferRegion(gTPosBuffer[backbufferIndex].Get(),
											inArrayOffset,
											gTCopyUpload.Get(),
											0,
											dataSizeInBytes);

				gCopiedData[backbufferIndex] += blockSize;
			}
			TIF(pCopyList->Close());
		}

#ifdef MULTI
		// Message that the recording is finished
		SetEvent(gTStartCopyEvent);
#endif
		backbufferIndex++;
		backbufferIndex %= NUM_BACKBUFFERS;
#ifdef MULTI
	} while(InterlockedCompareExchange(&gThreadsRunning, 0, 0));
#endif
	return 0;
}

#define M_PI 3.14159265358979323846
#define DEGTORAD(d) d*(M_PI / 180)
void CreateParticleFormation(ParticleFormation formation,
							 DATA particles[gTotalNumCubes],
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

float VRamUsage()
{
	float memoryUsage;
	IDXGIFactory* dxgifactory = nullptr;
	HRESULT ret_code =
		::CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgifactory));

	if(SUCCEEDED(ret_code))
	{
		IDXGIAdapter* dxgiAdapter = nullptr;

		if(SUCCEEDED(dxgifactory->EnumAdapters(0, &dxgiAdapter)))
		{
			IDXGIAdapter4* dxgiAdapter4 = NULL;
			if(SUCCEEDED(
				   dxgiAdapter->QueryInterface(__uuidof(IDXGIAdapter4), (void**)&dxgiAdapter4)))
			{
				DXGI_QUERY_VIDEO_MEMORY_INFO info;

				if(SUCCEEDED(dxgiAdapter4->QueryVideoMemoryInfo(
					   0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
				{
					memoryUsage = float(info.CurrentUsage / 1024.0 / 1024.0); //MiB
				};

				dxgiAdapter4->Release();
			}
			dxgiAdapter->Release();
		}
		dxgifactory->Release();
	}

	return memoryUsage;
}

void WriteToFile()
{
	std::ofstream myfile;
	myfile.open("Data.txt");
	myfile << "Index\tTime Elapsed(ms)\tParticle Count\tDispatch Duration(ms)\tDraw "
			  "Duration(ms)\tFrame Time(ms)\tVRAM Usage(MB)\t";
	myfile << vRam << "\n";
	for(int i = 0; i < perfDataIndex; i++)
	{
		myfile << i << "\t";
		myfile << perfData[i].currentTime << "\t";
		myfile << perfData[i].particleCount << "\t";
		myfile << perfData[i].DispatchDuration << "\t";
		myfile << perfData[i].DrawDuration << "\t";
		myfile << perfData[i].FrameTime << "\t";
		myfile << "\n";
	}
	myfile.close();
}