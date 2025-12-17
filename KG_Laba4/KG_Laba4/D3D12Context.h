#pragma once

#include <windows.h>
#include <cstdint>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class D3D12Context {
public:
    bool Initialize(HWND hWnd, uint32_t width, uint32_t height);
    void Shutdown();

    // Рисуем кадр: Clear + Present
    void Render(float r, float g, float b, float a);

private:
    static constexpr uint32_t FrameCount = 2;

    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain(HWND hWnd, uint32_t width, uint32_t height);
    bool CreateRTVHeapAndViews();

    void WaitForGPU();
    void MoveToNextFrame();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

private:
    // Core
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12Fence> m_fence;

    // Per-frame allocators
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];

    // SwapChain
    ComPtr<IDXGISwapChain4> m_swapChain;
    uint32_t m_frameIndex = 0;

    // RTV
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    uint32_t m_rtvDescriptorSize = 0;
    ComPtr<ID3D12Resource> m_backBuffers[FrameCount];

    // Sync
    uint64_t m_fenceValues[FrameCount]{};
    HANDLE   m_fenceEvent = nullptr;

    // Viewport/scissor
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    D3D12_VIEWPORT m_viewport{};
    D3D12_RECT     m_scissorRect{};
};

