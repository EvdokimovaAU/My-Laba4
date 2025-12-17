#include "D3D12Context.h"

#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

bool D3D12Context::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = (float)width;
    m_viewport.Height = (float)height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = (LONG)width;
    m_scissorRect.bottom = (LONG)height;

#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    if (!CreateDevice())
        return false;

    if (!CreateCommandObjects())
        return false;

    if (!CreateSwapChain(hWnd, width, height))
        return false;

    if (!CreateRTVHeapAndViews())
        return false;

    // Fence + event
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
        return false;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
        return false;

    for (uint32_t i = 0; i < FrameCount; ++i)
        m_fenceValues[i] = 0;

    return true;
}

void D3D12Context::Shutdown()
{
    if (m_device)
    {
        WaitForGPU();
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

bool D3D12Context::CreateDevice()
{
    // Можно попробовать аппаратный девайс сразу
    if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
        return true;

    // Если не получилось — можно добавить WARP (но для ДЗ обычно не нужно).
    // Оставим false, чтобы ты видела проблему поддержки.
    return false;
}

bool D3D12Context::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))))
        return false;

    for (uint32_t i = 0; i < FrameCount; ++i)
    {
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]))))
            return false;
    }

    if (FAILED(m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[0].Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList))))
        return false;

    // close initially
    if (FAILED(m_commandList->Close()))
        return false;

    return true;
}

bool D3D12Context::CreateSwapChain(HWND hWnd, uint32_t width, uint32_t height)
{
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return false;

    // (не обязательно, но полезно) запретим Alt+Enter fullscreen
    factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FrameCount;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        hWnd,
        &desc,
        nullptr,
        nullptr,
        &swapChain1)))
        return false;

    if (FAILED(swapChain1.As(&m_swapChain)))
        return false;

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12Context::CreateRTVHeapAndViews()
{
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = FrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap))))
        return false;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (uint32_t i = 0; i < FrameCount; ++i)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]))))
            return false;

        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);

        // следующий дескриптор (вместо CD3DX12_CPU_DESCRIPTOR_HANDLE::Offset)
        handle.ptr += (SIZE_T)m_rtvDescriptorSize;
    }

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Context::GetCurrentRTV() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (SIZE_T)m_frameIndex * (SIZE_T)m_rtvDescriptorSize;
    return handle;
}

void D3D12Context::Render(float r, float g, float b, float a)
{
    // Reset allocator + list
    m_commandAllocators[m_frameIndex]->Reset();
    m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);

    // viewport/scissor
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Transition: Present -> RenderTarget
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_backBuffers[m_frameIndex].Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_commandList->ResourceBarrier(1, &barrier);
    }

    // Clear
    float clearColor[4] = { r, g, b, a };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetCurrentRTV();
    m_commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    // Transition: RenderTarget -> Present
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_backBuffers[m_frameIndex].Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_commandList->ResourceBarrier(1, &barrier);
    }

    m_commandList->Close();

    ID3D12CommandList* lists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, lists);

    // Present
    m_swapChain->Present(1, 0);

    MoveToNextFrame();
}

void D3D12Context::WaitForGPU()
{
    // Signal
    const uint64_t fenceToWaitFor = ++m_fenceValues[m_frameIndex];
    m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor);

    // Wait
    if (m_fence->GetCompletedValue() < fenceToWaitFor)
    {
        m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void D3D12Context::MoveToNextFrame()
{
    // Signal current frame
    const uint64_t currentFenceValue = ++m_fenceValues[m_frameIndex];
    m_commandQueue->Signal(m_fence.Get(), currentFenceValue);

    // Next backbuffer index
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Wait if next frame not ready
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}
