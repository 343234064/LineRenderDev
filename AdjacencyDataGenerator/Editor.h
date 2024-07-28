#pragma once

#include <stdio.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <functional>
#include <d3d12.h>
#include <DirectXMath.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include "Adjacency.h"



class HintText
{
public:
    HintText():
        Text(""), Color(ImVec4(1.0,1.0,1.0,1.0))
    {}
    
    void Error(const char* T)
    {
        Text = T;
        Color = ImVec4(1.0, 0.0, 0.0, 1.0);
    }

    void Normal(const char* T)
    {
        Text = T;
        Color = ImVec4(1.0, 1.0, 1.0, 1.0);
    }

    void ErrorColor()
    {
        Color = ImVec4(1.0, 0.0, 0.0, 1.0);
    }

    void NormalColor()
    {
        Color = ImVec4(1.0, 1.0, 1.0, 1.0);
    }

public:
    std::string Text;
    ImVec4 Color;
};


typedef std::function<bool(AdjacencyProcesser* Processer, std::string& FilePath, std::string& State)> PassType;

class Camera
{
public:
    Camera():
        Position(0.0),
        FovY(30.8),
        AspectRatio(1.0),
        NearZ(0.1),
        FarZ(10000.0),
        MoveSpeed(0.2),
        RotateSpeed(0.2),
        LookAt(0.0),
        Up(0.0,1.0,0.0)
    {
        UpdateDirection();
    }

    void Reset(BoundingBox& TotalModelBounding)
    {
        Position = TotalModelBounding.Center - Float3(TotalModelBounding.HalfLength.x, 0.0, TotalModelBounding.HalfLength.z) * 2.0f;
        LookAt = TotalModelBounding.Center;
        Up = Float3(0.0, 1.0, 0.0);
        UpdateDirection();
    }

    void UpdateDirection()
    {
        Forward = Normalize(LookAt - Position);
        Left = Normalize(Cross(Up, Forward));
        Up = Normalize(Cross(Forward, Left));
    }

    //void Rotate(Float3 Rotation)
    //{
    //    Float3 SinAngle;
    //    Float3 CosAngle;
    //    DirectX::XMScalarSinCos(&SinAngle.x, &CosAngle.x, Rotation.x * RotateSpeed);
    //    DirectX::XMScalarSinCos(&SinAngle.y, &CosAngle.y, Rotation.y * RotateSpeed);
    //    DirectX::XMScalarSinCos(&SinAngle.z, &CosAngle.z, Rotation.z * RotateSpeed);

    //    Float3 R1 = Float3(CosAngle.z * CosAngle.y + SinAngle.z * SinAngle.x * SinAngle.y, CosAngle.z * SinAngle.x * SinAngle.y - SinAngle.z * CosAngle.y, CosAngle.x * SinAngle.y);
    //    Float3 R2 = Float3(SinAngle.z * CosAngle.x, CosAngle.z * CosAngle.x, -SinAngle.x);
    //    Float3 R3 = Float3(SinAngle.z * SinAngle.x * CosAngle.y - CosAngle.z * SinAngle.y, SinAngle.z * SinAngle.y + CosAngle.z * SinAngle.x * CosAngle.y, CosAngle.x * CosAngle.y);

    //    Position = Float3(Dot(Position, R1), Dot(Position, R2), Dot(Position, R3));

    //}

    void Move(Float3 MoveOffset)
    {
        Float3 Origin = LookAt - Position;
        Float3 Trans = Float3(Dot(Origin, Left), Dot(Origin, Up), Dot(Origin, Forward));

        Trans = Trans + MoveOffset * MoveSpeed;

        LookAt = Float3(Dot(Trans, Float3(Left.x, Up.x, Forward.x)), Dot(Trans, Float3(Left.y, Up.y, Forward.y)), Dot(Trans, Float3(Left.z, Up.z, Forward.z))) + Position;
        Position = Position + (Left * MoveOffset.x + Up * MoveOffset.y + Forward * MoveOffset.z) * MoveSpeed;
    }

    void RotateAround(Float3 Direction)
    {
        Position = Position + Forward * Direction.z * MoveSpeed;
        float ViewLengthPrev = Length(Position - LookAt);
        Position = Position + (Left * Direction.x + Up * Direction.y) * MoveSpeed;

        Position = LookAt + Normalize(Position - LookAt) * ViewLengthPrev;
    }



public:
    Float3 Position;
    float FovY;
    float AspectRatio;
    float NearZ;
    float FarZ;
    float MoveSpeed;
    float RotateSpeed;

    Float3 LookAt;
    Float3 Forward;
    Float3 Left;
    Float3 Up;

};


class MeshData
{
public:
    MeshData() :
        VertexNum(0),
        VertexBuffer(nullptr)
    {
        IndexNum = 0;
        IndexBuffer = nullptr;
        IndexBufferView = {};
        VertexBufferView = {};
    }


    void Clear()
    {
        if (VertexBuffer) {
            VertexBuffer->Release();
            VertexBuffer = nullptr;
        }

        if (IndexBuffer) {
            IndexBuffer->Release();
            IndexBuffer = nullptr;
        }

        IndexNum = 0;
        VertexNum = 0;
    }

public:
    int IndexNum;
    int VertexNum;
    ID3D12Resource* IndexBuffer;
    ID3D12Resource* VertexBuffer;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

};
class Mesh
{
public:
    Mesh():
        Name("")
    {
    }


    void Clear()
    {
        Triangle.Clear();
        FaceNormal.Clear();
        VertexNormal.Clear();
    }

public:
    std::string Name;
    MeshData Triangle;
    MeshData FaceNormal;
    MeshData VertexNormal;

    BoundingBox Bounding;
};



class Constants
{
public:
    Constants() :
        GPUAddress(nullptr)
    {
        memset(WorldViewProjection, 0, sizeof(float) * 4 * 4);
    }

    size_t Size()
    {
        return sizeof(float) * 4 * 4;
    }

    void Copy()
    {
        if (GPUAddress)
            memcpy(GPUAddress, WorldViewProjection, Size());
    }

public:
    void* GPUAddress;
    float  WorldViewProjection[4][4];
};
class MaterialParameters
{
public:
    MaterialParameters() :
        GPUAddress(nullptr)
    {
        DisplayMode = 0;
        DisplayTransparent0 = 0.0f;
        DisplayTransparent1 = 0.0f;
        DisplayTransparent2 = 0.0f;
        DisplayTransparent3 = 0.0f;
    }

    size_t Size()
    {
        return sizeof(int) + sizeof(float) + sizeof(float) + sizeof(float) + sizeof(float);
    }

    void Copy()
    {
        if (GPUAddress) {
            unsigned char* Dest = reinterpret_cast<unsigned char*>(GPUAddress);
            memcpy(Dest, &DisplayMode, sizeof(int));
            Dest += sizeof(int);
            memcpy(Dest, &DisplayTransparent0, sizeof(float));
            Dest += sizeof(float);
            memcpy(Dest, &DisplayTransparent1, sizeof(float));
            Dest += sizeof(float);
            memcpy(Dest, &DisplayTransparent2, sizeof(float));
            Dest += sizeof(float);
            memcpy(Dest, &DisplayTransparent3, sizeof(float));
        }
    }

public:
    void* GPUAddress;
    int  DisplayMode;
    float DisplayTransparent0;
    float DisplayTransparent1;
    float DisplayTransparent2;
    float DisplayTransparent3;
};


class MeshRenderer
{
public:
    MeshRenderer(ID3D12Device* device, ID3D12DescriptorHeap* d3dSrcDescHeap):
        D3dDevice(device),
        D3dSrcDescriptorHeap(d3dSrcDescHeap),
        RootSignature(nullptr),
        SolidPipelineState(nullptr),
        WireFramePipelineState(nullptr),
        LinePipelineState(nullptr),
        ConstantsBuffer(nullptr)
    {
        ShowWireFrame = false;
        ShowFaceNormal = false;
        ShowVertexNormal = false;
        ShowAdjacencyFace = false;
        ShowMeshletLayer = false;
        AdjacencyFaceTransparent = 1.0f;
        MeshletLayerTransparent[0] = 1.0f;
        MeshletLayerTransparent[1] = 0.0f;
        MeshletLayerTransparent[2] = 0.0f;

        if (!InitRenderPipeline())
        {
            Hint.Error("Init Renderer Error");
        }
    }
    ~MeshRenderer()
    {
        ClearResource();
        D3dDevice = nullptr;

        Release();
    }

    void LoadMesh(AdjacencyProcesser* Processer)
    {
        bool Success = LoadMeshFromProcesser(Processer);
        if (!Success)
        {
            Hint.Error("Load Mesh Failed.");
        }
        Sygnal = false;
    }
    bool NeedWaitForNextFrame()
    {
        return Sygnal;
    }

    
    void RenderModel(const Float3& DisplaySize, ID3D12GraphicsCommandList* CommandList);
    void ShowViewerSettingUI(bool ModelIsLoaded, bool GeneratorIsWorking);


private:
    bool InitRenderPipeline();
    void ClearResource();
    void Release()
    {
        if (ParametersBuffer)
        {
            ParametersBuffer->Release();
            ParametersBuffer = nullptr;
        }
        if (ConstantsBuffer)
        {
            ConstantsBuffer->Release();
            ConstantsBuffer = nullptr;
        }
        if (SolidPipelineState) {
            SolidPipelineState->Release();
            SolidPipelineState = nullptr;
        }
        if (WireFramePipelineState) {
            WireFramePipelineState->Release();
            WireFramePipelineState = nullptr;
        }
        if (LinePipelineState) {
            LinePipelineState->Release();
            LinePipelineState = nullptr;
        }
        if (RootSignature) {
            RootSignature->Release();
            RootSignature = nullptr;
        }
    }

    void UpdateEveryFrameState(const Float3& DisplaySize);
    bool LoadMeshFromProcesser(AdjacencyProcesser* Processer);




private:
    HintText Hint;
    bool ShowWireFrame;
    bool ShowFaceNormal;
    bool ShowVertexNormal;
    bool ShowAdjacencyFace;
    float AdjacencyFaceTransparent;
    bool ShowMeshletLayer;
    float MeshletLayerTransparent[3];

private:
    std::vector<Mesh> MeshList;
    Constants ConstantParams;
    MaterialParameters MaterialParams;
    Camera RenderCamera;
    BoundingBox TotalBounding;
    bool Sygnal;

private:
    ID3D12Device* D3dDevice;
    ID3D12DescriptorHeap* D3dSrcDescriptorHeap;

    ID3D12RootSignature* RootSignature;
    ID3D12PipelineState* SolidPipelineState;
    ID3D12PipelineState* WireFramePipelineState;
    ID3D12PipelineState* LinePipelineState;
    ID3D12Resource* ConstantsBuffer;
    ID3D12Resource* ParametersBuffer;


};


class Editor
{
public:
    Editor(HWND hwnd, int numFramesInFlight, ID3D12Device* d3dDevice, ID3D12DescriptorHeap* d3dSrcDescHeap);
    ~Editor();

    void PreLoadFile(std::string& FilePath)
    {
        if (FilePath.size() != 0) {
            PreLoad = true;
            TriangleFilePath = FilePath;
        }
    }
    void Render(ID3D12GraphicsCommandList* CommandList);

    bool NeedWaitForSumittedFrame()
    {
        if (!MeshViewer.get()) return false;
        return MeshViewer->NeedWaitForNextFrame();
    }

    void OnLastFrameFinished()
    {
        if (MeshViewer.get())
            MeshViewer->LoadMesh(Processer.get());
    }

private:
    void OnLoadButtonClicked();
    void OnGenerateButtonClicked();
    void OnOneProgressFinished();
    void OnAllProgressFinished();

    void RenderBackground(ID3D12GraphicsCommandList* CommandList);
    double GetProgress();

private:
    HintText Hint;

private:
    void BrowseFile();
    bool KickGenerateMission();
    void ClearPassPool();

private:
    std::unique_ptr<AdjacencyProcesser> Processer;
    std::unique_ptr<MeshRenderer> MeshViewer;

    std::queue<PassType> PassPool;
    double Progress;

    std::string TriangleFilePath;

    bool Terminated;
    bool Working;
    bool Loaded;
    bool PreLoad;
    int CurrentPassIndex;


};




