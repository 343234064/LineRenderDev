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
        NearZ(1.0),
        FarZ(5000.0),
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

class Mesh
{
public:
    Mesh():
        Name(""),
        VertexNum(0),
        VertexBuffer(nullptr)
    {
        IndexNum[0] = 0;
        IndexNum[1] = 0;
        IndexBuffer[0] = nullptr;
        IndexBuffer[1] = nullptr;
        IndexBufferView[0] = {};
        IndexBufferView[1] = {};
        VertexBufferView = {};
    }

    void Clear()
    {
        if (VertexBuffer) {
            VertexBuffer->Release();
            VertexBuffer = nullptr;
        }

        if (IndexBuffer[0]) {
            IndexBuffer[0]->Release();
            IndexBuffer[0] = nullptr;
        }
        if (IndexBuffer[1]) {
            IndexBuffer[1]->Release();
            IndexBuffer[1] = nullptr;
        }
        IndexNum[0] = 0;
        IndexNum[1] = 0;
        VertexNum = 0;
    }

public:
    std::string Name;
    int IndexNum[2];
    int VertexNum;
    ID3D12Resource* IndexBuffer[2];
    ID3D12Resource* VertexBuffer;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView[2];
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

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


class MeshRenderer
{
public:
    MeshRenderer(ID3D12Device* device, ID3D12DescriptorHeap* d3dSrcDescHeap):
        D3dDevice(device),
        D3dSrcDescriptorHeap(d3dSrcDescHeap),
        RootSignature(nullptr),
        SolidPipelineState(nullptr),
        LinePipelineState(nullptr),
        ConstantsBuffer(nullptr)
    {
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

    bool LoadMeshFromProcesser(AdjacencyProcesser* Processer);
    void RenderModel(const Float3& DisplaySize, ID3D12GraphicsCommandList* CommandList);

    void ShowViewerSettingUI(AdjacencyProcesser* Processer, bool ModelIsLoaded, bool GeneratorIsWorking);


private:
    bool InitRenderPipeline();
    void ClearResource();
    void Release()
    {
        if (ConstantsBuffer)
        {
            ConstantsBuffer->Release();
            ConstantsBuffer = nullptr;
        }
        if (SolidPipelineState) {
            SolidPipelineState->Release();
            SolidPipelineState = nullptr;
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
 
private:
    HintText Hint;
    std::vector<Mesh> MeshList;
    Constants ConstantParams;
    Camera RenderCamera;
    BoundingBox TotalBounding;

private:
    ID3D12Device* D3dDevice;
    ID3D12DescriptorHeap* D3dSrcDescriptorHeap;

    ID3D12RootSignature* RootSignature;
    ID3D12PipelineState* SolidPipelineState;
    ID3D12PipelineState* LinePipelineState;
    ID3D12Resource* ConstantsBuffer;

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

};




