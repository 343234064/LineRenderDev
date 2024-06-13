using System;
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

    public override string ToString() => $"({x}, {y})";
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

    public override string ToString() => $"({x}, {y}, {z})";
}

public struct Array4<T>
{
    public Array4(T _x, T _y, T _z, T _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

    public T x;
    public T y;
    public T z;
    public T w;

    public override string ToString() => $"({x}, {y}, {z}, {w})";
}




public struct LineSegment
{
    Vector4 NDCPosition1;
    Vector4 NDCPosition2;

    Vector2 Direction;

    uint BackFacing;

    int Id;

    uint RunTimeVertexIndex1;
    uint RunTimeVertexIndex2;

    uint MeshletIndex1;
    uint MeshletIndex2;

    public override string ToString() => $"LineSegment(Id={Id}))";

    public static int Size()
    {
        return sizeof(float) * 4 * 2 + sizeof(float) * 2 + sizeof(uint) + sizeof(int) + sizeof(uint) * 2 + sizeof(uint) * 2;
    }
}
public struct VisibleLineSegment
{
    int Id;
    uint PixelLength;
    uint PixelLengthShrink;
    uint PixelLengthShrinkTotal;
    uint ClippedPositionOffset;
    float InvPixelLengthBeforeClipped;

    public static int Size()
    {
        return sizeof(int) + sizeof(uint) * 4 + sizeof(float);
    }
}

public struct Slice
{
    uint Id;
    uint BeginSegmentIndex;
    uint BeginPixel;

    public override string ToString() => $"Slice({Id}, {BeginSegmentIndex}, {BeginPixel})";

    public static int Size()
    {
        return sizeof(uint) * 3;
    }
}

public struct PlainLine
{
    Vector2 NDCPosition1;
    Vector2 NDCPosition2;

    float PixelPosition1;
    float PixelPosition2;

    uint Id1;
    int Id2;

    uint RunTimeVertexIndex1;
    uint RunTimeVertexIndex2;

    int LinkState1;
    int LinkState2;

    uint Visibility;

    uint MeshletIndex1;
    uint MeshletIndex2;

    public override string ToString() => $"PlainLine()";

    public static int Size()
    {
        return sizeof(float) * 4 + sizeof(float) * 2 + sizeof(uint) + sizeof(int) + sizeof(uint) * 2 + sizeof(int) * 2 + sizeof(uint) + sizeof(uint) * 2;
    }
}

public struct RunTimeVertexState
{

    uint AdjNum;

    int Left;
    int Right;

    public override string ToString() => $"R({AdjNum})";

    public static int Size()
    {
        return sizeof(uint) + sizeof(int) * 2;
    }
}

public struct LineMesh
{

    public static int Size()
    {
        return sizeof(float) * 2 * 4 + sizeof(float) * 2 * 4 + sizeof(float) * 2 + sizeof(uint);
    }
}






public class LineShader
{
    public ComputeShader ResetPassShader;
    public ComputeShader ExtractPassShader;
    public ComputeShader SlicePassShader;
    public ComputeShader VisibilityPassShader;
    public ComputeShader GeneratePassShader;

    public bool IsValid()
    {
        if (ResetPassShader == null)
            return false;
        if (ExtractPassShader == null)
            return false;
        else if (SlicePassShader == null)
            return false;
        else if (VisibilityPassShader == null)
            return false;
        else if (GeneratePassShader == null)
            return false;

        return true;
    }
}


public struct RenderParams
{
    public Vector3 LocalCameraPosition;
    public float CreaseAngleThreshold;
    public Matrix4x4 WorldViewProjectionMatrix;
    public Matrix4x4 WorldViewProjectionMatrixForClipping;
    public Vector4 ScreenScaledResolution;
    public float LineWidth;
    public float LineCenterOffset;
    public Vector3 ObjectScale;
}

public struct RenderConstants
{
    public uint TotalAdjacencyTrianglesNum;
    public uint SilhouetteEnable;
    public uint CreaseEnable;
    public uint BorderEnable;
    public uint HideVisibleEdge;
    public uint HideBackFaceEdge;
    public uint HideOccludedEdge;
   

    public static int Size()
    {
        return sizeof(uint) * 7;
    }
}


public class LineRuntimeContext
{
    public PackedLineData MetaData;
    public LineMaterial LineMaterialSetting;
    public Transform RumtimeTransform;

    public ComputeBuffer ConstantsBuffer;

    public ComputeBuffer AdjacencyBuffer;
    public ComputeBuffer VerticesBuffer;

    public ComputeBuffer SegmentBuffer;
    public ComputeBuffer VisibleSegmentBuffer;
    public ComputeBuffer SegmentArgBuffer;
    public uint SegmentArgBufferCounterOffset;
    public uint SegmentArgBufferDispatchOffset;
    public uint SegmentArgBufferVisibleSegmentNumOffset;

    public ComputeBuffer BucketBuffer;
    public ComputeBuffer SliceBuffer;
    public ComputeBuffer SliceArgBuffer;
    public uint SliceArgBufferDispatchOffset;

    public ComputeBuffer VisibleLineBuffer;
    public ComputeBuffer VisibleLineArgBuffer;
    public uint VisibleLineArgBufferCounterOffset;
    public uint VisibleLineArgBufferDispatchOffset;

    public ComputeBuffer RunTimeVertexBuffer;
    public uint SegmentArgBufferRunTimeVertexNumOffset;

    public ComputeBuffer LineMeshBuffer;
    public ComputeBuffer LineMeshArgBuffer;
    public uint LineMeshArgBufferTriangleCounterOffset;
    public uint LineMeshArgBufferDrawArgOffset;



    public LineRuntimeContext(Transform transform, LineMaterial material)
    {
        MetaData = null;
        RumtimeTransform = transform;
        LineMaterialSetting = material;

        ConstantsBuffer = null;

        AdjacencyBuffer = null;
        VerticesBuffer = null;

        SegmentBuffer = null;
        VisibleSegmentBuffer = null;
        SegmentArgBuffer = null;
        SegmentArgBufferDispatchOffset = 0;
        SegmentArgBufferCounterOffset = 0;
        SegmentArgBufferVisibleSegmentNumOffset = 0;

        BucketBuffer = null;
        SliceBuffer = null;
        SliceArgBuffer = null;
        SliceArgBufferDispatchOffset = 0;

        VisibleLineBuffer = null;
        VisibleLineArgBuffer = null;
        VisibleLineArgBufferCounterOffset = 0;
        VisibleLineArgBufferDispatchOffset = 0;

        RunTimeVertexBuffer = null;
        SegmentArgBufferRunTimeVertexNumOffset = 0;

        LineMeshBuffer = null;
        LineMeshArgBuffer = null;
        LineMeshArgBufferTriangleCounterOffset = 0;
        LineMeshArgBufferDrawArgOffset = 0;
    }

    public bool Init(PackedLineData metaData)
    {
        if (metaData == null)
            return false;
        MetaData = metaData;

        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();
        ConstantsBuffer = new ComputeBuffer(1, RenderConstants.Size(), ComputeBufferType.Constant);
        RenderConstants[] Constants = new RenderConstants[1];
        Constants[0].TotalAdjacencyTrianglesNum = (uint)MetaData.AdjacencyFaceList.Length;
        Constants[0].SilhouetteEnable = (uint)(LineMaterialSetting.SilhouetteEnable ? 1 : 0);
        Constants[0].CreaseEnable = (uint)(LineMaterialSetting.CreaseEnable ? 1 : 0);
        Constants[0].BorderEnable = (uint)(LineMaterialSetting.BorderEnable ? 1 : 0);
        Constants[0].HideVisibleEdge = (uint)(LineMaterialSetting.HideVisibleEdge ? 1 : 0);
        Constants[0].HideBackFaceEdge = (uint)(LineMaterialSetting.HideBackFaceEdge ? 1 : 0);
        Constants[0].HideOccludedEdge = (uint)(LineMaterialSetting.HideOccludedEdge ? 1 : 0);
        ConstantsBuffer.SetData(Constants);


        if (AdjacencyBuffer != null)
            AdjacencyBuffer.Release();
        AdjacencyBuffer = new ComputeBuffer(MetaData.AdjacencyFaceList.Length, AdjFace.Size());
        AdjacencyBuffer.SetData(MetaData.AdjacencyFaceList);

        if (VerticesBuffer != null)
            VerticesBuffer.Release();
        VerticesBuffer = new ComputeBuffer(MetaData.VertexList.Length, AdjVertex.Size());
        VerticesBuffer.SetData(MetaData.VertexList);


        if (SegmentBuffer != null)
            SegmentBuffer.Release();
        SegmentBuffer = new ComputeBuffer(MetaData.AdjacencyFaceList.Length * 3, LineSegment.Size());
        SegmentBuffer.SetCounterValue(0);
        if (VisibleSegmentBuffer != null)
            VisibleSegmentBuffer.Release();
        VisibleSegmentBuffer = new ComputeBuffer(MetaData.AdjacencyFaceList.Length * 3, VisibleLineSegment.Size(), ComputeBufferType.Append);
        if (SegmentArgBuffer != null)
            SegmentArgBuffer.Release();
        SegmentArgBuffer = new ComputeBuffer(6, sizeof(uint), ComputeBufferType.IndirectArguments);
        uint[] SegmentArg = new uint[6] { 0, 1, 1, 1, 0, 0 }; // visble instance count, visble instance count/1024+1, 1, 1(for next dispatch x y z), total instance count(used for id), RumTimeVertexId Num
        SegmentArgBuffer.SetData(SegmentArg);
        SegmentArgBufferCounterOffset = 0;
        SegmentArgBufferDispatchOffset = sizeof(uint);
        SegmentArgBufferVisibleSegmentNumOffset = sizeof(uint) * 4;
        SegmentArgBufferRunTimeVertexNumOffset = sizeof(uint) * 5;


        if (BucketBuffer != null)
            BucketBuffer.Release();
        BucketBuffer = new ComputeBuffer(1024, sizeof(uint));
        if (SliceBuffer != null)
            SliceBuffer.Release();
        // Screen.currentResolution.width * Screen.currentResolution.height / 128 is not a accurate value
        int SliceNumPredict = Screen.currentResolution.width * Screen.currentResolution.height / 128;
        SliceBuffer = new ComputeBuffer(SliceNumPredict, Slice.Size());
        if (SliceArgBuffer != null)
            SliceArgBuffer.Release();
        SliceArgBuffer = new ComputeBuffer(3, sizeof(uint), ComputeBufferType.IndirectArguments);
        uint[] SliceArg = new uint[3] { 0, 1, 1}; // slice count, 1, 1(for next dispatch x y z)
        SliceArgBuffer.SetData(SliceArg);
        SliceArgBufferDispatchOffset = 0;

        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        VisibleLineBuffer = new ComputeBuffer(SliceNumPredict * 64, PlainLine.Size());
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
        VisibleLineArgBuffer = new ComputeBuffer(4, sizeof(uint), ComputeBufferType.IndirectArguments);
        //uint[] VisibleLineArg = new uint[4] { 2, 1, 0, 0 }; // vertex count, instance count, start vertex, start instance
        uint[] VisibleLineArg = new uint[4] { 0, 1, 1, 1 }; // visible line count,  visible line count/256+1, 1, 1(for next dispatch x y z)
        VisibleLineArgBuffer.SetData(VisibleLineArg);
        VisibleLineArgBufferCounterOffset = 0;
        VisibleLineArgBufferDispatchOffset = sizeof(uint);


        if (RunTimeVertexBuffer != null)
            RunTimeVertexBuffer.Release();
        RunTimeVertexBuffer = new ComputeBuffer(MetaData.VertexList.Length, RunTimeVertexState.Size());
        RunTimeVertexState[] RunTimeVertexArray = new RunTimeVertexState[MetaData.VertexList.Length];
        Array.Clear(RunTimeVertexArray, 0, MetaData.VertexList.Length);
        RunTimeVertexBuffer.SetData(RunTimeVertexArray);


        if (LineMeshBuffer != null)
            LineMeshBuffer.Release();
        // warning : MetaData.AdjacencyFaceList.Length * 3 * 4 is not a precise number, may cause error behaviour
        LineMeshBuffer = new ComputeBuffer(MetaData.AdjacencyFaceList.Length * 3 * 4, LineMesh.Size()); 
        if (LineMeshArgBuffer != null)
            LineMeshArgBuffer.Release();
        LineMeshArgBuffer = new ComputeBuffer(4, sizeof(uint), ComputeBufferType.IndirectArguments);
        uint[] LineMeshArg = new uint[4] { 3, 0, 0, 0 }; // vertex count, triangle count, start vertex, start instance
        LineMeshArgBuffer.SetData(LineMeshArg);
        LineMeshArgBufferTriangleCounterOffset = sizeof(uint);
        LineMeshArgBufferDrawArgOffset = 0;

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
        if (VisibleSegmentBuffer != null)
            VisibleSegmentBuffer.Release();
        if (SegmentArgBuffer != null)
            SegmentArgBuffer.Release();

        if (BucketBuffer != null)
            BucketBuffer.Release();
        if (SliceBuffer != null)
            SliceBuffer.Release();
        if (SliceArgBuffer != null)
            SliceArgBuffer.Release();


        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();

        if (RunTimeVertexBuffer != null)
            RunTimeVertexBuffer.Release();

        if (LineMeshBuffer != null)
            LineMeshBuffer.Release();
        if (LineMeshArgBuffer != null)
            LineMeshArgBuffer.Release();
    }

}





public class RenderLayer
{
    public RenderParams EveryFrameParams;

    private bool Available;
    private ComputePass ResetPass;
    private ComputePass ExtractPass;
    private ComputePass SlicePassPart1;
    private ComputePass SlicePassPart2;
    private ComputePass SlicePassPart3;
    private ComputePass VisiblePass;
    private ComputePass GeneratePass;

    private CommandBuffer RenderCommands;
    private ComputeBuffer EmptyBuffer;

    public RenderLayer()
    {
        Available = false;
        RenderCommands = new CommandBuffer();
        EveryFrameParams = new RenderParams();

        ResetPass = new ComputePass();
        ExtractPass = new ComputePass();
        SlicePassPart1 = new ComputePass();
        SlicePassPart2 = new ComputePass();
        SlicePassPart3 = new ComputePass();
        VisiblePass = new ComputePass();
        GeneratePass = new ComputePass();

        EmptyBuffer = null;
    }

    public bool Init(LineShader InputShaders)
    {
        Available = false;
        if (!InputShaders.IsValid())
            return false;

        RenderCommands.name = "Draw Lines";

        if (!ResetPass.Init("ResetPass", "Reset", InputShaders.ResetPassShader))
            return false;
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
        if (!GeneratePass.Init("GeneratePass", "Generate", InputShaders.GeneratePassShader))
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

        ResetPass.Destroy();
        ExtractPass.Destroy();
        SlicePassPart1.Destroy();
        SlicePassPart2.Destroy();
        SlicePassPart3.Destroy();
        VisiblePass.Destroy();
        GeneratePass.Destroy();

        if (EmptyBuffer != null)
            EmptyBuffer.Release();

        Available = false;
        Debug.Log("Render Layer Destroy");
    }

    public void Render(LineRuntimeContext Current)
    {
        if (!Available)
            return;

  
        //Reset
        //Temp, maybe do this in better way
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "RunTimeVertexBuffer", Current.RunTimeVertexBuffer);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "SegmentArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "SliceArgBuffer", Current.SliceArgBuffer);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "VisibleLineArgBuffer", Current.VisibleLineArgBuffer);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "LineMeshArgBuffer", Current.LineMeshArgBuffer);
        RenderCommands.DispatchCompute(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, ((int)(Current.VerticesBuffer.count / ResetPass.CoreShaderGroupSize.x) + 1), 1, 1);

        /*
         *  ExtractPass
         *  Limited:
         *  Direct3D 11 DirectCompute 5.0:
         *  interlockedAdd
         *  numthreads max: D3D11_CS_THREAD_GROUP_MAX_X (1024) D3D11_CS_THREAD_GROUP_MAX_Y (1024) D3D11_CS_THREAD_GROUP_MAX_Z (64)
         *  dispatch max: D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION (65535)
         *  Max 65535 * 256  -> 16,776,960 tris
         */
        RenderCommands.SetComputeVectorParam(ExtractPass.CoreShader, "LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        RenderCommands.SetComputeFloatParam(ExtractPass.CoreShader, "CreaseAngleThreshold", EveryFrameParams.CreaseAngleThreshold);
        RenderCommands.SetComputeMatrixParam(ExtractPass.CoreShader, "WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrix);
        RenderCommands.SetComputeMatrixParam(ExtractPass.CoreShader, "WorldViewProjectionForClipping", EveryFrameParams.WorldViewProjectionMatrixForClipping);
        RenderCommands.SetComputeVectorParam(ExtractPass.CoreShader, "ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
        RenderCommands.SetComputeConstantBufferParam(ExtractPass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "AdjacencyTriangles", Current.AdjacencyBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "Vertices", Current.VerticesBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "SegmentArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "Segments", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "VisibleSegments", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "RunTimeVertexBuffer", Current.RunTimeVertexBuffer);
        RenderCommands.SetBufferCounterValue(Current.VisibleSegmentBuffer, 0);
        RenderCommands.DispatchCompute(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, ((int)(Current.AdjacencyBuffer.count / ExtractPass.CoreShaderGroupSize.x) + 1), 1, 1);
        RenderCommands.CopyCounterValue(Current.VisibleSegmentBuffer, Current.SegmentArgBuffer, Current.SegmentArgBufferCounterOffset);


        /*
         * Slice Pass
         * Limited:
         * Max 1024 * 1024 -> 1,048,576 extracted edges
         */
        RenderCommands.SetComputeBufferParam(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, "InputArray", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, "SegmentArgBuffer", Current.SegmentArgBuffer);

        RenderCommands.DispatchCompute(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, Current.SegmentArgBuffer, Current.SegmentArgBufferDispatchOffset);

        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "InputArray", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "SegmentArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "BucketArray", Current.BucketBuffer);
        RenderCommands.DispatchCompute(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, 1, 1, 1); //Max 1024 * 1024

        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "InputArray", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "BucketArray", Current.BucketBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "SegmentArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "SliceArgBuffer", Current.SliceArgBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "Slices", Current.SliceBuffer);
        RenderCommands.DispatchCompute(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, Current.SegmentArgBuffer, Current.SegmentArgBufferDispatchOffset);

        /*
         * Visibility Pass
         *  Limited:
         *  Direct3D 11 DirectCompute 5.0:
         *  firstbithigh/firstbitlow, interlockedAnd, interlockedOr
         */
        RenderTargetIdentifier DepthRTIdentifier = new RenderTargetIdentifier(BuiltinRenderTextureType.Depth);
        RenderCommands.SetComputeConstantBufferParam(VisiblePass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeVectorParam(VisiblePass.CoreShader, "ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
        RenderCommands.SetComputeTextureParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "SceneDepthTexture", DepthRTIdentifier);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "Segments", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "VisibleSegments", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "Slices", Current.SliceBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "SegmentArgBuffer", Current.SegmentArgBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "SliceArgBuffer", Current.SliceArgBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "VisibleLineArgBuffer", Current.VisibleLineArgBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "VisibleLines", Current.VisibleLineBuffer);
        RenderCommands.DispatchCompute(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, Current.SliceArgBuffer, Current.SliceArgBufferDispatchOffset);



        /*
         * Chainning Pass
         *  
         */
        RenderCommands.SetComputeFloatParam(GeneratePass.CoreShader, "LineExtractWidth", EveryFrameParams.LineWidth); 
        RenderCommands.SetComputeFloatParam(GeneratePass.CoreShader, "LineCenterOffset", EveryFrameParams.LineCenterOffset);
        RenderCommands.SetComputeVectorParam(GeneratePass.CoreShader, "ObjectScale", EveryFrameParams.ObjectScale);
        RenderCommands.SetComputeVectorParam(GeneratePass.CoreShader, "ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "VisibleLineArgBuffer", Current.VisibleLineArgBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "VisibleLines", Current.VisibleLineBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "Segments", Current.SegmentBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "RunTimeVertexBuffer", Current.RunTimeVertexBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "LineMeshArgBuffer", Current.LineMeshArgBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "LineMeshBuffer", Current.LineMeshBuffer);
        RenderCommands.DispatchCompute(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, Current.VisibleLineArgBuffer, Current.VisibleLineArgBufferDispatchOffset);



        /*
         * Draw Mesh
         */
        Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("Lines", Current.LineMeshBuffer);
        RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterial, -1, MeshTopology.Triangles, Current.LineMeshArgBuffer, (int)Current.LineMeshArgBufferDrawArgOffset);


        /*
         * 等最终输出完成后，搞个debug数据输出最终线条数量、像素总长度以及limit最大限制
         */
        //RenderCommands.WaitAllAsyncReadbackRequests();
        //ComputeBufferUtils.Instance.PrintInLine<PlainLine>(Current.VisibleLineBuffer);
        //ComputeBufferUtils.Instance.PrintInLine<RunTimeVertexState>(Current.RunTimeVertexBuffer, 0, 80);
        //ComputeBufferUtils.Instance.PrintInLine<int>(Current.RunTimeVertexIdBuffer, 0, 310);
        //RenderCommands.WaitAllAsyncReadbackRequests();
        
        //uint[] Array = new uint[1];
        //Current.SliceArgBuffer.GetData(Array, 0, 0, 1);
        //Debug.Log("SliceCount:" + Array[0]);
        //ComputeBufferUtils.Instance.PrintInLine<Slice>(Current.SliceBuffer, 0, (int)Array[0]);
        //ComputeBufferUtils.Instance.PrintInLine<uint>(Current.SegmentArgBuffer, 0, 4);
        //ComputeBufferUtils.Instance.GetCount(Current.SliceBuffer);
        //Debug.Log("VCount:" + ComputeBufferUtils.Instance.GetCount(Current.VisibleLineBuffer));
        //Debug.Log("LCount:" + ComputeBufferUtils.Instance.GetCount(Current.LineMeshBuffer));
        //ComputeBufferUtils.Instance.PrintInLine<uint>(Current.VisibleLineArgBuffer, 0, 4);
        //ComputeBufferUtils.Instance.PrintInLine<uint>(Current.LineMeshArgBuffer, 0, 4);
        //Current.RunTimeVertexBuffer.SetData(Current.RunTimeVertexArray);
        //Current.RunTimeVertexIdBuffer.SetData(Current.RunTImeVertexIdArray);
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

