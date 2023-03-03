﻿
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


public struct Array2<T>
{
    public Array2(T _x, T _y)
    {
        x = _x;
        y = _y;
    }

    public T x;
    public T y;
}


public struct Array3<T>
{
    public Array3(T _x, T _y, T _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    public T x;
    public T y;
    public T z;
}





public struct LineSegment
{
    Vector3 LocalPosition1;
    Vector3 LocalPosition2;
    Vector3 ScreenPosition1;
    Vector3 ScreenPosition2;

    uint BackFacing;
    uint PixelLength;
    uint PixelLengthTotal;

    public override string ToString() => $"LineSegment(PixelLength={PixelLength}), PixelLengthTotal={PixelLengthTotal})";

    public static int Size()
    {
        return sizeof(float) * 3 * 2 + sizeof(float) * 3 * 2 + sizeof(uint) * 3;
    }
}

public struct Slice
{
    uint Visibility1;
    uint Visibility2;

    uint BeginSegmentIndex;
    uint BeginPixel;

    public override string ToString() => $"Slice(Index={BeginSegmentIndex}), Pixel={BeginPixel})";

    public static int Size()
    {
        return sizeof(uint) * 4;
    }
}


public class LineShader
{
    public ComputeShader ExtractPassShader;
    public ComputeShader SlicePassShader;
    public ComputeShader VisibilityPassShader;

    public bool IsValid()
    {
        if (ExtractPassShader == null)
            return false;
        else if (SlicePassShader == null)
            return false;
        else if (VisibilityPassShader == null)
            return false;

        return true;
    }
}


public struct RenderParams
{
    public Vector3 LocalCameraPosition;
    public float CreaseAngleThreshold;
    public Matrix4x4 WorldViewProjectionMatrix;
    public Matrix4x4 WorldViewMatrix;
    public Matrix4x4 ObjectWorldMatrix;
    public int ScreenWidthScaled;
    public int ScreenHeightScaled;
    public int ScreenWidthFixed;
    public int ScreenHeightFixed;

}

public struct RenderConstants
{
    public uint TotalAdjacencyTrianglesNum;
    public uint SilhouetteEnable;
    public uint CreaseEnable;
    public uint BorderEnable;
    public uint HideBackFaceEdge;
    public uint HideOccludedEdge;

    public static int Size()
    {
        return sizeof(uint) * 6;
    }
}


public class LineContext
{
    public Mesh RumtimeMesh;
    public LineMaterial LineMaterialSetting;
    public Transform RumtimeTransform;

    public ComputeBuffer ConstantsBuffer;

    public ComputeBuffer AdjacencyBuffer;
    public ComputeBuffer VerticesBuffer;

    public ComputeBuffer SegmentBuffer;
    public ComputeBuffer SegmentArgBuffer;
    public int SegmentArgBufferOffset;

    public ComputeBuffer BucketBuffer;
    public ComputeBuffer SliceBuffer;

    public ComputeBuffer VisibleLineBuffer;
    public ComputeBuffer VisibleLineArgBuffer;
    public int VisibleLineArgBufferOffset;

    public LineContext(Mesh meshObj, Transform transform, LineMaterial material)
    {
        RumtimeMesh = meshObj;
        RumtimeTransform = transform;
        LineMaterialSetting = material;

        ConstantsBuffer = null;

        AdjacencyBuffer = null;
        VerticesBuffer = null;

        SegmentBuffer = null;
        SegmentArgBuffer = null;
        SegmentArgBufferOffset = 0;

        BucketBuffer = null;
        SliceBuffer = null;

        VisibleLineBuffer = null;
        VisibleLineArgBuffer = null;        
        VisibleLineArgBufferOffset = 0;
    }

    public bool Init(AdjFace[] AdjTris)
    {
        if (AdjTris == null)
            return false;

        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();
        ConstantsBuffer = new ComputeBuffer(1, RenderConstants.Size(), ComputeBufferType.Constant);
        RenderConstants[] Constants = new RenderConstants[1];
        Constants[0].TotalAdjacencyTrianglesNum = (uint)AdjTris.Length;
        Constants[0].SilhouetteEnable = (uint)(LineMaterialSetting.SilhouetteEnable ? 1 : 0);
        Constants[0].CreaseEnable = (uint)(LineMaterialSetting.CreaseEnable ? 1 : 0);
        Constants[0].BorderEnable = (uint)(LineMaterialSetting.BorderEnable ? 1 : 0);
        Constants[0].HideBackFaceEdge = (uint)(LineMaterialSetting.HideBackFaceEdge ? 1 : 0);
        Constants[0].HideOccludedEdge = (uint)(LineMaterialSetting.HideOccludedEdge ? 1 : 0);
        ConstantsBuffer.SetData(Constants);


        if (AdjacencyBuffer != null)
            AdjacencyBuffer.Release();
        AdjacencyBuffer = new ComputeBuffer(AdjTris.Length, AdjFace.Size());
        AdjacencyBuffer.SetData(AdjTris);

        if (VerticesBuffer != null)
            VerticesBuffer.Release();
        VerticesBuffer = new ComputeBuffer(RumtimeMesh.vertices.Length, sizeof(float) * 3);
        VerticesBuffer.SetData(RumtimeMesh.vertices);



        if (SegmentBuffer != null)
            SegmentBuffer.Release();
        SegmentBuffer = new ComputeBuffer(AdjTris.Length * 3, LineSegment.Size(), ComputeBufferType.Append);
        SegmentBuffer.SetCounterValue(0);
        if (SegmentArgBuffer != null)
            SegmentArgBuffer.Release();
        SegmentArgBuffer = new ComputeBuffer(7, sizeof(int), ComputeBufferType.IndirectArguments);
        int[] SegmentArg = new int[7] { 0, 1, 1, 1, 0, 1, 1 }; // for slice pass counting, instance count/1024+1, 1, 1(for next dispatch x y z), instance count(temp), 1, 1(for next dispatch x y z), 
        SegmentArgBuffer.SetData(SegmentArg);
        SegmentArgBufferOffset = sizeof(int);

        if (BucketBuffer != null)
            BucketBuffer.Release();
        BucketBuffer = new ComputeBuffer(1024, sizeof(uint));
        if (SliceBuffer != null)
            SliceBuffer.Release();
        SliceBuffer = new ComputeBuffer(Screen.currentResolution.width * Screen.currentResolution.height / 128, Slice.Size(), ComputeBufferType.Append);
        SliceBuffer.SetCounterValue(0);

        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        VisibleLineBuffer = new ComputeBuffer(AdjTris.Length * 3, LineSegment.Size(), ComputeBufferType.Append);
        VisibleLineBuffer.SetCounterValue(0);
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
        VisibleLineArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        int[] VisibleLineArg = new int[4] { 2, 1, 0, 0 }; // vertex count, instance count, start vertex, start instance
        VisibleLineArgBuffer.SetData(VisibleLineArg);
        VisibleLineArgBufferOffset = sizeof(int);
        

        return true;
    }

    public void Destroy()
    {
        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();

        if (AdjacencyBuffer != null)
            AdjacencyBuffer.Release();
        if (VerticesBuffer != null)
            VerticesBuffer.Release();

        if (SegmentBuffer != null)
            SegmentBuffer.Release();
        if (SegmentArgBuffer != null)
            SegmentArgBuffer.Release();

        if (BucketBuffer != null)
            BucketBuffer.Release();
        if (SliceBuffer != null)
            SliceBuffer.Release();


        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
    }

}





public class RenderLayer
{
    public RenderParams EveryFrameParams;

    private bool Available;
    private ComputePass ExtractPass;
    private ComputePass SlicePassPart1;
    private ComputePass SlicePassPart2;
    private ComputePass SlicePassPart3;
    private ComputePass VisiblePass;

    private CommandBuffer RenderCommands;
    private ComputeBuffer EmptyBuffer;

    public RenderLayer()
    {
        Available = false;
        RenderCommands = new CommandBuffer();
        EveryFrameParams = new RenderParams();

        ExtractPass = new ComputePass();
        SlicePassPart1 = new ComputePass();
        SlicePassPart2 = new ComputePass();
        SlicePassPart3 = new ComputePass();
        VisiblePass = new ComputePass();

        EmptyBuffer = null;
    }

    public bool Init(LineShader InputShaders)
    {
        Available = false;
        if (!InputShaders.IsValid())
            return false;

        RenderCommands.name = "Draw Lines";

        if (!ExtractPass.Init("ExtractionPass", "Extraction", InputShaders.ExtractPassShader))
            return false;
        if (!SlicePassPart1.Init("SlicePassPart1", "SumSingleBucketInclusive", InputShaders.SlicePassShader))
            return false;
        if (!SlicePassPart2.Init("SlicePassPart2", "SumAllBucket", InputShaders.SlicePassShader))
            return false;
        if (!SlicePassPart3.Init("SlicePassPart3", "SumTotalAndSlice", InputShaders.SlicePassShader))
            return false;
        if (!VisiblePass.Init("VisibilityPass", "Visibility", InputShaders.VisibilityPassShader))
            return false;

        if (EmptyBuffer != null)
            EmptyBuffer.Release();
        EmptyBuffer = new ComputeBuffer(1, sizeof(int), ComputeBufferType.Append);
        EmptyBuffer.SetData(new int[1] { 0 });
        EmptyBuffer.SetCounterValue(0);

        Available = true;
        Debug.Log("Render Layer Init");

        return true;
    }

    public void Destroy()
    {
        RenderCommands.Release();

        ExtractPass.Destroy();
        SlicePassPart1.Destroy();
        SlicePassPart2.Destroy();
        SlicePassPart3.Destroy();
        VisiblePass.Destroy();

        if (EmptyBuffer != null)
            EmptyBuffer.Release();

        Available = false;
        Debug.Log("Render Layer Destroy");
    }

    public void Render(LineContext Current)
    {
        if (!Available)
            return;

        //Reset
        //Do this in slice pass
        RenderCommands.SetBufferCounterValue(EmptyBuffer, 0);
        RenderCommands.CopyCounterValue(EmptyBuffer, Current.SegmentArgBuffer, 0);
        RenderCommands.SetBufferCounterValue(EmptyBuffer, 1);
        RenderCommands.CopyCounterValue(EmptyBuffer, Current.SegmentArgBuffer, sizeof(int));
        /*
         *  ExtractPass
         *  Limited:
         *  Direct3D 11 DirectCompute 5.0:
         *  numthreads max: D3D11_CS_THREAD_GROUP_MAX_X (1024) D3D11_CS_THREAD_GROUP_MAX_Y (1024) D3D11_CS_THREAD_GROUP_MAX_Z (64)
         *  dispatch max: D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION (65535)
         *  Max 65535 * 256  -> 16,776,960 tris
         */
        RenderCommands.SetComputeVectorParam(ExtractPass.CoreShader, "LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        RenderCommands.SetComputeFloatParam(ExtractPass.CoreShader, "CreaseAngleThreshold", EveryFrameParams.CreaseAngleThreshold);
        RenderCommands.SetComputeMatrixParam(ExtractPass.CoreShader, "WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrix);
        //RenderCommands.SetComputeMatrixParam(ExtractPass.CoreShader, "WorldView", EveryFrameParams.WorldViewMatrix);
        RenderCommands.SetComputeIntParam(ExtractPass.CoreShader, "ScreenWidth", EveryFrameParams.ScreenWidthScaled);
        RenderCommands.SetComputeIntParam(ExtractPass.CoreShader, "ScreenHeight", EveryFrameParams.ScreenHeightScaled);
        RenderCommands.SetComputeConstantBufferParam(ExtractPass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "AdjacencyTriangles", Current.AdjacencyBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "Vertices", Current.VerticesBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "ArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "Segments", Current.SegmentBuffer);
        RenderCommands.SetBufferCounterValue(Current.SegmentBuffer, 0);
        RenderCommands.DispatchCompute(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, ((int)(Current.AdjacencyBuffer.count / ExtractPass.CoreShaderGroupSize.x) + 1), 1, 1);
        RenderCommands.CopyCounterValue(Current.SegmentBuffer, Current.SegmentArgBuffer, sizeof(int) * 4); //temp


        /*
         * Slice Pass
         * Limited:
         * Max 1024 * 1024 -> 1,048,576 edges
         */
        RenderCommands.SetComputeBufferParam(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, "InputArray", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, "ArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.DispatchCompute(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, Current.SegmentArgBuffer, (uint)Current.SegmentArgBufferOffset);

        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "InputArray", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "ArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "BucketArray", Current.BucketBuffer);
        RenderCommands.DispatchCompute(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, 1, 1, 1); //Max 1024 * 1024

        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "InputArray", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "ArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "BucketArray", Current.BucketBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "Slices", Current.SliceBuffer);
        RenderCommands.SetBufferCounterValue(Current.SliceBuffer, 0);
        RenderCommands.DispatchCompute(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, Current.SegmentArgBuffer, (uint)Current.SegmentArgBufferOffset);


        /*
         * Visibility Pass
         */
        RenderTargetIdentifier DepthRTIdentifier = new RenderTargetIdentifier(BuiltinRenderTextureType.Depth);
        RenderCommands.SetComputeTextureParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "SceneDepthTexture", DepthRTIdentifier);
        RenderCommands.SetComputeConstantBufferParam(VisiblePass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "Segments", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "Output2DLines", Current.VisibleLineBuffer);
        RenderCommands.SetBufferCounterValue(Current.VisibleLineBuffer, 0);
        RenderCommands.DispatchCompute(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, Current.SegmentArgBuffer, sizeof(int) * 4);
        RenderCommands.CopyCounterValue(Current.VisibleLineBuffer, Current.VisibleLineArgBuffer, (uint)Current.VisibleLineArgBufferOffset);


        Current.LineMaterialSetting.LineRenderMaterial.SetMatrix("ObjectWorldMatrix", EveryFrameParams.ObjectWorldMatrix);
        Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("Lines", Current.VisibleLineBuffer);


        RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterial, -1, MeshTopology.Lines, Current.VisibleLineArgBuffer, 0);

        //RenderCommands.WaitAllAsyncReadbackRequests();
        //ComputeBufferUtils.Instance.PrintInLine<LineSegment>(Current.SegmentBuffer);
        //ComputeBufferUtils.Instance.PrintInLine<Slice>(Current.SliceBuffer);
    }

    public CommandBuffer GetRenderCommand()
    {
        return RenderCommands;
    }

    public void ClearFrame()
    {
        
    }

    public void ClearCommandBuffer()
    {
        RenderCommands.Clear();
    }

}

