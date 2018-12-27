
#include "D3D12Triangle.h"

// �R���X�g���N�^
D3D12Triangle::D3D12Triangle(UINT width, UINT height, std::wstring name):
	DXTestBase(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0)
{

}

// ������
void D3D12Triangle::OnInit()
{
	LoadPipeline();
	LoadAssets();
}


// �����_�����O�p�C�v���C���̓ǂݍ���
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

	// DirectX�O���t�B�b�N�C���X�g���N�`���[�쐬
	// NOTE:DXGI�̓��[�U�[���[�h�ƃJ�[�l�����[�h�̂����𒇉�������S���B
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		// Warp�f�o�C�X�����iWindows Advanced Rasterization Platform�j
		// NOTE:�O���t�B�b�N�n�[�h�E�F�A��Direct3D�̋@�\���x�����\���ɃT�|�[�g���Ȃ��ꍇ�ł��J�W���A���ȗp�r�ł���Ύ��p�ɑς���f�o�C�X�B

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
		// �n�[�h�E�F�A�f�o�C�X����
		// NOTE:Warp���������\�ȃf�o�C�X

		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}

	// �R�}���h�L���[��������
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// GPU�^�C���A�E�g���L��
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// ���ڃR�}���h�L���[
	// �R�}���h�L���[����
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// �X���b�v�`�F�C����������
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	// �X���b�v�`�F�C������
	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // �����I��Flush����ׂɃL���[��K�v�Ƃ��܂��B
		WinApp::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// Alt+Enter�ɂ��t���X�N���[�����s��Ȃ�
	ThrowIfFailed(factory->MakeWindowAssociation(WinApp::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// DescriptorHeap����
	// NOTE:�C�ӂ̃o�b�t�@�������������̂Ȃ̂����L�q�����f�[�^��Descriptor�B
	//      Descriptor��DescriptorHeap�ƌĂ΂��q�[�v�ɃR�s�[���Ȃ��Ǝg�p�ł��Ȃ��B
	{
		// ��������DiscripterHeap�̐ݒ�B
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;	// �l�̒��g�́w2�x�t�����g�o�b�t�@�ƃo�b�N�o�b�t�@�p�B
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// RenderTargetView�̗��́BRenderTargetView�Ƃ��ăq�[�v���g���B
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		// DiscripterHeap����
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
		// RTV�p��Descriptor�̃T�C�Y���擾
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �X���b�v�`�F�C���̃o�b�t�@��DiscripterHeap�ɓo�^����B
	{
		// �q�[�v�̐擪�A�h���X���擾
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// �X���b�v�`�F�C���ŗp�ӂ����o�b�t�@���񂷁B
		for (UINT n = 0; n < FrameCount; n++)
		{
			// 
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			// RTV����
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			// �q�[�v�T�C�Y���A�h���X�����炷�B
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	// �R�}���h�A���P�[�^����
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// �A�Z�b�g�̓ǂݍ���
void D3D12Triangle::LoadAssets()
{
	// ��̃��[�g�V�O�l�`������
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	// RootSignature���쐬����̂ɕK�v�ȃo�b�t�@���m�ہATable�����V���A���C�Y
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	// ��L�Ŋm�ۂ����o�b�t�@(signature)���w�肵��RootSignature���쐬
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	ComPtr<ID3DBlob> vertexShader;	// ���_�V�F�[�_�[
	ComPtr<ID3DBlob> pixelShader;	// �s�N�Z���V�F�[�_�[

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	// �t�@�C������R���p�C���F���_�V�F�[�_�[
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"TriangleShaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	// �t�@�C������R���p�C���F�s�N�Z���V�F�[�_�[�V�F�[�_�[
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"TriangleShaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

	// ���_���̓��C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�iPSO�j�̐ݒ�𐶐�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };	// ���_���̓��C�A�E�g
	psoDesc.pRootSignature = m_rootSignature.Get();								// ���[�g�V�O�l�`��
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());					// ���_�V�F�[�_�[
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());					// �s�N�Z���V�F�[�_�[
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);			// ���X�^���C�U�[�X�e�[�g�i���_�z�񂩂�Ȃ�|���S���� 2D �̕��ʂɕ`�悷����@�̐ݒ�������ǂ�j
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);						// �u�����h�X�e�[�g�i�������Ƃ��̍����̂�AADD�AZERO�AONE���X�j
	psoDesc.DepthStencilState.DepthEnable = FALSE;								// �[�x�ݒ�
	psoDesc.DepthStencilState.StencilEnable = FALSE;							// �X�e���V���ݒ�
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// �v���~�e�B�u�`��̃g�|���W�[�w��i���C���X�g���b�v�Ƃ��g���C�A���O�����X�g�Ƃ��j
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	// �R�}���h���X�g�̐���
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// �R�}���h���X�g�̓��R�[�h��ԂŐ����쐬�����̂ŁA�g����܂ł͎��O�ɕ��Ă���
	ThrowIfFailed(m_commandList->Close());

	// ���_�o�b�t�@�쐬 ----------------------------------------------------------
	// Define the geometry for a triangle.
	Vertex triangleVertices[] =
	{
		{ { 0.0f, 0.25f * m_aspectRatio, 0.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f * m_aspectRatio, 0.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);

	// �ǂ�������Ȃ����ǃR�[�h��P��������ׂɁA������Ȃ��Ƃ����Ă���͗l�B
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	// ���_�o�b�t�@�ɎO�p�`�f�[�^���R�s�[
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	// ���_�o�b�t�@View��������
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;


	// �������Ƃ�ׂ̃t�F���X�𐶐�����
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

// �`��V�[��
void D3D12Triangle::OnRender()
{
	// �V�[���̃����_�����O�ɕK�v�ȑS�ẴR�}���h���R�}���h���X�g�ɋL�^
	PopulateCommandList();

	// �R�}���h���X�g�̎��s
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
	// �R�}���h���X�g�A���P�[�^�́A�֘A����R�}���h���X�g��GPU�Ŏ��s���I���������̂݃��Z�b�g�\�B
	// ���̂��߁A�t�F���X�𗘗p����GPU���s�̐i�s�󋵂𔻒f����K�v������B
	ThrowIfFailed(m_commandAllocator->Reset());

	// �������AExecuteCommandList()������̃R�}���h���X�g�ŌĂяo�����ƁA���̃R�}���h���X�g�͂��ł����Z�b�g�ł��܂��B
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// �K�v�ȏ��̐ݒ�
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// �o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�Ƃ��Ďg�p�����悤�Ɏw�肵�Ă܂��B
	// BeforeState:D3D12_RESOURCE_STATE_PRESENT -> AfterState:D3D12_RESOURCE_STATE_RENDER_TARGET
	// ���X�e�[�g���烌���_�[�X�e�[�g�֕`��
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// DiscripterHeap�̐擪�A�h���X�����Descriptor�̃T�C�Y���t���[���v�f�����炵���ʒu�̃A�h���X���擾
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// �w�i�F�ݒ�
	const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	// 
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// �g�|���W�[�w��
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);				// ���_�o�b�t�@�w��
	m_commandList->DrawInstanced(3, 1, 0, 0);	// �C���f�b�N�X�w������Ȃ��v���~�e�B�u�`��

	// �o�b�N�o�b�t�@���\���Ɏg�p����邱�Ƃ������B
	// BeforeState:D3D12_RESOURCE_STATE_RENDER_TARGET -> AfterState:D3D12_RESOURCE_STATE_PRESENT
	// �����_�[�X�e�[�g���猻�X�e�[�g�֕`��
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// �R�}���h���X�g�ւ̖��߂̓o�^���I������̂ŁA����
	ThrowIfFailed(m_commandList->Close());
}

// �`�抮����҂�
void D3D12Triangle::WaitForPreviousFrame()
{

	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// ���݂̃t�F���X�̒l���R�}���h�I����Ƀt�F���X�ɏ������܂��悤�ɂ���B
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// 
	if (m_fence->GetCompletedValue() < fence)
	{
		// ���̃t�F���X�ɂ����āAfence�̒l�ɂȂ�����C�x���g�𔭉΂�����
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		// �C�x���g�����΂���̂�҂�
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
