// AdjacencyDataGenerator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include "Editor.h"




struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64 FenceValue;
};

static int const NUM_BACK_BUFFERS = 3;
static int const NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext  gFrameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT gFrameIndex = 0;
static ID3D12Device* gD3dDevice = nullptr;
static IDXGISwapChain3* gSwapChain = nullptr;
static HANDLE gSwapChainWaitableObject = nullptr;
static ID3D12DescriptorHeap* gD3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* gD3dDsvDescHeap = nullptr;
static ID3D12DescriptorHeap* gD3dSrvDescHeap = nullptr;
static ID3D12CommandQueue* gD3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* gD3dCommandList = nullptr;
static ID3D12Fence* gFence = nullptr;
static HANDLE gFenceEvent = nullptr;
static UINT64 gFenceLastSignaledValue = 0;
static ID3D12Resource* gMainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static ID3D12Resource* gMainDepthBufferResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  gMainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  gMainDepthBufferDescriptor[NUM_BACK_BUFFERS] = {};

static Editor* gEditor = nullptr;




void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &gFrameContext[gFrameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (gFence->GetCompletedValue() >= fenceValue)
        return;

    gFence->SetEventOnCompletion(fenceValue, gFenceEvent);
    WaitForSingleObject(gFenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = gFrameIndex + 1;
    gFrameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { gSwapChainWaitableObject, nullptr };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &gFrameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        gFence->SetEventOnCompletion(fenceValue, gFenceEvent);
        waitableObjects[1] = gFenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
}

void CreateRenderTarget(UINT Width, UINT Height)
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        gSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        gD3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, gMainRenderTargetDescriptor[i]);
        gMainRenderTargetResource[i] = pBackBuffer;


        D3D12_CLEAR_VALUE DepthOptimizedClearValue = {};
        DepthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        DepthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        DepthOptimizedClearValue.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES HeapProp;
        memset(&HeapProp, 0, sizeof(D3D12_HEAP_PROPERTIES));
        HeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC DepthBufferDesc = {};
        DepthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        DepthBufferDesc.Width = Width;
        DepthBufferDesc.Height = Height;
        DepthBufferDesc.DepthOrArraySize = 1;
        DepthBufferDesc.MipLevels = 1;
        DepthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
        DepthBufferDesc.SampleDesc.Count = 1;
        DepthBufferDesc.SampleDesc.Quality = 0;
        DepthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        DepthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        DepthBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        ID3D12Resource* pDepthBuffer = nullptr;
        if (FAILED(gD3dDevice->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthBufferDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &DepthOptimizedClearValue, IID_PPV_ARGS(&pDepthBuffer))))
        {
            std::cout << "Create Depth Buffer Failed." << std::endl;
        }
        gMainDepthBufferResource[i] = pDepthBuffer;

        D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilDesc = {};
        DepthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        DepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        DepthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        gD3dDevice->CreateDepthStencilView(gMainDepthBufferResource[i], &DepthStencilDesc, gMainDepthBufferDescriptor[i]);
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++) {
        if (gMainRenderTargetResource[i]) {
            gMainRenderTargetResource[i]->Release();
            gMainRenderTargetResource[i] = nullptr;
        }

        if (gMainDepthBufferResource[i]) {
            gMainDepthBufferResource[i]->Release();
            gMainDepthBufferResource[i] = nullptr;
        }
    }
}

bool CreateDeviceD3D(HWND hWnd)
{

#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug>DebugController0;
    Microsoft::WRL::ComPtr<ID3D12Debug1>DebugController1;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController0))))
    {
        std::cout << "D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController0))) FAILED" << std::endl;
        return true;
    }
    if (FAILED(DebugController0->QueryInterface(IID_PPV_ARGS(&DebugController1))))
    {
        std::cout << "DebugController0->QueryInterface(IID_PPV_ARGS(&DebugController1)) FAILED" << std::endl;
        return true;
    }
    if (DebugController1) {
        DebugController1->SetEnableGPUBasedValidation(true);
        DebugController1->EnableDebugLayer();
    }
    
#endif

    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&gD3dDevice)) != S_OK)
        return false;

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (gD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gD3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = gD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gD3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            gMainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
        Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        Desc.NumDescriptors = NUM_BACK_BUFFERS;
        Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (gD3dDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&gD3dDsvDescHeap)) != S_OK)
            return false;

        SIZE_T DsvDescriptorSize = gD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle = gD3dDsvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            gMainDepthBufferDescriptor[i] = DsvHandle;
            DsvHandle.ptr += DsvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 2; 
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (gD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gD3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (gD3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gD3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (gD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gFrameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (gD3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gFrameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&gD3dCommandList)) != S_OK ||
        gD3dCommandList->Close() != S_OK)
        return false;

    if (gD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)) != S_OK)
        return false;

    gFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (gFenceEvent == nullptr)
        return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForHwnd(gD3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&gSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        gSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        gSwapChainWaitableObject = gSwapChain->GetFrameLatencyWaitableObject();
    }
  
    RECT Rect = { 0, 0, 0, 0 };
    ::GetClientRect(hWnd, &Rect);
    CreateRenderTarget(Rect.right - Rect.left, Rect.bottom - Rect.top);

    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (gSwapChain) { gSwapChain->SetFullscreenState(false, nullptr); gSwapChain->Release(); gSwapChain = nullptr; }
    if (gSwapChainWaitableObject != nullptr) { CloseHandle(gSwapChainWaitableObject); }
    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (gFrameContext[i].CommandAllocator) { gFrameContext[i].CommandAllocator->Release(); gFrameContext[i].CommandAllocator = nullptr; }
    if (gD3dCommandQueue) { gD3dCommandQueue->Release(); gD3dCommandQueue = nullptr; }
    if (gD3dCommandList) { gD3dCommandList->Release(); gD3dCommandList = nullptr; }
    if (gD3dRtvDescHeap) { gD3dRtvDescHeap->Release(); gD3dRtvDescHeap = nullptr; }
    if (gD3dDsvDescHeap) { gD3dDsvDescHeap->Release();  gD3dDsvDescHeap = nullptr; }
    if (gD3dSrvDescHeap) { gD3dSrvDescHeap->Release(); gD3dSrvDescHeap = nullptr; }
    if (gFence) { gFence->Release(); gFence = nullptr; }
    if (gFenceEvent) { CloseHandle(gFenceEvent); gFenceEvent = nullptr; }
    if (gD3dDevice) { gD3dDevice->Release(); gD3dDevice = nullptr; }

}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (gD3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            CleanupRenderTarget();
            HRESULT result = gSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main(int argc, char** argv)
{
    bool SilentMode = false;
    std::string PreLoadFilePath;
    std::cout << LINE_STRING << std::endl;
    if (argc == 1)
    {
        std::cout << "No argument\n";
    }
    else
    {
        if (strcmp(argv[1], "-s") == 0)
        {
            SilentMode = true;
            PreLoadFilePath = argv[2];
        }
        else
        {
            PreLoadFilePath = argv[1];
        }
        std::cout << PreLoadFilePath.c_str() << std::endl;
    }

    // Create application window
    UINT Width = 1690;
    UINT Height = 960;
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Line Data Generator", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Line Data Generator", WS_OVERLAPPEDWINDOW, 100, 100, 1690, 960, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    PreLoadFilePath = "F:\\OutlineDev\\LineRenderDev\\LineRenderDev\\Assets\\Models\\SphereAndTorus.triangles";
    gEditor = new Editor(hwnd, NUM_FRAMES_IN_FLIGHT, gD3dDevice, gD3dSrvDescHeap);
    gEditor->PreLoadFile(PreLoadFilePath);

    // Main loop
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = gSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = gMainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        gD3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        gD3dCommandList->ResourceBarrier(1, &barrier);

        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        gD3dCommandList->ClearRenderTargetView(gMainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
        gD3dCommandList->ClearDepthStencilView(gMainDepthBufferDescriptor[backBufferIdx], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        gD3dCommandList->OMSetRenderTargets(1, &gMainRenderTargetDescriptor[backBufferIdx], FALSE, &gMainDepthBufferDescriptor[backBufferIdx]);
        gD3dCommandList->SetDescriptorHeaps(1, &gD3dSrvDescHeap);
        
        // Rendering
        gEditor->Render(gD3dCommandList);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        gD3dCommandList->ResourceBarrier(1, &barrier);
        gD3dCommandList->Close();

        gD3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&gD3dCommandList);

        gSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = gFenceLastSignaledValue + 1;
        gD3dCommandQueue->Signal(gFence, fenceValue);
        gFenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

    WaitForLastSubmittedFrame();

    // Cleanup
    delete gEditor;

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

//int main(int argc, char** argv)
//{
//    bool SilentMode = false;
//    std::string FilePath;
//    std::cout << LINE_STRING << std::endl;
//    if (argc == 1)
//    {
//        std::cout << "No argument\n";
//    }
//    else
//    {
//        if (strcmp(argv[1], "-s") == 0)
//        {
//            SilentMode = true;
//            FilePath = argv[2];
//        }
//        else
//        {
//            FilePath = argv[1];
//        }
//        std::cout << FilePath.c_str() << std::endl;
//    }
//    //temp
//    //FilePath = "F:\\OutlineDev\\LineRenderDev\\LineRenderDev\\Assets\\Models\\MeetMat.triangles";//SphereAndTorus WoodPen BoxAndBox Platonic YJS_Mod
//    
//    if (FilePath.size() == 0)
//    {
//        std::cout << "Input File path:" << std::endl;
//        std::cin >> FilePath;
//    }
//    std::cout << LINE_STRING << std::endl;
//
//    {
//        AdjacencyProcesser Processer;
//        bool Success = false;
//
//        /*****Pass 0*****/////////////////////////////////////////////////////////////////////////////////////////
//        /*
//        * Merge duplicate vertex
//        */
//        Pass0(&Processer, FilePath);
//        /*****Pass 0*****/////////////////////////////////////////////////////////////////////////////////////////
//
//        /*****Pass 1*****/////////////////////////////////////////////////////////////////////////////////////////
//        /*
//        * Generate face list and edge list(without repeated)
//        */
//        Pass1(&Processer, FilePath);
//        /*****Pass 1*****/////////////////////////////////////////////////////////////////////////////////////////
//
//
//        /*****Pass 2*****/////////////////////////////////////////////////////////////////////////////////////////
//        /*
//        * Generate adjacency face list
//        */
//        Pass2(&Processer, FilePath);
//        /*
//        * Generate adjacency edge list(edge with adjacency vertex index)
//        */
//        /*****Pass 2*****/////////////////////////////////////////////////////////////////////////////////////////
//
//
//        /*****Pass 3*****/////////////////////////////////////////////////////////////////////////////////////////
//        /*
//        * Remove repeated adjacency face
//        */
//        Pass3(&Processer, FilePath);
//        /*****Pass 3*****/////////////////////////////////////////////////////////////////////////////////////////
//
//        ///////////////////////
//
//        /*****Pass 4*****/////////////////////////////////////////////////////////////////////////////////////////
//        /*
//        * Generate vertex to adjacency vertex map
//        */
//        Pass4(&Processer, FilePath);
//        /*****Pass 4*****/////////////////////////////////////////////////////////////////////////////////////////
//
//        /*****Pass 5*****/////////////////////////////////////////////////////////////////////////////////////////
//        /*
//        * Generate vertex to adjacency vertex list(serialize)
//        */
//        Pass5(&Processer, FilePath);
//        /*****Pass 5*****/////////////////////////////////////////////////////////////////////////////////////////
//
//        ///////////////////////
//        
//        //DebugCheck
//        //std::vector<SourceContext*> ContextList = Processer.GetTriangleContextList();
//        //SourceContext* Context = ContextList[0];
//        //ContextList[0]->DumpEdgeList();
//        //ContextList[0]->DumpFaceList();
//        //ContextList[0]->DumpEdgeFaceList();
//        //ContextList[0]->DumpAdjacencyFaceListShrink();
//        //ContextList[0]->DumpAdjacencyVertexList();
//        //std::vector<VertexContext*> VContextList = Processer.GetVertexContextList();
//        //VContextList[0]->DumpIndexMap();
//        //VContextList[0]->DumpVertexList();
//
//        Export(&Processer, FilePath);
//    }
//
//    if (!SilentMode) {
//        char AnyKey[16];
//        std::cout << std::endl;
//        std::cout << "Input any key to exit" << std::endl;
//        std::cin >> AnyKey;
//    }
//    return 0;
//}

