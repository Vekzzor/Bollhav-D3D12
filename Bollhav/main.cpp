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
void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList);

ComPtr<ID3D12DescriptorHeap> g_Heap;

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

	CopyList cpyListVertex  = CopyList(device.GetDevice());
	CopyList cpyListGrid	= CopyList(device.GetDevice());
	CopyList cpyListGeneral = CopyList(device.GetDevice());

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
	VertexBuffer boxBuffer(device.GetDevice(), &vbDesc, &cpyListVertex);

	GraphicsPipelineState gps;
	auto desc	 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.CullMode = D3D12_CULL_MODE_FRONT;
	gps.SetRasterizerState(desc);
	gps.SetPixelShader(L"Shaders/CubePS.hlsl");
	gps.SetVertexShader(L"Shaders/CubeVS.hlsl");
	gps.Finalize(device.GetDevice(), pRootGraphics.Get());

	// Generate grid
	Grid grid(device.GetDevice(), pRootGraphics.Get(), 10, 1, &cpyListGrid);

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

	struct DATA
	{
		float x, y, z; // Position
		float vx, vy, vz; // Velocity
	};
	DATA positions[4];
	positions[0].x  = -5;
	positions[0].y  = 0;
	positions[0].z  = 0;
	positions[0].vx = positions[0].vy = positions[0].vz = 1.1f;

	positions[1].x  = 5;
	positions[1].y  = 0;
	positions[1].z  = 0;
	positions[1].vx = positions[1].vy = positions[1].vz = 1.0f;

	positions[2].x  = 0;
	positions[2].y  = 0;
	positions[2].z  = 5;
	positions[2].vx = positions[2].vy = positions[2].vz = 1.0f;

	positions[3].x  = 0;
	positions[3].y  = 0;
	positions[3].z  = -5;
	positions[3].vx = positions[3].vy = positions[3].vz = 1.0f;

	//D3D12_HEAP_PROPERTIES uploadHeap = {};
	//uploadHeap.CPUPageProperty		 = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//uploadHeap.CreationNodeMask		 = 1;
	//uploadHeap.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
	//uploadHeap.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	//uploadHeap.VisibleNodeMask		 = 1;

	//D3D12_RESOURCE_DESC uploadBufferDesc = {};
	//uploadBufferDesc.Alignment			 = 0;
	//uploadBufferDesc.DepthOrArraySize	= 1;
	//uploadBufferDesc.Dimension			 = D3D12_RESOURCE_DIMENSION_BUFFER;
	//uploadBufferDesc.Flags				 = D3D12_RESOURCE_FLAG_NONE;
	//uploadBufferDesc.Format				 = DXGI_FORMAT_UNKNOWN;
	//uploadBufferDesc.Height				 = 1;
	//uploadBufferDesc.Width				 = sizeof(positions);
	//uploadBufferDesc.Layout				 = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//uploadBufferDesc.MipLevels			 = 1;
	//uploadBufferDesc.SampleDesc.Count	= 1;

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
	bufferDesc.Width			   = sizeof(positions);
	bufferDesc.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels		   = 1;
	bufferDesc.SampleDesc.Count	= 1;

	ComPtr<ID3D12Resource> posbuffer;
	TIF(device->CreateCommittedResource(&defaultHeap,
										D3D12_HEAP_FLAG_NONE,
										&bufferDesc,
										D3D12_RESOURCE_STATE_COPY_DEST,
										nullptr,
										IID_PPV_ARGS(&posbuffer)));
	NAME_D3D12_OBJECT(posbuffer);

	cpyListGeneral.CreateUploadHeap(device.GetDevice(), sizeof(positions));

	D3D12_SUBRESOURCE_DATA subData;
	subData.pData	  = positions;
	subData.RowPitch   = bufferDesc.Width;
	subData.SlicePitch = subData.RowPitch;

	//D3D12_RESOURCE_BARRIER barrier;
	//barrier.Transition.pResource   = posbuffer.Get();
	//barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	//barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	//barrier.Transition.Subresource = 0;
	//barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
	//cpyListGeneral.GetList().Get()->ResourceBarrier(1, &barrier);

	cpyListGeneral.ScheduleCopy(posbuffer.Get(), cpyListGeneral.GetUploadHeap().Get(), subData);

	copyCommandQueue.SubmitList(cpyListGeneral.GetPtr());
	copyCommandQueue.SubmitList(cpyListVertex.GetPtr());
	copyCommandQueue.SubmitList(cpyListGrid.GetPtr());

	copyCommandQueue.Execute();
	copyCommandQueue.WaitForGPU();
	/*
	cpyListGeneral.Finish(posbuffer.Get()); 
	cpyListVertex.Finish(boxBuffer.GetVertexData()); 
	cpyListGrid.Finish(grid.GetVertexBuffer()->GetVertexData());*/

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Buffer.FirstElement				= 0;
	srvDesc.Buffer.NumElements				= ARRAYSIZE(positions);
	srvDesc.Buffer.StructureByteStride		= sizeof(DATA);
	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;

	device->CreateShaderResourceView(
		posbuffer.Get(), &srvDesc, g_Heap->GetCPUDescriptorHandleForHeapStart());

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement				 = 0;
	uavDesc.Buffer.NumElements				 = ARRAYSIZE(positions);
	uavDesc.Buffer.StructureByteStride		 = sizeof(DATA);
	uavDesc.Buffer.CounterOffsetInBytes		 = 0;
	uavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

	auto offHeap = g_Heap->GetCPUDescriptorHandleForHeapStart();
	UINT descSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	offHeap.ptr += descSize;
	device->CreateUnorderedAccessView(posbuffer.Get(), nullptr, &uavDesc, offHeap);

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

		// Compute
		TIF(pComputeAllocator->Reset());
		TIF(pComputeList->Reset(pComputeAllocator.Get(), pComputePipeline.Get()));

		D3D12_RESOURCE_BARRIER srvToUav = {};
		srvToUav.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		srvToUav.Transition.pResource   = posbuffer.Get();
		srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		srvToUav.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
		pComputeList->ResourceBarrier(1, &srvToUav);

		pComputeList->SetComputeRootSignature(pRootCompute.Get());
		ID3D12DescriptorHeap* ppHeaps2[] = {g_Heap.Get()};
		pComputeList->SetDescriptorHeaps(_countof(ppHeaps2), ppHeaps2);

		auto SrvHandle = g_Heap->GetGPUDescriptorHandleForHeapStart();
		auto UavHandle = g_Heap->GetGPUDescriptorHandleForHeapStart();
		UavHandle.ptr += descSize;

		pComputeList->SetComputeRootDescriptorTable(0, SrvHandle);
		pComputeList->SetComputeRootDescriptorTable(1, UavHandle);

		pComputeList->Dispatch(ARRAYSIZE(positions), 1, 1);

		srvToUav.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		srvToUav.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		pComputeList->ResourceBarrier(1, &srvToUav);

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
		cl->SetGraphicsRootShaderResourceView(2, posbuffer->GetGPUVirtualAddress());

		cl->RSSetViewports(1, &vp);
		cl->RSSetScissorRects(1, &scissor);

		// Draw cube
		cl->SetPipelineState(gps.GetPtr());
		cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cl->IASetVertexBuffers(0, 1, &boxBuffer.GetVertexView());
		cl->DrawInstanced(boxBuffer.GetVertexCount(), ARRAYSIZE(positions), 0, 0);

		// Draw Grid
		grid.Draw(cl.GetPtr());

		ImguiDraw(cl.GetPtr());

		ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
		cl->SetDescriptorHeaps(1, ppHeaps);

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cl.GetPtr());

		cl.Finish();

		CommandQueue.SubmitList(cl.GetPtr());
		CommandQueue.Execute();

		sc.Present();

		fm.SyncCommandQueue(frameCtxt, CommandQueue.GetCommandQueue());
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
		ImGui::End();
	}
}
