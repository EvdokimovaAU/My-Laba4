#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <string>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct PerObjectCB
{
    XMFLOAT4X4 World;
    XMFLOAT4X4 View;
    XMFLOAT4X4 Proj;
};

class D3D12Context
{
public:
    
    bool Initialize(HWND hwnd, UINT width, UINT height);
    void Shutdown();

    void Render(float r, float g, float b, float a);
    void SetRotation(float t);

private:
    bool CreateSponzaTestModel();
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain(HWND hwnd);
    bool CreateRTV();
    bool CreateDepthStencil();
    bool CreateFence();

    bool CreateRootSignature();
    bool CreatePipelineState();
    bool CreateGeometry();
    bool CreateConstantBuffer();
    bool CompileShaders();

    void UpdateCB();
    void WaitForGPU();

private:
    UINT m_width = 0;
    UINT m_height = 0;

    // DX12 core
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // Render targets
    static const UINT FrameCount = 2;
    UINT m_frameIndex = 0;

    ComPtr<ID3D12Resource> m_backBuffers[FrameCount];
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize = 0;

    // Depth buffer
    ComPtr<ID3D12Resource> m_depthBuffer;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    // Sync
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    // Pipeline
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;

    // Shaders
    ComPtr<ID3DBlob> m_vs;
    ComPtr<ID3DBlob> m_ps;

    // Geometry
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vbView{};
    D3D12_INDEX_BUFFER_VIEW m_ibView{};
    UINT m_indexCount = 0;

    // Constant buffer
    ComPtr<ID3D12Resource> m_constantBuffer;
    PerObjectCB m_cbData{};
    UINT8* m_cbMappedData = nullptr;

    // Animation
    float m_rotationT = 0.f;
};
