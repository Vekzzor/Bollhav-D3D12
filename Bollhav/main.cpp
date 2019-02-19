#include <Core/Compute/GPUComputing.h>
#include <Core/Graphics/CommandList.h>
#include <Core/Graphics/Device.h>
#include <Core/Graphics/FrameManager.h>
#include <Core/Graphics/GraphicsCommandQueue.h>
#include <Core/Graphics/GraphicsPipelineState.h>
#include <Core/Graphics/Swapchain.h>
#include <Core/Graphics/VertexBuffer.h>
#include <Core/Window/Window.h>

ComPtr<ID3D12DescriptorHeap> g_imguiSRVHeap;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
void ImguiSetup(ID3D12Device4* _pDevice, HWND _hWindowHandle);
void ImguiDraw(ID3D12GraphicsCommandList* _pCommandList);

int main(int, char**)
{
	Window window(VideoMode(1280, 720), L"Hejsan");

	Device device;
	Swapchain sc(device.GetDevice());
	GraphicsCommandQueue CommandQueue(device.GetDevice());
	sc.Init(device.GetDevice(), CommandQueue.GetCommandQueue());

	FrameManager fm(device.GetDevice());

	CommandList cl(device.GetDevice(), fm.GetReadyFrame(&sc)->GetCommandAllocator());

	ImguiSetup(device.GetDevice(), window.getHandle());

	//GPUComputing compute;
	//compute.init(device.GetDevice());

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

		rootParameters[1].ParameterType	= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(ranges);
		rootParameters[1].DescriptorTable.pDescriptorRanges   = ranges;

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
	pso.Finalize(device.GetDevice(), pRootSignature.Get());

	float verts[] = {
		0.0f,
		0.25f,
		0.0f,
		// First point
		0.25f,
		-0.25f,
		0.0f,
		// Second point
		- 0.25f,
		-0.25f,
		0.0f
		// Third point
	};
	VertexBuffer vb(device.GetDevice(), verts, sizeof(verts));

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

	while(window.isOpen())
	{
		window.pollEvents();

		// Rendering
		Frame* frameCtxt = fm.GetReadyFrame(&sc);
		TIF(frameCtxt->GetCommandAllocator()->Reset());

		cl.Prepare(frameCtxt->GetCommandAllocator(), sc.GetCurrentRenderTarget());

		cl->ClearRenderTargetView(sc.GetCurrentDescriptor(), (float*)&clear_color, 0, NULL);
		cl->OMSetRenderTargets(1, &sc.GetCurrentDescriptor(), FALSE, NULL);


		cl->SetGraphicsRootSignature(pRootSignature.Get());
		cl->SetPipelineState(pso.GetPtr());
		cl->RSSetViewports(1, &vp);
		cl->RSSetScissorRects(1, &scissor);
		cl->IASetVertexBuffers(0, 1, &vb.GetVertexView());
		cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cl->DrawInstanced(3, 1, 0, 0);
	
		ImguiDraw(cl.GetPtr());

		cl.Finish();

		CommandQueue.SubmitList(cl.GetPtr());
		CommandQueue.Execute();

		sc.Present();

		fm.SyncCommandQueue(frameCtxt, CommandQueue.GetCommandQueue());
	}

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

	static bool show_demo_window	= true;
	static bool show_another_window = false;

	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if(show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

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

	ID3D12DescriptorHeap* ppHeaps[] = {g_imguiSRVHeap.Get()};
	_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _pCommandList);
}
