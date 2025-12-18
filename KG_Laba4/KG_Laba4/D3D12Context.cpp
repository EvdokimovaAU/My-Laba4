#include "D3D12Context.h"

#include <d3dcompiler.h>
#include <vector>
#include <cstring>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX;

// ------------------------------------------------------------
// Initialize / Shutdown
// ------------------------------------------------------------

bool D3D12Context::Initialize(HWND hwnd, UINT width, UINT height)
{
    m_width = width;
    m_height = height;

    if (!CreateDevice()) {
        MessageBoxW(0, L"CreateDevice FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateCommandObjects()) {
        MessageBoxW(0, L"CreateCommandObjects FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateSwapChain(hwnd)) {
        MessageBoxW(0, L"CreateSwapChain FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateRTV()) {
        MessageBoxW(0, L"CreateRTV FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateDepthStencil()) {
        MessageBoxW(0, L"CreateDepthStencil FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateFence()) {
        MessageBoxW(0, L"CreateFence FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CompileShaders()) {
        MessageBoxW(0, L"CompileShaders FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateRootSignature()) {
        MessageBoxW(0, L"CreateRootSignature FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreatePipelineState()) {
        MessageBoxW(0, L"CreatePipelineState FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateGeometry()) {
        MessageBoxW(0, L"CreateGeometry FAILED", L"DX12", MB_OK);
        return false;
    }

    if (!CreateConstantBuffer()) {
        MessageBoxW(0, L"CreateConstantBuffer FAILED", L"DX12", MB_OK);
        return false;
    }

    return true;
}


void D3D12Context::Shutdown()
{
    WaitForGPU();

    if (m_cbMappedData)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_cbMappedData = nullptr;
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void D3D12Context::SetRotation(float t)
{
    m_rotationT = t;
}

void D3D12Context::Render(float r, float g, float b, float a)
{
    // Reset
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pso.Get());

    // Barrier: PRESENT -> RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_backBuffers[m_frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_commandList->ResourceBarrier(1, &barrier);

    // RTV handle
    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += SIZE_T(m_frameIndex) * SIZE_T(m_rtvDescriptorSize);

    // DSV handle (ВАЖНО: переменная!)
    D3D12_CPU_DESCRIPTOR_HANDLE dsv =
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    // Clear
    float clearColor[] = { r, g, b, a };
    m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    m_commandList->ClearDepthStencilView(
        dsv,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);

    // Set RT + DS
    m_commandList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);

    // Viewport / scissor
    D3D12_VIEWPORT vp{};
    vp.TopLeftX = 0.f;
    vp.TopLeftY = 0.f;
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;

    D3D12_RECT scissor{ 0, 0, (LONG)m_width, (LONG)m_height };

    m_commandList->RSSetViewports(1, &vp);
    m_commandList->RSSetScissorRects(1, &scissor);

    // Root signature + CB
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->SetGraphicsRootConstantBufferView(
        0,
        m_constantBuffer->GetGPUVirtualAddress());

    // Update CB
    UpdateCB();

    // Draw
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vbView);
    m_commandList->IASetIndexBuffer(&m_ibView);

    m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

    // Barrier: RENDER_TARGET -> PRESENT
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    m_commandList->ResourceBarrier(1, &barrier);

    // Execute
    m_commandList->Close();
    ID3D12CommandList* lists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, lists);

    m_swapChain->Present(1, 0);

    WaitForGPU();
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------

void D3D12Context::UpdateCB()
{
    XMMATRIX world =
        XMMatrixRotationY(XMConvertToRadians(30.0f)) *   // поворот вбок
        XMMatrixRotationX(XMConvertToRadians(0.0f));

    XMMATRIX view =
        XMMatrixLookAtLH(
            XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f), // камера
            XMVectorZero(),                     // цель
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    XMMATRIX proj =
        XMMatrixPerspectiveFovLH(
            XM_PIDIV4,
            (float)m_width / (float)m_height,
            0.1f,
            100.0f);

    XMStoreFloat4x4(&m_cbData.World, XMMatrixTranspose(world));
    XMStoreFloat4x4(&m_cbData.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&m_cbData.Proj, XMMatrixTranspose(proj));

    memcpy(m_cbMappedData, &m_cbData, sizeof(PerObjectCB));
}


void D3D12Context::WaitForGPU()
{
    m_fenceValue++;
    m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

// ------------------------------------------------------------
// Creation: device / commands / swapchain / RTV / DSV / fence
// ------------------------------------------------------------

bool D3D12Context::CreateDevice()
{
    return SUCCEEDED(D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)));
}

bool D3D12Context::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC q{};
    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (FAILED(m_device->CreateCommandQueue(&q, IID_PPV_ARGS(&m_commandQueue))))
        return false;

    if (FAILED(m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&m_commandAllocator))))
        return false;

    if (FAILED(m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList))))
        return false;

    m_commandList->Close();
    return true;
}

bool D3D12Context::CreateSwapChain(HWND hwnd)
{
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return false;

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = FrameCount;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swap;
    if (FAILED(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        hwnd,
        &desc,
        nullptr,
        nullptr,
        &swap)))
        return false;

    swap.As(&m_swapChain);
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12Context::CreateRTV()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap{};
    heap.NumDescriptors = FrameCount;
    heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    if (FAILED(m_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&m_rtvHeap))))
        return false;

    m_rtvDescriptorSize =
        m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < FrameCount; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]))))
            return false;

        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);
        handle.ptr += SIZE_T(m_rtvDescriptorSize);
    }
    return true;
}

bool D3D12Context::CreateDepthStencil()
{
    // DSV heap
    D3D12_DESCRIPTOR_HEAP_DESC heap{};
    heap.NumDescriptors = 1;
    heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    if (FAILED(m_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&m_dsvHeap))))
        return false;

    // Depth resource
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_width;
    desc.Height = m_height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear{};
    clear.Format = desc.Format;
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    if (FAILED(m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear,
        IID_PPV_ARGS(&m_depthBuffer))))
        return false;

    m_device->CreateDepthStencilView(
        m_depthBuffer.Get(),
        nullptr,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool D3D12Context::CreateFence()
{
    if (FAILED(m_device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&m_fence))))
        return false;

    m_fenceValue = 0;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return m_fenceEvent != nullptr;
}

// ------------------------------------------------------------
// Shaders / RootSig / PSO / Geometry / ConstantBuffer
// ------------------------------------------------------------

bool D3D12Context::CompileShaders()
{
    UINT flags = 0;
#if defined(_DEBUG)
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (!lastSlash)
        return false;
    *(lastSlash + 1) = 0;

    
    wcscat_s(exePath, L"Shaders.hlsl");

    
    HRESULT hr = D3DCompileFromFile(
        exePath,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain",
        "vs_5_0",
        flags,
        0,
        &m_vs,
        nullptr);

    if (FAILED(hr))
        return false;

    hr = D3DCompileFromFile(
        exePath,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain",
        "ps_5_0",
        flags,
        0,
        &m_ps,
        nullptr);

    return SUCCEEDED(hr);
}


bool D3D12Context::CreateRootSignature()
{
    // Root: 1 CBV(b0)
    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = 0;
    param.Descriptor.RegisterSpace = 0;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rs{};
    rs.NumParameters = 1;
    rs.pParameters = &param;
    rs.NumStaticSamplers = 0;
    rs.pStaticSamplers = nullptr;
    rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errors;

    if (FAILED(D3D12SerializeRootSignature(
        &rs,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &blob,
        &errors)))
        return false;

    return SUCCEEDED(m_device->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));
}

bool D3D12Context::CreatePipelineState()
{
    D3D12_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.InputLayout = { layout, _countof(layout) };
    pso.pRootSignature = m_rootSignature.Get();
    pso.VS = { m_vs->GetBufferPointer(), m_vs->GetBufferSize() };
    pso.PS = { m_ps->GetBufferPointer(), m_ps->GetBufferSize() };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.SampleMask = UINT_MAX;

    // Rasterizer default
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
   pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.RasterizerState.FrontCounterClockwise = FALSE;
    pso.RasterizerState.DepthClipEnable = TRUE;

    // Blend default
    pso.BlendState.AlphaToCoverageEnable = FALSE;
    pso.BlendState.IndependentBlendEnable = FALSE;
    for (int i = 0; i < 8; ++i)
    {
        pso.BlendState.RenderTarget[i].BlendEnable = FALSE;
        pso.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
        pso.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
        pso.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
        pso.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
        pso.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
        pso.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
        pso.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        pso.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
        pso.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // Depth
    pso.DepthStencilState.DepthEnable = TRUE;
    pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pso.DepthStencilState.StencilEnable = FALSE;

    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = 1;

    return SUCCEEDED(m_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_pso)));
}

bool D3D12Context::CreateGeometry()
{
    struct V { XMFLOAT3 p; XMFLOAT3 n; };

    std::vector<V> vertices =
    {
        // +Z
        {{-1,-1, 1},{0,0,1}}, {{-1, 1, 1},{0,0,1}}, {{ 1, 1, 1},{0,0,1}}, {{ 1,-1, 1},{0,0,1}},
        // -Z
        {{ 1,-1,-1},{0,0,-1}}, {{ 1, 1,-1},{0,0,-1}}, {{-1, 1,-1},{0,0,-1}}, {{-1,-1,-1},{0,0,-1}},
        // +Y
        {{-1, 1, 1},{0,1,0}}, {{-1, 1,-1},{0,1,0}}, {{ 1, 1,-1},{0,1,0}}, {{ 1, 1, 1},{0,1,0}},
        // -Y
        {{-1,-1,-1},{0,-1,0}}, {{-1,-1, 1},{0,-1,0}}, {{ 1,-1, 1},{0,-1,0}}, {{ 1,-1,-1},{0,-1,0}},
        // +X
        {{ 1,-1, 1},{1,0,0}}, {{ 1, 1, 1},{1,0,0}}, {{ 1, 1,-1},{1,0,0}}, {{ 1,-1,-1},{1,0,0}},
        // -X
        {{-1,-1,-1},{-1,0,0}}, {{-1, 1,-1},{-1,0,0}}, {{-1, 1, 1},{-1,0,0}}, {{-1,-1, 1},{-1,0,0}},
    };

    std::vector<uint16_t> indices =
    {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23
    };

    m_indexCount = (UINT)indices.size();

    UINT vbSize = (UINT)(vertices.size() * sizeof(V));
    UINT ibSize = (UINT)(indices.size() * sizeof(uint16_t));

    // Upload heap (достаточно для ДЗ)
    D3D12_HEAP_PROPERTIES upload{};
    upload.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC vbDesc{};
    vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbDesc.Width = vbSize;
    vbDesc.Height = 1;
    vbDesc.DepthOrArraySize = 1;
    vbDesc.MipLevels = 1;
    vbDesc.SampleDesc.Count = 1;
    vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(m_device->CreateCommittedResource(
        &upload, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_vertexBuffer))))
        return false;

    void* p = nullptr;
    m_vertexBuffer->Map(0, nullptr, &p);
    std::memcpy(p, vertices.data(), vbSize);
    m_vertexBuffer->Unmap(0, nullptr);

    D3D12_RESOURCE_DESC ibDesc = vbDesc;
    ibDesc.Width = ibSize;

    if (FAILED(m_device->CreateCommittedResource(
        &upload, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_indexBuffer))))
        return false;

    m_indexBuffer->Map(0, nullptr, &p);
    std::memcpy(p, indices.data(), ibSize);
    m_indexBuffer->Unmap(0, nullptr);

    m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vbView.StrideInBytes = sizeof(V);
    m_vbView.SizeInBytes = vbSize;

    m_ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_ibView.SizeInBytes = ibSize;
    m_ibView.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

bool D3D12Context::CreateConstantBuffer()
{
    // 256-byte alignment
    UINT cbSize = (sizeof(PerObjectCB) + 255) & ~255u;

    D3D12_HEAP_PROPERTIES upload{};
    upload.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = cbSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(m_device->CreateCommittedResource(
        &upload, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_constantBuffer))))
        return false;

    // Init
    XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_cbData.World, XMMatrixTranspose(I));

    D3D12_RANGE readRange{ 0,0 };
    if (FAILED(m_constantBuffer->Map(0, &readRange, (void**)&m_cbMappedData)))
        return false;

    std::memcpy(m_cbMappedData, &m_cbData, sizeof(PerObjectCB));
    return true;
}

