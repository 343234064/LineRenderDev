#include "Editor.h"
#include <ShObjIdl_core.h>
#include <d3dcompiler.h>




std::string ToUtf8(const std::wstring& str)
{
    std::string ret;
    int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0, NULL, NULL);
    if (len > 0)
    {
        ret.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.length(), &ret[0], len, NULL, NULL);
    }
    return ret;
}


Editor::Editor(HWND hwnd, int numFramesInFlight, ID3D12Device* d3dDevice, ID3D12DescriptorHeap* d3dSrcDescHeap)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(d3dDevice, numFramesInFlight,
        DXGI_FORMAT_R8G8B8A8_UNORM, d3dSrcDescHeap,
        d3dSrcDescHeap->GetCPUDescriptorHandleForHeapStart(),
        d3dSrcDescHeap->GetGPUDescriptorHandleForHeapStart());

    Processer.reset(new AdjacencyProcesser());
    MeshViewer.reset(new MeshRenderer(d3dDevice, d3dSrcDescHeap));

    TriangleFilePath = "NO File Loaded";
    Progress = 0.0;

    Terminated = false;
    Working = false;
    Loaded = false;
}

Editor::~Editor()
{
    MeshViewer.release();
    Processer.release();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}


void Editor::Render(ID3D12GraphicsCommandList* CommandList)
{
    // Render 3d model
    RenderBackground(CommandList);

    // Render UI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        static float f = 0.0f;
        static int counter = 0;
        ImGuiIO& io = ImGui::GetIO();

        //ImGuiWindowFlags window_flags = 0;
        //window_flags |= ImGuiWindowFlags_NoResize;
        //window_flags |= ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos(ImVec2(4, 6), ImGuiCond_FirstUseEver);
        //ImGui::Begin("Inspector", NULL, window_flags);
        ImGui::Begin("Inspector");
        ImGui::SetWindowSize(ImVec2(750, 250), ImGuiCond_FirstUseEver);

        ImGui::Text("FPS: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Text("");

        ImGui::BeginDisabled(Working);
        if (ImGui::Button("Load Triangle File"))
        {
            OnLoadButtonClicked();
        }
        ImGui::EndDisabled();

        ImGui::Text(TriangleFilePath.c_str());
        ImGui::Text("");
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Text("");

        ImGui::BeginDisabled(Working);
        if (ImGui::Button("Generate") || PreLoad)
        {
            if (PreLoad) PreLoad = false;
            OnGenerateButtonClicked();
        }
        ImGui::EndDisabled();

        ImGui::ProgressBar(GetProgress(), ImVec2(-1.0f, 0.0f));
        ImGui::TextColored(Hint.Color, Hint.Text.c_str());

        ImGui::End();
    }

    if (MeshViewer)
        MeshViewer->ShowViewerSettingUI(Processer.get(), Loaded, Working);
   
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);
}

void Editor::BrowseFile()
{
    IFileOpenDialog* pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pfd));

    std::string SeletedFile;
    if (SUCCEEDED(hr))
    {
        DWORD dwFlags;
        COMDLG_FILTERSPEC rgSpec[] =
        {
            { L"TRIANGLES", L"*.triangles" }
        };

        hr = pfd->GetOptions(&dwFlags);
        hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
        hr = pfd->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);

        hr = pfd->Show(NULL);
        if (SUCCEEDED(hr))
        {
            IShellItemArray* psiaResults;
            hr = pfd->GetResults(&psiaResults);
            if (SUCCEEDED(hr))
            {
                DWORD resultNum;
                hr = psiaResults->GetCount(&resultNum);
                if (resultNum >= 1)
                {
                    IShellItem* psiaResult;
                    hr = psiaResults->GetItemAt(0, &psiaResult);
                    PWSTR pszFilePath = NULL;
                    hr = psiaResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr))
                    {
                        //MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        std::wstring FilePath = std::wstring(pszFilePath);
                        TriangleFilePath = ToUtf8(FilePath);
                    }
                    CoTaskMemFree(pszFilePath);
                }
            }
            psiaResults->Release();
        }
        pfd->Release();
    }
    CoUninitialize();

    return;
}


void Editor::OnLoadButtonClicked()
{
   BrowseFile();
}


void Editor::OnGenerateButtonClicked()
{
    bool Success = false;
    if (Processer != nullptr)
        Success = KickGenerateMission();

    if (!Success)
    {
        Hint.Error("NO File Loaded OR Failed to start process...");
    }
}


void Editor::OnOneProgressFinished()
{
    // flush working mission
    while (true)
    {
        if (Processer->IsWorking())
            Processer->GetProgress();
        else
            break;
    }

    std::string& ErrorString = Processer->GetErrorString();
    if (ErrorString.size() > 0) {
        std::cout << LINE_STRING << std::endl;
        std::cout << "Something get error, please see error0.log." << std::endl;
        std::cout << LINE_STRING << std::endl;

        Processer->DumpErrorString(0);
    }
  
}

void Editor::OnAllProgressFinished()
{

    Export(Processer.get(), TriangleFilePath, Hint.Text);
    Loaded = true;
    MeshViewer->LoadMeshFromProcesser(Processer.get());
}


double Editor::GetProgress()
{
    if (Processer != nullptr)
        Progress = Processer->GetProgress();

    if (Progress == 1.0 && !PassPool.empty())
    {
        OnOneProgressFinished();
        PassType NextPass = PassPool.front();
        if (NextPass != nullptr) {
            bool Success = NextPass(Processer.get(), TriangleFilePath, Hint.Text);
            if (!Success)
            {
                Hint.ErrorColor();
                ClearPassPool();
                Terminated = true;
            }
            else {
                Hint.NormalColor();
                Progress = 0.0;
            }
        }

        PassPool.pop();
    }
    else if (Progress == 1.0 && PassPool.empty())
    {
        if (!Terminated && Working) {
            OnAllProgressFinished();
        }
        Working = false;
    }

    return Progress;
}

void Editor::ClearPassPool()
{
    if (!PassPool.empty())
    {
        std::queue<PassType> Empty;
        swap(Empty, PassPool);
    }
}


bool Editor::KickGenerateMission()
{
    bool Success = false;

    ClearPassPool();

    Success = PassGenerateVertexMap(Processer.get(), TriangleFilePath, Hint.Text);

    if (Success) {
        Progress = 0.0;
        Working = true;
        Terminated = false;
        Loaded = false;
        Hint.NormalColor();

        PassPool.push(PassType(PassGenerateFaceAndEdgeData));
        PassPool.push(PassType(PassGenerateAdjacencyData));
        PassPool.push(PassType(PassShrinkAdjacencyData));
        PassPool.push(PassType(PassGenerateAdjacencyVertexMap));
        PassPool.push(PassType(PassSerializeAdjacencyVertexMap));

        PassPool.push(PassType(PassGenerateRenderData));
    }
    else {
        Working = false;
        Terminated = true;
        Hint.ErrorColor();
    }

    return Success;
}



void Editor::RenderBackground(ID3D12GraphicsCommandList* CommandList)
{

    if (MeshViewer) {
     
        Float3 DisplaySize = Float3(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0);
        MeshViewer->RenderModel(DisplaySize, CommandList);

    }

}



void MeshRenderer::ShowViewerSettingUI(AdjacencyProcesser* Processer, bool ModelIsLoaded, bool GeneratorIsWorking)
{

    ImGui::SetNextWindowPos(ImVec2(4, 256), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug Viewer");
    ImGui::SetWindowSize(ImVec2(255, 508), ImGuiCond_FirstUseEver);
    ImGui::Separator();
    ImGui::BeginDisabled(GeneratorIsWorking);
    if (ImGui::Button("Render Model"))
    {
        if (!ModelIsLoaded)
        {
            Hint.Error("Please Run Generate First.");
            return;
        }

        bool Success = LoadMeshFromProcesser(Processer);
        if (!Success)
        {
            Hint.Error("Load Mesh Failed.");
        }
    }
    ImGui::TextColored(Hint.Color, Hint.Text.c_str());
    ImGui::EndDisabled();

    ImGui::SeparatorText("Camera Setting");

    if (ImGui::Button("Reset Camera"))
    {
        RenderCamera.Reset(TotalBounding);
    }
    ImGui::InputFloat("Move Speed", &RenderCamera.MoveSpeed, 0.1f, 1.0f, "%.3f");
    ImGui::InputFloat("FOV(Vertical)", &RenderCamera.FovY, 0.01f, 1.0f, "%.3f");

    ImGui::End();


    Float3 MoveDirection;
    Float3 MoveOffset;
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        MoveDirection.x += io.MouseDelta.x * 0.05f;//(io.MouseDelta.x > 0.0f ? 1.0f : (io.MouseDelta.x < 0.0f ? -1.0f : 0.0f));
        MoveDirection.y -= io.MouseDelta.y * 0.05f;//(io.MouseDelta.y > 0.0f ? 1.0f : (io.MouseDelta.y < 0.0f ? -1.0f : 0.0f));
    }
    MoveDirection.z += io.MouseWheel;
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        MoveOffset.x += io.MouseDelta.x * 0.05f;
        MoveOffset.y -= io.MouseDelta.y * 0.05f;
    }

    if (ImGui::IsKeyDown(ImGuiKey_W))
    {
        MoveDirection.z += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_S))
    {
        MoveDirection.z -= 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_A))
    {
        MoveDirection.x += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_D))
    {
        MoveDirection.x -= 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_Q))
    {
        MoveDirection.y += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_E))
    {
        MoveDirection.y -= 1.0f;
    }

    if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
    {
        MoveOffset.y += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
    {
        MoveOffset.y -= 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
    {
        MoveOffset.x += 1.0f;
    }
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
    {
        MoveOffset.x -= 1.0f;
    }

    RenderCamera.RotateAround(MoveDirection);
    RenderCamera.Move(MoveOffset);
    RenderCamera.UpdateDirection();

    if (ImGui::IsKeyDown(ImGuiKey_F))
    {
        RenderCamera.Reset(TotalBounding);
    }
}


bool MeshRenderer::LoadMeshFromProcesser(AdjacencyProcesser* Processer)
{
    if (D3dDevice == nullptr) return false;
    ClearResource();
    
    std::cout << LINE_STRING << std::endl;
    std::cout << "Begin Loading Mesh Data..." << std::endl;

    std::vector<SourceContext*>& SrcList = Processer->GetTriangleContextList();
    for (int i = 0; i < SrcList.size(); i++)
    {

        Mesh NewMesh;
        NewMesh.Name = SrcList[i]->Name;
        NewMesh.IndexNum[0] = SrcList[i]->FaceList.size() * 3;
        NewMesh.IndexNum[1] = SrcList[i]->FaceList.size() * 6;
        NewMesh.VertexNum = SrcList[i]->FaceList.size() * 3;
        NewMesh.Bounding = SrcList[i]->VertexData->Bounding;

        std::cout << "Loading Mesh : " << NewMesh.Name << std::endl;
        std::cout << "Index Num  : " << NewMesh.IndexNum[0] << std::endl;
        std::cout << "Vertex Num : " << NewMesh.VertexNum << std::endl;

        {
            D3D12_HEAP_PROPERTIES HeapProp;
            memset(&HeapProp, 0, sizeof(D3D12_HEAP_PROPERTIES));
            HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC Desc;
            memset(&Desc, 0, sizeof(D3D12_RESOURCE_DESC));
            Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            Desc.Width = NewMesh.VertexNum * sizeof(DrawRawVertex);
            Desc.Height = 1;
            Desc.DepthOrArraySize = 1;
            Desc.MipLevels = 1;
            Desc.Format = DXGI_FORMAT_UNKNOWN;
            Desc.SampleDesc.Count = 1;
            Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            if (D3dDevice->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&NewMesh.VertexBuffer)) < 0)
                return false;
        }

        {
            D3D12_HEAP_PROPERTIES HeapProp;
            memset(&HeapProp, 0, sizeof(D3D12_HEAP_PROPERTIES));
            HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC Desc;
            memset(&Desc, 0, sizeof(D3D12_RESOURCE_DESC));
            Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            Desc.Width = NewMesh.IndexNum[0] * sizeof(DrawRawIndex);
            Desc.Height = 1;
            Desc.DepthOrArraySize = 1;
            Desc.MipLevels = 1;
            Desc.Format = DXGI_FORMAT_UNKNOWN;
            Desc.SampleDesc.Count = 1;
            Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            if (D3dDevice->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&NewMesh.IndexBuffer[0])) < 0)
                return false;
        }

        {
            D3D12_HEAP_PROPERTIES HeapProp;
            memset(&HeapProp, 0, sizeof(D3D12_HEAP_PROPERTIES));
            HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC Desc;
            memset(&Desc, 0, sizeof(D3D12_RESOURCE_DESC));
            Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            Desc.Width = NewMesh.IndexNum[1] * sizeof(DrawRawIndex);
            Desc.Height = 1;
            Desc.DepthOrArraySize = 1;
            Desc.MipLevels = 1;
            Desc.Format = DXGI_FORMAT_UNKNOWN;
            Desc.SampleDesc.Count = 1;
            Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            if (D3dDevice->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&NewMesh.IndexBuffer[1])) < 0)
                return false;
        }

        void* VertexResource = nullptr;
        void* IndexResource = nullptr;
        void* IndexLineResource = nullptr;
        D3D12_RANGE Range;
        memset(&Range, 0, sizeof(D3D12_RANGE));

        if (NewMesh.VertexBuffer->Map(0, &Range, &VertexResource) != S_OK)
            return false;
        DrawRawVertex* VertexDest = (DrawRawVertex*)VertexResource;
        memcpy(VertexDest, SrcList[i]->VertexData->DrawVertexList, NewMesh.VertexNum * sizeof(DrawRawVertex));
        NewMesh.VertexBuffer->Unmap(0, &Range);

        if (NewMesh.IndexBuffer[0]->Map(0, &Range, &IndexResource) != S_OK)
            return false;
        DrawRawIndex* IndexDest = (DrawRawIndex*)IndexResource;
        memcpy(IndexDest, SrcList[i]->DrawIndexList, NewMesh.IndexNum[0] * sizeof(DrawRawIndex));
        NewMesh.IndexBuffer[0]->Unmap(0, &Range);

        if (NewMesh.IndexBuffer[1]->Map(0, &Range, &IndexLineResource) != S_OK)
            return false;
        DrawRawIndex* IndexLineDest = (DrawRawIndex*)IndexLineResource;
        memcpy(IndexLineDest, SrcList[i]->DrawIndexLineList, NewMesh.IndexNum[1] * sizeof(DrawRawIndex));
        NewMesh.IndexBuffer[1]->Unmap(0, &Range);

        NewMesh.VertexBufferView.BufferLocation = NewMesh.VertexBuffer->GetGPUVirtualAddress();
        NewMesh.VertexBufferView.StrideInBytes = sizeof(DrawRawVertex);
        NewMesh.VertexBufferView.SizeInBytes = NewMesh.VertexNum * sizeof(DrawRawVertex);

        NewMesh.IndexBufferView[0].BufferLocation = NewMesh.IndexBuffer[0]->GetGPUVirtualAddress();
        NewMesh.IndexBufferView[0].Format = DXGI_FORMAT_R32_UINT;
        NewMesh.IndexBufferView[0].SizeInBytes = NewMesh.IndexNum[0] * sizeof(DrawRawIndex);

        NewMesh.IndexBufferView[1].BufferLocation = NewMesh.IndexBuffer[1]->GetGPUVirtualAddress();
        NewMesh.IndexBufferView[1].Format = DXGI_FORMAT_R32_UINT;
        NewMesh.IndexBufferView[1].SizeInBytes = NewMesh.IndexNum[1] * sizeof(DrawRawIndex);

        std::cout << "Loading Mesh Finished : " << NewMesh.Name << std::endl;

        MeshList.push_back(NewMesh);
        TotalBounding.Resize(NewMesh.Bounding);
    }

    RenderCamera.Reset(TotalBounding);
    std::cout << "Loading Mesh Finished." << std::endl;
    std::cout << LINE_STRING << std::endl;

    return true;
}


void MeshRenderer::ClearResource()
{
    std::vector<Mesh>::iterator it;
    for (it = MeshList.begin(); it != MeshList.end(); it++)
    {
        it->Clear();
    }
    MeshList.clear();

    TotalBounding.Clear();
}


bool MeshRenderer::InitRenderPipeline()
{
    if (D3dDevice == nullptr || D3dSrcDescriptorHeap == nullptr) return false;

    {
        D3D12_DESCRIPTOR_RANGE DescriptorRange[1] = {};
        //CBV
        DescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        DescriptorRange[0].NumDescriptors = 1;
        DescriptorRange[0].BaseShaderRegister = 0;
        DescriptorRange[0].RegisterSpace = 0;
        DescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER RootParameters[1] = {};
        //CBV
        RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        RootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(DescriptorRange);
        RootParameters[0].DescriptorTable.pDescriptorRanges = &DescriptorRange[0];

        D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
        RootSignatureDesc.NumParameters = _countof(RootParameters);
        RootSignatureDesc.pParameters = RootParameters;
        RootSignatureDesc.NumStaticSamplers = 0;
        RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* Blob = nullptr;
        if (FAILED(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Blob, nullptr)))
        {
            std::cout << "Create RootSignatureBlob Failed." << std::endl;
            RootSignature = nullptr;
        }

        if (FAILED(D3dDevice->CreateRootSignature(0, Blob->GetBufferPointer(), Blob->GetBufferSize(), IID_PPV_ARGS(&RootSignature))))
        {
            std::cout << "Create RootSignature Failed." << std::endl;
            RootSignature = nullptr;
        }

        if(Blob) Blob->Release();
    }
    if (!RootSignature)
    {
        Release();
        return false;
    }

    {
        ID3DBlob* BlobVertexShader1 = nullptr;
        ID3DBlob* BlobVertexShader2 = nullptr;
        ID3DBlob* BlobPixelShader1 = nullptr;
        ID3DBlob* BlobPixelShader2 = nullptr;
        ID3DBlob* BlobVertexCompileMsg = nullptr;
        ID3DBlob* BlobPixelCompileMsg = nullptr;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT CompileFlags = 0;
#endif
        CompileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        TCHAR	 ExePath[MAX_PATH] = {};
        ::GetModuleFileName(NULL, ExePath, MAX_PATH);
        std::filesystem::path ExePathW = std::wstring(ExePath);
        std::filesystem::path ShaderPath = ExePathW.replace_filename(std::filesystem::path("Shaders/BaseShader.hlsl"));
        
        bool Success = true;
        D3D_SHADER_MACRO Defines1[] = { {"DRAW_LINE", "0"} , {NULL, NULL} };
        D3D_SHADER_MACRO Defines2[] = { {"DRAW_LINE", "1"} , {NULL, NULL} };

        std::cout << "Compile Shader Path : " << ShaderPath.generic_string() << std::endl;
        if(FAILED(D3DCompileFromFile(ShaderPath.generic_wstring().c_str(), Defines1, nullptr, "VSMain", "vs_5_0", CompileFlags, 0, &BlobVertexShader1, &BlobVertexCompileMsg)))
        {
            if(BlobVertexCompileMsg)
                std::cout << (const char*)(BlobVertexCompileMsg->GetBufferPointer()) << std::endl;
            Success = false;
        }
        if (FAILED(D3DCompileFromFile(ShaderPath.generic_wstring().c_str(), Defines2, nullptr, "VSMain", "vs_5_0", CompileFlags, 0, &BlobVertexShader2, &BlobVertexCompileMsg)))
        {
            if (BlobVertexCompileMsg)
                std::cout << (const char*)(BlobVertexCompileMsg->GetBufferPointer()) << std::endl;
            Success = false;
        }
        if (BlobVertexCompileMsg)
            BlobVertexCompileMsg->Release();

        if (FAILED(D3DCompileFromFile(ShaderPath.generic_wstring().c_str(), Defines1, nullptr, "PSMain", "ps_5_0", CompileFlags, 0, &BlobPixelShader1, &BlobPixelCompileMsg)))
        {
            if (BlobPixelCompileMsg)
                std::cout << (const char*)(BlobPixelCompileMsg->GetBufferPointer()) << std::endl;
            Success = false;
        }
        if (FAILED(D3DCompileFromFile(ShaderPath.generic_wstring().c_str(), Defines2, nullptr, "PSMain", "ps_5_0", CompileFlags, 0, &BlobPixelShader2, &BlobPixelCompileMsg)))
        {
            if (BlobPixelCompileMsg)
                std::cout << (const char*)(BlobPixelCompileMsg->GetBufferPointer()) << std::endl;
            Success = false;
        }
        if (BlobPixelCompileMsg)
            BlobPixelCompileMsg->Release();

        if (!Success)
        {
            if (BlobVertexShader1)
                BlobVertexShader1->Release();
            if (BlobVertexShader2)
                BlobVertexShader2->Release();
            if (BlobPixelShader1)
                BlobPixelShader1->Release();
            if (BlobPixelShader2)
                BlobPixelShader2->Release();
            Release();
            std::cout << "Compile Shader Failed." << std::endl;
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateDesc = {};
        memset(&PipelineStateDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        PipelineStateDesc.InputLayout = { InputElementDesc, _countof(InputElementDesc) };
        PipelineStateDesc.pRootSignature = RootSignature;

        PipelineStateDesc.VS.pShaderBytecode = BlobVertexShader1->GetBufferPointer();
        PipelineStateDesc.VS.BytecodeLength = BlobVertexShader1->GetBufferSize();

        PipelineStateDesc.PS.pShaderBytecode = BlobPixelShader1->GetBufferPointer();
        PipelineStateDesc.PS.BytecodeLength = BlobPixelShader1->GetBufferSize();

        PipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        PipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

        PipelineStateDesc.BlendState.AlphaToCoverageEnable = FALSE;
        PipelineStateDesc.BlendState.IndependentBlendEnable = FALSE;
        PipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        PipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
        PipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        PipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        PipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
        
        PipelineStateDesc.SampleMask = UINT_MAX;
        PipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PipelineStateDesc.NumRenderTargets = 1;
        PipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        PipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        PipelineStateDesc.SampleDesc.Count = 1;
        PipelineStateDesc.SampleDesc.Quality = 0;

        if (FAILED(D3dDevice->CreateGraphicsPipelineState(&PipelineStateDesc, IID_PPV_ARGS(&SolidPipelineState))))
        {
            if (BlobVertexShader1)
                BlobVertexShader1->Release();
            if (BlobVertexShader2)
                BlobVertexShader2->Release();
            if (BlobPixelShader1)
                BlobPixelShader1->Release();
            if (BlobPixelShader2)
                BlobPixelShader2->Release();
            Release();
            std::cout << "Create Solid Pipeline State Failed." << std::endl;
            return false;
        }

        PipelineStateDesc.VS.pShaderBytecode = BlobVertexShader2->GetBufferPointer();
        PipelineStateDesc.VS.BytecodeLength = BlobVertexShader2->GetBufferSize();
        PipelineStateDesc.PS.pShaderBytecode = BlobPixelShader2->GetBufferPointer();
        PipelineStateDesc.PS.BytecodeLength = BlobPixelShader2->GetBufferSize();
        PipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        if (FAILED(D3dDevice->CreateGraphicsPipelineState(&PipelineStateDesc, IID_PPV_ARGS(&LinePipelineState))))
        {
            if (BlobVertexShader1)
                BlobVertexShader1->Release();
            if (BlobVertexShader2)
                BlobVertexShader2->Release();
            if (BlobPixelShader1)
                BlobPixelShader1->Release();
            if (BlobPixelShader2)
                BlobPixelShader2->Release();
            Release();
            std::cout << "Create Solid Pipeline State Failed." << std::endl;
            return false;
        }
    }


    {
        D3D12_HEAP_PROPERTIES HeapProp;
        memset(&HeapProp, 0, sizeof(D3D12_HEAP_PROPERTIES));
        HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC ConstantBufferDesc = {};
        ConstantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ConstantBufferDesc.Width = (ConstantParams.Size() + 255) & ~255; //Constant buffer must be aligned with 256
        ConstantBufferDesc.Height = 1;
        ConstantBufferDesc.DepthOrArraySize = 1;
        ConstantBufferDesc.MipLevels = 1;
        ConstantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        ConstantBufferDesc.SampleDesc.Count = 1;
        ConstantBufferDesc.SampleDesc.Quality = 0;
        ConstantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ConstantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        ConstantBufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        if (FAILED(D3dDevice->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ConstantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ConstantsBuffer))))
        {
            Release();
            std::cout << "Create Constant Buffer Failed." << std::endl;
            return false;
        }
        


        D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferViewDesc = {};
        ConstantBufferViewDesc.BufferLocation = ConstantsBuffer->GetGPUVirtualAddress();
        ConstantBufferViewDesc.SizeInBytes = (ConstantParams.Size() + 255) & ~255; //Constant buffer must be aligned with 256
        D3D12_CPU_DESCRIPTOR_HANDLE CPUCBVHandle = D3dSrcDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        CPUCBVHandle.ptr += D3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3dDevice->CreateConstantBufferView(&ConstantBufferViewDesc, CPUCBVHandle);

        D3D12_RANGE Range = {};
        memset(&Range, 0, sizeof(D3D12_RANGE));
        if (FAILED(ConstantsBuffer->Map(0, &Range, reinterpret_cast<void**>(&ConstantParams.GPUAddress))))
        {
            std::cout << "ConstantsBuffer Map Failed." << std::endl;
            return false;
        }
        ConstantParams.Copy();

     }

    return true;
}



void MeshRenderer::UpdateEveryFrameState(const Float3& DisplaySize)
{
    
    RenderCamera.AspectRatio = DisplaySize.x / DisplaySize.y;

    DirectX::XMVECTOR Forward = DirectX::XMVectorSet(RenderCamera.Forward.x, RenderCamera.Forward.y, RenderCamera.Forward.z, 0.0f);
    DirectX::XMVECTOR Left = DirectX::XMVectorSet(RenderCamera.Left.x, RenderCamera.Left.y, RenderCamera.Left.z, 0.0f);
    DirectX::XMVECTOR UpDir= DirectX::XMVectorSet(RenderCamera.Up.x, RenderCamera.Up.y, RenderCamera.Up.z, 0.0f);
    DirectX::XMVECTOR NegEyePosition = DirectX::XMVectorSet(-RenderCamera.Position.x, -RenderCamera.Position.y, -RenderCamera.Position.z, 1.0f);

    DirectX::XMVECTOR D0 = DirectX::XMVector3Dot(Left, NegEyePosition); 
    DirectX::XMVECTOR D1 = DirectX::XMVector3Dot(UpDir, NegEyePosition);
    DirectX::XMVECTOR D2 = DirectX::XMVector3Dot(Forward, NegEyePosition);
    DirectX::XMMATRIX ViewMatrix;
    DirectX::XMVECTORU32 Mask = { { { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 } } };
    ViewMatrix.r[0] = DirectX::XMVectorSelect(D0, Left, Mask);
    ViewMatrix.r[1] = DirectX::XMVectorSelect(D1, UpDir, Mask);
    ViewMatrix.r[2] = DirectX::XMVectorSelect(D2, Forward, Mask);
    ViewMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    ViewMatrix = DirectX::XMMatrixTranspose(ViewMatrix);

    //DirectX::XMVECTOR EyePos = DirectX::XMVectorSet(RenderCamera.Position.x, RenderCamera.Position.y, RenderCamera.Position.z, 1.0);
    //DirectX::XMVECTOR LookAt = DirectX::XMVectorSet(TotalBounding.Center.x, TotalBounding.Center.y, TotalBounding.Center.z, 1.0);
    //UpDir = DirectX::XMVectorSet(0.0, 1.0, 0.0, 0.0);
    //ViewMatrix = DirectX::XMMatrixLookAtLH(EyePos, LookAt, UpDir);

    DirectX::XMMATRIX ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(RenderCamera.FovY, RenderCamera.AspectRatio, RenderCamera.NearZ, RenderCamera.FarZ);

    DirectX::XMMATRIX MVP = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);

    _mm_storeu_ps(ConstantParams.WorldViewProjection[0], MVP.r[0]);
    _mm_storeu_ps(ConstantParams.WorldViewProjection[1], MVP.r[1]);
    _mm_storeu_ps(ConstantParams.WorldViewProjection[2], MVP.r[2]);
    _mm_storeu_ps(ConstantParams.WorldViewProjection[3], MVP.r[3]);

}

void MeshRenderer::RenderModel(const Float3& DisplaySize, ID3D12GraphicsCommandList* CommandList)
{
    if (MeshList.size() < 1 || RootSignature == nullptr) return;

    UpdateEveryFrameState(DisplaySize);
    
    ConstantParams.Copy();

    D3D12_VIEWPORT Viewport;
    memset(&Viewport, 0, sizeof(D3D12_VIEWPORT));
    Viewport.Width = DisplaySize.x;
    Viewport.Height = DisplaySize.y;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = Viewport.TopLeftY = 0.0f;
    CommandList->RSSetViewports(1, &Viewport);

    D3D12_RECT Scissor = {};
    Scissor.left = 0;
    Scissor.top = 0;
    Scissor.right = DisplaySize.x;
    Scissor.bottom = DisplaySize.y;
    CommandList->RSSetScissorRects(1, &Scissor);

    CommandList->SetGraphicsRootSignature(RootSignature);

    //Set Constant buffer
    D3D12_GPU_DESCRIPTOR_HANDLE GPUCBVHandle = D3dSrcDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    GPUCBVHandle.ptr += D3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CommandList->SetGraphicsRootDescriptorTable(0, GPUCBVHandle);

    CommandList->SetPipelineState(SolidPipelineState);
    for (std::vector<Mesh>::iterator it = MeshList.begin(); it != MeshList.end(); it++)
    {
        CommandList->IASetIndexBuffer(&(it->IndexBufferView[0]));
        CommandList->IASetVertexBuffers(0, 1, &(it->VertexBufferView));
        CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        CommandList->DrawIndexedInstanced(it->IndexNum[0], 1, 0, 0, 0);

    }

    CommandList->SetPipelineState(LinePipelineState);
    for (std::vector<Mesh>::iterator it = MeshList.begin(); it != MeshList.end(); it++)
    {
        CommandList->IASetIndexBuffer(&(it->IndexBufferView[1]));
        CommandList->IASetVertexBuffers(0, 1, &(it->VertexBufferView));
        CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        CommandList->DrawIndexedInstanced(it->IndexNum[1], 1, 0, 0, 0);

    }
}


