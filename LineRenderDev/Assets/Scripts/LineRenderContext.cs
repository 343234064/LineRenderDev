using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;





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
    public ComputeShader ContourizePassShader;
    public ComputeShader SlicePassShader;
    public ComputeShader VisibilityPassShader;
    public ComputeShader GeneratePassShader;

    public bool IsValid()
    {
        if (ResetPassShader == null)
            return false;
        if (ExtractPassShader == null)
            return false;
        if (ContourizePassShader == null)
            return false;
        if (SlicePassShader == null)
            return false;
        if (VisibilityPassShader == null)
            return false;
        if (GeneratePassShader == null)
            return false;

        return true;
    }
}


public struct RenderParams
{
    public Vector3 WorldCameraPosition;
    public Vector3 LocalCameraPosition;
    public float CreaseAngleThreshold;
    public Matrix4x4 WorldViewProjectionMatrix;
    public Matrix4x4 WorldViewProjectionMatrixForDebug;
    public Vector4 ScreenScaledResolution;
    public float LineWidth;
    public float LineCenterOffset;
    public Vector3 ObjectScale;
}

public struct RenderConstants
{
    public uint TotalFacesNum;
    public uint TotalVerticesNum;
    public uint TotalMeshletsNum;

    public uint SilhouetteEnable;
    public uint CreaseEnable;
    public uint BorderEnable;
    public uint HideVisibleEdge;
    public uint HideBackFaceEdge;
    public uint HideOccludedEdge;
   

    public static int Size()
    {
        return sizeof(uint) * 9;
    }
}




public class LineRuntimeContext
{
    public PackedLineData MetaData;
    public LineMaterial LineMaterialSetting;
    public Transform RumtimeTransform;
    public ComputeBuffer ConstantsBuffer;

    // mesh data
    public ComputeBuffer VertexBuffer;
    public ComputeBuffer FaceBuffer;
    public ComputeBuffer MeshletBuffer;

    //runtime data
    public ArgBufferLayout ArgLayout;
    public ComputeBuffer ArgBuffer1;
    public ComputeBuffer SegmentBuffer;
    public ComputeBuffer ContourFaceBuffer;


    public LineRuntimeContext(Transform transform, LineMaterial material)
    {
        MetaData = null;
        RumtimeTransform = transform;
        LineMaterialSetting = material;
        ConstantsBuffer = null;

        VertexBuffer = null;
        FaceBuffer = null;
        MeshletBuffer = null;

        ArgLayout = new ArgBufferLayout();
        ArgBuffer1 = null;
        SegmentBuffer = null;
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
        Constants[0].TotalFacesNum = MetaData.FaceNum;
        Constants[0].TotalVerticesNum = MetaData.VertexNum;
        Constants[0].TotalMeshletsNum = MetaData.MeshletNum;
        Constants[0].SilhouetteEnable = (uint)(LineMaterialSetting.SilhouetteEnable ? 1 : 0);
        Constants[0].CreaseEnable = (uint)(LineMaterialSetting.CreaseEnable ? 1 : 0);
        Constants[0].BorderEnable = (uint)(LineMaterialSetting.BorderEnable ? 1 : 0);
        Constants[0].HideVisibleEdge = (uint)(LineMaterialSetting.HideVisibleEdge ? 1 : 0);
        Constants[0].HideBackFaceEdge = (uint)(LineMaterialSetting.HideBackFaceEdge ? 1 : 0);
        Constants[0].HideOccludedEdge = (uint)(LineMaterialSetting.HideOccludedEdge ? 1 : 0);

        ConstantsBuffer.SetData(Constants);

        // mesh data
        if (VertexBuffer != null)
            VertexBuffer.Release();
        VertexBuffer = new ComputeBuffer((int)MetaData.VertexNum, (int)MetaData.VertexStructSize);
        VertexBuffer.SetData(MetaData.VertexList);

        if (FaceBuffer != null)
            FaceBuffer.Release();
        FaceBuffer = new ComputeBuffer((int)MetaData.FaceNum, (int)MetaData.FaceStructSize);
        FaceBuffer.SetData(MetaData.FaceList);

        if (MeshletBuffer != null)
            MeshletBuffer.Release();
        MeshletBuffer = new ComputeBuffer((int)MetaData.MeshletNum, (int)MetaData.MeshletStructSize);
        MeshletBuffer.SetData(MetaData.MeshletList);


        // runtime data
        if (ArgBuffer1 != null)
            ArgBuffer1.Release();
        ArgBuffer1 = new ComputeBuffer(ArgLayout.Count(), (int)ArgLayout.StrideSize(), ComputeBufferType.IndirectArguments);
        ArgBuffer1.SetData(ArgLayout.Buffer);

        if (SegmentBuffer != null)
            SegmentBuffer.Release();
        /*
         * In practice, for general man-made triangular meshes,McGuire (2004) measured empirically a trend closer to pow(faceNum, 0.8)
         * M. McGuire. 《Observations on silhouette sizes》
         */
        
        double SegmentSize = MetaData.EdgeNum + Math.Pow(MetaData.FaceNum, 0.8);
        SegmentBuffer = new ComputeBuffer((int)SegmentSize, Segment.Size());

        if (ContourFaceBuffer != null)
            ContourFaceBuffer.Release();
        ContourFaceBuffer = new ComputeBuffer((int)MetaData.FaceNum, ContourFace.Size());
   

        return true;
    }

    public void Destroy()
    {
        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();

        if (VertexBuffer != null)
            VertexBuffer.Release();

        if (FaceBuffer != null)
            FaceBuffer.Release();

        if (MeshletBuffer != null)
            MeshletBuffer.Release();

        if (ArgBuffer1 != null)
            ArgBuffer1.Release();

        if (SegmentBuffer != null)
            SegmentBuffer.Release();

        if (ContourFaceBuffer != null)
            ContourFaceBuffer.Release();
    }

}





public class RenderLayer
{
    public RenderParams EveryFrameParams;

    private bool Available;
    private ComputePass ResetPass;
    private ComputePass ExtractPass;
    private ComputePass ContourizePass;
    private ComputePass SlicePassPart1;
    private ComputePass SlicePassPart2;
    private ComputePass SlicePassPart3;
    private ComputePass VisiblePass;
    private ComputePass GeneratePass;

    private CommandBuffer RenderCommands;
    private ComputeBuffer EmptyBuffer;

    private LocalKeyword DebugKeyword;

    public RenderLayer()
    {
        Available = false;
        RenderCommands = new CommandBuffer();
        EveryFrameParams = new RenderParams();

        ResetPass = new ComputePass();
        ExtractPass = new ComputePass();
        ContourizePass = new ComputePass();
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
        if (!ContourizePass.Init("ContourizePass", "Contourize", InputShaders.ContourizePassShader))
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

        DebugKeyword = new LocalKeyword(InputShaders.ExtractPassShader, "ENABLE_DEBUG_CLIPPING");

        Available = true;
        Debug.Log("Render Layer Init");

        return true;
    }

    public void Destroy()
    {
        RenderCommands.Release();

        ResetPass.Destroy();
        ExtractPass.Destroy();
        ContourizePass.Destroy();
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

    public void Render(LineRuntimeContext Current, bool EnableDebug)
    {
        if (!Available)
            return;

        //Reset
        //Temp, maybe do this in better way
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "ArgBuffer", Current.ArgBuffer1);
        RenderCommands.DispatchCompute(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, 1, 1, 1);

        /*
         *  ExtractPass
         *  Limited:
         *  Direct3D 11 DirectCompute 5.0:
         *  interlockedAdd
         *  numthreads max: D3D11_CS_THREAD_GROUP_MAX_X (1024) D3D11_CS_THREAD_GROUP_MAX_Y (1024) D3D11_CS_THREAD_GROUP_MAX_Z (64)
         *  dispatch max: D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION (65535)
         *  Max 65535 * 256  -> 16,776,960 tris
         */
        RenderCommands.SetComputeConstantBufferParam(ExtractPass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeVectorParam(ExtractPass.CoreShader, "LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        //RenderCommands.SetComputeFloatParam(ExtractPass.CoreShader, "CreaseAngleThreshold", EveryFrameParams.CreaseAngleThreshold);
        RenderCommands.SetComputeMatrixParam(ExtractPass.CoreShader, "WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrix);
        RenderCommands.SetComputeMatrixParam(ExtractPass.CoreShader, "WorldViewProjectionForClipping", EveryFrameParams.WorldViewProjectionMatrixForDebug);
        //RenderCommands.SetComputeVectorParam(ExtractPass.CoreShader, "ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "VertexBuffer", Current.VertexBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "FaceBuffer", Current.FaceBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "MeshletBuffer", Current.MeshletBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "ContourFaceBuffer", Current.ContourFaceBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "SegmentBuffer", Current.SegmentBuffer);

        //RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "SegmentList", Current.SegmentBuffer);
        //RenderCommands.SetKeyword(ExtractPass.CoreShader, DebugKeyword, EnableDebug);
        //RenderCommands.SetBufferCounterValue(Current.SegmentBuffer, 0);
        RenderCommands.DispatchCompute(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, (int)Current.MetaData.MeshletNum, 1, 1);
        //Debug.Log(Current.MetaData.FaceNum);
        //RenderCommands.DispatchCompute(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, (int)(Current.MetaData.FaceNum / 256) + 1, 1, 1);
        //RenderCommands.CopyCounterValue(Current.SegmentBuffer, Current.ArgBuffer1, Current.ArgLayout.SegmentCountOffset);


        /*
         * Contourize Pass
         * 
         */
        RenderCommands.SetComputeConstantBufferParam(ContourizePass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeVectorParam(ContourizePass.CoreShader, "LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        RenderCommands.SetComputeMatrixParam(ContourizePass.CoreShader, "WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrix);
        RenderCommands.SetComputeMatrixParam(ContourizePass.CoreShader, "WorldViewProjectionForClipping", EveryFrameParams.WorldViewProjectionMatrixForDebug);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "VertexBuffer", Current.VertexBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "ContourFaceBuffer", Current.ContourFaceBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "SegmentBuffer", Current.SegmentBuffer);
        RenderCommands.SetKeyword(ContourizePass.CoreShader, DebugKeyword, EnableDebug);
        RenderCommands.DispatchCompute(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, Current.ArgBuffer1, (uint)Current.ArgLayout.ContourizePassDispatchCount());


        /*
         * Draw Mesh
         */
        if (EnableDebug)
        {
            Current.LineMaterialSetting.LineRenderMaterialForDebug.SetBuffer("SegmentList", Current.SegmentBuffer);
            RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterialForDebug, -1, MeshTopology.Lines, Current.ArgBuffer1, Current.ArgLayout.IndirectDrawStart());
        }
        else {
            Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("SegmentList", Current.SegmentBuffer);
            RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterial, -1, MeshTopology.Lines, Current.ArgBuffer1, (int)Current.ArgLayout.IndirectDrawStart());
        }
        Current.LineMaterialSetting.SetMainCameraPosition(EveryFrameParams.WorldCameraPosition);



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

