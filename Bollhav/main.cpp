#include <Core/Compute/GPUComputing.h>
#include <Core/Graphics/CommandList.h>
#include <Core/Graphics/ConstantBuffer.h>
#include <Core/Graphics/Device.h>
#include <Core/Graphics/FrameManager.h>
#include <Core/Graphics/GraphicsCommandQueue.h>
#include <Core/Graphics/GraphicsPipelineState.h>
#include <Core/Graphics/Swapchain.h>
#include <Core/Graphics/VertexBuffer.h>
#include <Core/Window/Window.h>

#include <Core/Camera/FPSCamera.h>

#include <Core/Input/Input.h>

ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
void ImguiSetup(ID3D12Device4* _pDevice, HWND _hWindowHandle);
void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList);

ComPtr<ID3D12DescriptorHeap> g_cbvHeap;

int main(int, char**)
{
	Window window(VideoMode(1280, 720), L"Hejsan");

	Device device;
	Swapchain sc(device.GetDevice());
	GraphicsCommandQueue CommandQueue(device.GetDevice());
	sc.Init(device.GetDevice(), CommandQueue.GetCommandQueue());

	ComPtr<ID3D12RootSignature> pRootSignature;
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

		D3D12_ROOT_PARAMETER1 rootParameters[2];
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

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSig;
		rootSig.Version					   = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSig.Desc_1_1.NumParameters	 = _countof(rootParameters);
		rootSig.Desc_1_1.pParameters	   = rootParameters;
		rootSig.Desc_1_1.NumStaticSamplers = 0;
		rootSig.Desc_1_1.pStaticSamplers   = nullptr;
		rootSig.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		TIF(D3DX12SerializeVersionedRootSignature(
			&rootSig, featureData.HighestVersion, &signature, &error));
		TIF(device->CreateRootSignature(0,
										signature->GetBufferPointer(),
										signature->GetBufferSize(),
										IID_PPV_ARGS(&pRootSignature)));

		NAME_D3D12_OBJECT(pRootSignature);
	}

	GraphicsPipelineState pso;
	pso.SetVertexShader(L"Shaders/simpleVertex.hlsl");
	pso.SetPixelShader(L"Shaders/simplePixel.hlsl");
	pso.SetWireFrame(false);
	pso.Finalize(device.GetDevice(), pRootSignature.Get());

	FrameManager fm(device.GetDevice());
	CommandList cl(device.GetDevice(), fm.GetReadyFrame(&sc)->GetCommandAllocator());

	ImguiSetup(device.GetDevice(), window.getHandle());

	ComPtr<ID3D12DescriptorHeap> pDSVHeap;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type		   = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags		   = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	TIF(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&pDSVHeap)));
	NAME_D3D12_OBJECT(pDSVHeap);

	ComPtr<ID3D12Resource> pDepthResource;

	D3D12_HEAP_PROPERTIES hp;
	hp.Type					= D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask		= 1;
	hp.VisibleNodeMask		= 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension		   = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rd.Alignment		   = 0;
	rd.Width			   = window.GetWidth();
	rd.Height			   = window.GetHeight();
	rd.DepthOrArraySize	= 1;
	rd.MipLevels		   = 0;
	rd.SampleDesc.Count	= 1;
	rd.SampleDesc.Quality  = 0;
	rd.Flags			   = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	rd.Layout			   = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rd.Format			   = DXGI_FORMAT_D32_FLOAT;

	D3D12_CLEAR_VALUE depthOp	= {};
	depthOp.Format				 = DXGI_FORMAT_D32_FLOAT;
	depthOp.DepthStencil.Depth   = 1;
	depthOp.DepthStencil.Stencil = 0;

	TIF(device->CreateCommittedResource(&hp,
										D3D12_HEAP_FLAG_NONE,
										&rd,
										D3D12_RESOURCE_STATE_DEPTH_WRITE,
										&depthOp,
										IID_PPV_ARGS(&pDepthResource)));

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format						   = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension				   = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags						   = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(
		pDepthResource.Get(), &depthStencilDesc, pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	//GPUComputing compute;
	//compute.init(device.GetDevice());

	float vertices[] = {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
						0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
						-0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

						-0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
						0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
						-0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

						-0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
						-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
						-0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

						0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
						0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
						0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

						-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
						0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
						-0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

						-0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
						0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
						-0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};
	VertexBuffer vb(device.GetDevice(), vertices, sizeof(vertices));

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors			   = 1;
	cbvHeapDesc.Flags					   = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	TIF(device.GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&g_cbvHeap)));

	struct DATA
	{
		DirectX::XMFLOAT4 pos;
	};

	FPSCamera camera;

	DATA lol;
	lol.pos = {0, 0, 0, 0};

	ConstantBuffer buffer(device.GetDevice(), g_cbvHeap.Get(), sizeof(DATA));
	buffer.SetData(&lol);

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

	float x = 0;
	float y = 0;
	float z = 0;
	while(Input::IsKeyPressed(VK_ESCAPE) == false && window.isOpen())
	{

		window.pollEvents();
		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

			static bool lol = false;

		if(Input::IsKeyTyped('F'))
		{
			lol != lol;
			pso.SetWireFrame(lol);
			pso.Finalize(device.GetDevice(), pRootSignature.Get());
		}

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

		camera.rotate(-dx * deltaTime, -dy * deltaTime);

		float speed = 10;
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

		buffer.SetData(&lol);
		// Rendering
		Frame* frameCtxt = fm.GetReadyFrame(&sc);
		TIF(frameCtxt->GetCommandAllocator()->Reset());

	

		

		cl.Prepare(frameCtxt->GetCommandAllocator(), sc.GetCurrentRenderTarget());
		cl->OMSetRenderTargets(
			1, &sc.GetCurrentDescriptor(), true, &pDSVHeap->GetCPUDescriptorHandleForHeapStart());
		cl->ClearDepthStencilView(pDSVHeap->GetCPUDescriptorHandleForHeapStart(),
								  D3D12_CLEAR_FLAG_DEPTH,
								  1.0f,
								  0,
								  0,
								  nullptr);
		cl->ClearRenderTargetView(sc.GetCurrentDescriptor(), (float*)&clear_color, 0, NULL);

		cl->SetGraphicsRootSignature(pRootSignature.Get());
		cl->SetGraphicsRootConstantBufferView(0, buffer.GetVirtualAddress());
		cl->SetGraphicsRoot32BitConstants(
			1, 16, reinterpret_cast<LPCVOID>(&(camera.getView() * camera.getProjection())), 0);
		cl->SetPipelineState(pso.GetPtr());

		cl->RSSetViewports(1, &vp);
		cl->RSSetScissorRects(1, &scissor);
		cl->IASetVertexBuffers(0, 1, &vb.GetVertexView());
		cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cl->DrawInstanced(36, 1, 0, 0);

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

	//ImGui_ImplDX12_Shutdown();
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

	static bool show_demo_window	= true;
	static bool show_another_window = false;

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	/*if(show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);*/

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f	 = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

		ImGui::Text(
			"This is some useful text."); // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window",
						&show_demo_window); // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat(
			"float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color",
						  (float*)&clear_color); // Edit 3 floats representing a color

		if(ImGui::Button(
			   "Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
					1000.0f / ImGui::GetIO().Framerate,
					ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if(show_another_window)
	{
		ImGui::Begin(
			"Another Window",
			&show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if(ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}
