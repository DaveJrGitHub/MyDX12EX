
#include "../Std/stdafx.h"
#include "D3D12Basic.h"

// コンストラクタ
D3D12Basic::D3D12Basic(UINT width, UINT height, std::wstring name):
	DXTestBase(width, height, name)
{

}

// 初期化
void D3D12Basic::OnInit()
{
	LoadPipeline();
	LoadAssets();
}


// レンダリングパイプラインの読み込み
void D3D12Basic::LoadPipeline()
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
void D3D12Basic::LoadAssets()
{
	// コマンドリストの生成
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// コマンドリストはレコード状態で生成作成されるので、使われるまでは事前に閉じておく
	ThrowIfFailed(m_commandList->Close());

	// 同期をとる為のフェンスを生成する
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
}

// Update frame-based values.
void D3D12Basic::OnUpdate()
{
}

// 描画シーン
void D3D12Basic::OnRender()
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

void D3D12Basic::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void D3D12Basic::PopulateCommandList()
{
	// コマンドリストアロケータは、関連するコマンドリストがGPUで実行を終了した時のみリセット可能。
	// このため、フェンスを利用してGPU実行の進行状況を判断する必要がある。
	ThrowIfFailed(m_commandAllocator->Reset());

	// ただし、ExecuteCommandList()が特定のコマンドリストで呼び出されると、そのコマンドリストはいつでもリセットできます。
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// バックバッファがレンダーターゲットとして使用されるように指定してます。
	// BeforeState:D3D12_RESOURCE_STATE_PRESENT -> AfterState:D3D12_RESOURCE_STATE_RENDER_TARGET
	// 現ステートからレンダーステートへ描画
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// DiscripterHeapの先頭アドレスを基準にDescriptorのサイズ分フレーム要素分ずらした位置のアドレスを取得
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

	// Record commands.
	const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// バックバッファが表示に使用されることを示す。
	// BeforeState:D3D12_RESOURCE_STATE_RENDER_TARGET -> AfterState:D3D12_RESOURCE_STATE_PRESENT
	// レンダーステートから現ステートへ描画
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// コマンドリストへの命令の登録が終わったので、閉じる
	ThrowIfFailed(m_commandList->Close());
}

// 描画完了を待つ
void D3D12Basic::WaitForPreviousFrame()
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
