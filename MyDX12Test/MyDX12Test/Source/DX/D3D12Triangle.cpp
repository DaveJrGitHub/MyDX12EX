
#include "D3D12Triangle.h"

// コンストラクタ
D3D12Triangle::D3D12Triangle(UINT width, UINT height, std::wstring name):
	DXTestBase(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0)
{

}

// 初期化
void D3D12Triangle::OnInit()
{
	LoadPipeline();
	LoadAssets();
}


// レンダリングパイプラインの読み込み
void D3D12Triangle::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	// DirectXグラフィックインストラクチャー作成
	// NOTE:DXGIはユーザーモードとカーネルモードのやり取りを仲介する役割を担う。
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		// Warpデバイス生成（Windows Advanced Rasterization Platform）
		// NOTE:グラフィックハードウェアがDirect3Dの機能レベルを十分にサポートしない場合でもカジュアルな用途であれば実用に耐えるデバイス。

		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}
	else
	{
		// ハードウェアデバイス生成
		// NOTE:Warpよりも高性能なデバイス

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}

	// コマンドキュー生成準備
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// GPUタイムアウトが有効
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// 直接コマンドキュー
	// コマンドキュー生成
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// スワップチェイン生成準備
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	// スワップチェイン生成
	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // 強制的にFlushする為にキューを必要とします。
		WinApp::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// Alt+Enterによるフルスクリーンを行わない
	ThrowIfFailed(factory->MakeWindowAssociation(WinApp::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// DescriptorHeap生成
	// NOTE:任意のバッファが何を示すものなのかを記述したデータがDescriptor。
	//      DescriptorはDescriptorHeapと呼ばれるヒープにコピーしないと使用できない。
	{
		// 生成するDiscripterHeapの設定。
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;	// 値の中身は『2』フロントバッファとバックバッファ用。
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// RenderTargetViewの略称。RenderTargetViewとしてヒープを使う。
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		// DiscripterHeap生成
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
		// RTV用のDescriptorのサイズを取得
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// スワップチェインのバッファをDiscripterHeapに登録する。
	{
		// ヒープの先頭アドレスを取得
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// スワップチェインで用意したバッファ分回す。
		for (UINT n = 0; n < FrameCount; n++)
		{
			// 
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			// RTV生成
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			// ヒープサイズ分アドレスをずらす。
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	// コマンドアロケータ生成
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// アセットの読み込み
void D3D12Triangle::LoadAssets()
{
	// 空のルートシグネチャ生成
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	// RootSignatureを作成するのに必要なバッファを確保、Table情報をシリアライズ
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	// 上記で確保したバッファ(signature)を指定してRootSignatureを作成
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	ComPtr<ID3DBlob> vertexShader;	// 頂点シェーダー
	ComPtr<ID3DBlob> pixelShader;	// ピクセルシェーダー

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	// ファイルからコンパイル：頂点シェーダー
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"TriangleShaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	// ファイルからコンパイル：ピクセルシェーダーシェーダー
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"TriangleShaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

	// 頂点入力レイアウト
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	// グラフィックスパイプラインステートオブジェクト（PSO）の設定を生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };	// 頂点入力レイアウト
	psoDesc.pRootSignature = m_rootSignature.Get();								// ルートシグネチャ
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());					// 頂点シェーダー
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());					// ピクセルシェーダー
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);			// ラスタライザーステート（頂点配列からなるポリゴンを 2D の平面に描画する方法の設定をつかさどる）
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);						// ブレンドステート（半透明とかの合成のやつ、ADD、ZERO、ONE等々）
	psoDesc.DepthStencilState.DepthEnable = FALSE;								// 深度設定
	psoDesc.DepthStencilState.StencilEnable = FALSE;							// ステンシル設定
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// プリミティブ描画のトポロジー指定（ラインストリップとかトライアングルリストとか）
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	// コマンドリストの生成
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// コマンドリストはレコード状態で生成作成されるので、使われるまでは事前に閉じておく
	ThrowIfFailed(m_commandList->Close());

	// 頂点バッファ作成 ----------------------------------------------------------
	// Define the geometry for a triangle.
	Vertex triangleVertices[] =
	{
		{ { 0.0f, 0.25f * m_aspectRatio, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);

	// 良く分からないけどコードを単純化する為に、非効率なことをしている模様。
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	// 頂点バッファに三角形データをコピー
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	// 頂点バッファViewを初期化
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;


	// 同期をとる為のフェンスを生成する
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValue = 1;

	// Create an event handle to use for frame synchronization.
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

// Update frame-based values.
void D3D12Triangle::OnUpdate()
{
}

// 描画シーン
void D3D12Triangle::OnRender()
{
	// シーンのレンダリングに必要な全てのコマンドをコマンドリストに記録
	PopulateCommandList();

	// コマンドリストの実行
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3D12Triangle::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void D3D12Triangle::PopulateCommandList()
{
	// コマンドリストアロケータは、関連するコマンドリストがGPUで実行を終了した時のみリセット可能。
	// このため、フェンスを利用してGPU実行の進行状況を判断する必要がある。
	ThrowIfFailed(m_commandAllocator->Reset());

	// ただし、ExecuteCommandList()が特定のコマンドリストで呼び出されると、そのコマンドリストはいつでもリセットできます。
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// 必要な情報の設定
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// バックバッファがレンダーターゲットとして使用されるように指定してます。
	// BeforeState:D3D12_RESOURCE_STATE_PRESENT -> AfterState:D3D12_RESOURCE_STATE_RENDER_TARGET
	// 現ステートからレンダーステートへ描画
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// DiscripterHeapの先頭アドレスを基準にDescriptorのサイズ分フレーム要素分ずらした位置のアドレスを取得
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// 背景色設定
	const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	// 
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// トポロジー指定
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);				// 頂点バッファ指定
	m_commandList->DrawInstanced(3, 1, 0, 0);	// インデックス指定をしないプリミティブ描画

	// バックバッファが表示に使用されることを示す。
	// BeforeState:D3D12_RESOURCE_STATE_RENDER_TARGET -> AfterState:D3D12_RESOURCE_STATE_PRESENT
	// レンダーステートから現ステートへ描画
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// コマンドリストへの命令の登録が終わったので、閉じる
	ThrowIfFailed(m_commandList->Close());
}

// 描画完了を待つ
void D3D12Triangle::WaitForPreviousFrame()
{

	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// 現在のフェンスの値がコマンド終了後にフェンスに書き込まれるようにする。
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// 
	if (m_fence->GetCompletedValue() < fence)
	{
		// このフェンスにおいて、fenceの値になったらイベントを発火させる
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		// イベントが発火するのを待つ
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
