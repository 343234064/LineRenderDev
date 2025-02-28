using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;




public class LineShader
{
    public ComputeShader ResetPassShader;
    public ComputeShader ExtractPassShader;
    public ComputeShader ContourizePassShader;
    public ComputeShader SlicePassShader;
    public ComputeShader VisibilityPassShader;
    public ComputeShader GeneratePassShader;
    public ComputeShader ChainningPassShader;

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
        if (ChainningPassShader == null)
            return false;

        return true;
    }
}


public struct RenderParams
{
    public uint VisibilityFlag;
    public Vector3 WorldCameraPosition;
    public Vector3 LocalCameraPosition;
    public Vector3 LocalCameraForward;
    public Matrix4x4 WorldViewProjectionMatrixForRenderCamera;
    public Matrix4x4 WorldViewProjectionMatrixForDebugCamera;
    public Matrix4x4 InvWorldViewProjectionMatrixForRenderCamera;
    public float CreaseAngleThreshold;
    public Vector4 ScreenScaledResolution;
    public float LineWidthScale;
    public Vector3 ObjectScale;
    public float ChainningAngleThreshold;
}

public struct RenderConstants
{
    public uint TotalVerticesNum; 
    public uint TotalRepeatedVerticesNum;
    public uint TotalFacesNum;
    public uint TotalMeshletsNum;

    public uint ContourEnable;
    public uint CreaseEnable;
    public uint BorderEnable;

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

    // mesh data
    public ComputeBuffer VertexBuffer;
    public ComputeBuffer FaceBuffer;
    public ComputeBuffer MeshletBuffer;

    //runtime data
    public ArgBufferLayout ArgLayout;
    //pingpong arg buffer
    public ComputeBuffer ArgBuffer1;
    public ComputeBuffer ArgBuffer2;

    public ComputeBuffer ContourBuffer;
    public ComputeBuffer VisibleSegmentBuffer;
    public ComputeBuffer AllSegmentBuffer;
    public ComputeBuffer AnchorVertexBuffer;
    public ComputeBuffer AnchorEdgeBuffer;
    public ComputeBuffer AnchorSliceBuffer;
    public ComputeBuffer BucketBuffer;
    public ComputeBuffer SliceBuffer;
    public ComputeBuffer LineMeshBuffer;
    public ComputeBuffer HeadBuffer;
    public ComputeBuffer GroupBuffer;

    public LineRuntimeContext(Transform transform, LineMaterial material)
    {
        MetaData = null;
        RumtimeTransform = transform;
        LineMaterialSetting = material;
        ConstantsBuffer = null;

        VertexBuffer = null;
        FaceBuffer = null;
        MeshletBuffer = null;

        ArgLayout = null;
        ArgBuffer1 = null;
        ArgBuffer2 = null;
        ContourBuffer = null;
        VisibleSegmentBuffer = null;
        AllSegmentBuffer = null;
        AnchorVertexBuffer = null;
        AnchorEdgeBuffer = null;
        AnchorSliceBuffer = null;
        BucketBuffer = null;
        SliceBuffer = null;
        LineMeshBuffer = null;
        HeadBuffer = null;
        GroupBuffer = null;
    }

    public bool Init(PackedLineData metaData, bool EnableDebug)
    {
        if (metaData == null)
            return false;
        MetaData = metaData;
        ArgLayout = new ArgBufferLayout(EnableDebug);

        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();
        ConstantsBuffer = new ComputeBuffer(1, RenderConstants.Size(), ComputeBufferType.Constant);
        RenderConstants[] Constants = new RenderConstants[1];
        Constants[0].TotalVerticesNum = MetaData.VertexNum;
        Constants[0].TotalRepeatedVerticesNum = MetaData.RepeatedVertexNum;
        Constants[0].TotalFacesNum = MetaData.FaceNum;
        Constants[0].TotalMeshletsNum = MetaData.MeshletLayer0Num;
        Constants[0].ContourEnable = (uint)(LineMaterialSetting.ContourEnable ? 1 : 0);
        Constants[0].CreaseEnable = (uint)(LineMaterialSetting.CreaseEnable ? 1 : 0);
        Constants[0].BorderEnable = (uint)(LineMaterialSetting.BorderEnable ? 1 : 0);

        ConstantsBuffer.SetData(Constants);
        ConstantsBuffer.name = "ConstantsBuffer";

        // mesh data
        if (VertexBuffer != null)
            VertexBuffer.Release();
        VertexBuffer = new ComputeBuffer((int)MetaData.RepeatedVertexNum, (int)MetaData.VertexStructSize);
        VertexBuffer.SetData(MetaData.RepeatedVertexList);
        VertexBuffer.name = "VertexBuffer";

        if (FaceBuffer != null)
            FaceBuffer.Release();
        FaceBuffer = new ComputeBuffer((int)MetaData.FaceNum, (int)MetaData.FaceStructSize);
        FaceBuffer.SetData(MetaData.FaceList);
        FaceBuffer.name = "FaceBuffer";

        if (MeshletBuffer != null)
            MeshletBuffer.Release();
        MeshletBuffer = new ComputeBuffer((int)MetaData.MeshletLayer0Num, (int)MetaData.MeshletStructSize);
   
        MeshletBuffer.SetData(MetaData.MeshletList);
        MeshletBuffer.name = "MeshletBuffer";

        // runtime data
        if (ArgBuffer1 != null)
            ArgBuffer1.Release();
        ArgBuffer1 = new ComputeBuffer(ArgLayout.Count(), (int)ArgLayout.StrideSize(), ComputeBufferType.IndirectArguments);
        ArgBuffer1.SetData(ArgLayout.Buffer);
        ArgBuffer1.name = "ArgBuffer1";
        if (ArgBuffer2 != null)
            ArgBuffer2.Release();
        ArgBuffer2 = new ComputeBuffer(ArgLayout.Count(), (int)ArgLayout.StrideSize(), ComputeBufferType.IndirectArguments);
        ArgBuffer2.SetData(ArgLayout.Buffer);
        ArgBuffer2.name = "ArgBuffer2";

        /*
         * In practice, for general man-made triangular meshes,McGuire (2004) measured empirically a trend closer to pow(faceNum, 0.8)
         * M. McGuire. 《Observations on silhouette sizes》
         * 33554432^0.8 = 1024*1024 -> max 33554432 face per object
         */
        double ContourSize = Math.Pow(MetaData.FaceNum, 0.8);
        double SegmentSize = MetaData.EdgeNum + ContourSize;
        // It's not a accurate value, temp
        int SliceNumPredict = Screen.currentResolution.width * Screen.currentResolution.height / 128;

        if (ContourBuffer != null)
            ContourBuffer.Release();
        ContourBuffer = new ComputeBuffer((int)SegmentSize, Contour.Size());
        ContourBuffer.name = "ContourBuffer";

        if (VisibleSegmentBuffer != null)
            VisibleSegmentBuffer.Release();
        VisibleSegmentBuffer = new ComputeBuffer((int)SegmentSize, SegmentMetaData.Size(EnableDebug));
        VisibleSegmentBuffer.name = "VisibleSegmentBuffer";

        if (AllSegmentBuffer != null)
            AllSegmentBuffer.Release();
        AllSegmentBuffer = new ComputeBuffer((int)SliceNumPredict*32, Segment.Size(EnableDebug));
        AllSegmentBuffer.name = "AllSegmentBuffer";

        if (AnchorVertexBuffer != null)
            AnchorVertexBuffer.Release();
        AnchorVertexBuffer = new ComputeBuffer((int)MetaData.VertexNum, AnchorVertex.Size());
        AnchorVertexBuffer.name = "AnchorVertexBuffer";

        if (AnchorEdgeBuffer != null)
            AnchorEdgeBuffer.Release();
        AnchorEdgeBuffer = new ComputeBuffer((int)MetaData.EdgeNum, AnchorEdge.Size());
        AnchorEdgeBuffer.name = "AnchorEdgeBuffer";

        if (AnchorSliceBuffer != null)
            AnchorSliceBuffer.Release();
        AnchorSliceBuffer = new ComputeBuffer(SliceNumPredict, AnchorSlice.Size());
        AnchorSliceBuffer.name = "AnchorSliceBuffer";

        if (BucketBuffer != null)
            BucketBuffer.Release();
        BucketBuffer = new ComputeBuffer(1024, sizeof(uint));
        BucketBuffer.name = "BucketBuffer";

        if (SliceBuffer != null)
            SliceBuffer.Release();
        SliceBuffer = new ComputeBuffer(SliceNumPredict, Slice.Size());
        SliceBuffer.name = "SliceBuffer";

        if (LineMeshBuffer != null)
            LineMeshBuffer.Release();
        LineMeshBuffer = new ComputeBuffer((int)SliceNumPredict * 32, LineMesh.Size(EnableDebug));
        LineMeshBuffer.name = "LineMeshBuffer";

        if (HeadBuffer != null)
            HeadBuffer.Release();
        HeadBuffer = new ComputeBuffer((int)SliceNumPredict * 32, sizeof(uint)); // first element is total head count
        HeadBuffer.name = "HeadBuffer";

        if (GroupBuffer != null)
            GroupBuffer.Release();
        GroupBuffer = new ComputeBuffer((int)SliceNumPredict * 32, LineGroup.Size());
        GroupBuffer.name = "GroupBuffer";

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

        if (ArgBuffer2 != null)
            ArgBuffer2.Release();

        if (ContourBuffer != null)
            ContourBuffer.Release();

        if (VisibleSegmentBuffer != null)
            VisibleSegmentBuffer.Release();

        if (AllSegmentBuffer != null)
            AllSegmentBuffer.Release();

        if (AnchorVertexBuffer != null)
            AnchorVertexBuffer.Release();

        if (AnchorEdgeBuffer != null)
            AnchorEdgeBuffer.Release();

        if (AnchorSliceBuffer != null)
            AnchorSliceBuffer.Release();

        if (BucketBuffer != null)
            BucketBuffer.Release();

        if (SliceBuffer != null)
            SliceBuffer.Release();

        if (LineMeshBuffer != null)
            LineMeshBuffer.Release();

        if (HeadBuffer != null)
            HeadBuffer.Release();

        if (GroupBuffer != null)
            GroupBuffer.Release();


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
    private ComputePass ChainningPassLayer1;
    private ComputePass ChainningPassLayer2;
    private ComputePass ChainningPassLayer3;

    private CommandBuffer RenderCommands;

    private GlobalKeyword DebugKeyword;
    private RenderTargetIdentifier DepthTextureIdentifier;

    //debug
    public RenderTexture DepthTextureTemporary;
    public RenderTargetIdentifier DepthTextureTemporaryIdentifier;

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
        ChainningPassLayer1 = new ComputePass();
        ChainningPassLayer2 = new ComputePass();
        ChainningPassLayer3 = new ComputePass();

        DepthTextureTemporary = null;
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
        if (!ChainningPassLayer1.Init("ChainningPass", "ChainningLayer1", InputShaders.ChainningPassShader))
            return false;
        if (!ChainningPassLayer2.Init("ChainningPass", "ChainningLayer2", InputShaders.ChainningPassShader))
            return false;
        if (!ChainningPassLayer3.Init("ChainningPass", "ChainningLayer3", InputShaders.ChainningPassShader))
            return false;

        DebugKeyword = GlobalKeyword.Create("ENABLE_DEBUG_VIEW");
        DepthTextureIdentifier = new RenderTargetIdentifier(BuiltinRenderTextureType.Depth);

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
        ChainningPassLayer1.Destroy();
        ChainningPassLayer2.Destroy();
        ChainningPassLayer3.Destroy();

        if (DepthTextureTemporary != null)
            DepthTextureTemporary.Release();

        Available = false;
        Debug.Log("Render Layer Destroy");

    }

    public void PreRender(Camera MainCamera, GameObject PivotCameraObject, bool EnableDebug)
    {

        if (!EnableDebug && PivotCameraObject != null)
        {
            if ((DepthTextureTemporary == null)  ||  ((DepthTextureTemporary != null) && (DepthTextureTemporary.width != MainCamera.scaledPixelWidth || DepthTextureTemporary.height != MainCamera.scaledPixelHeight)))
            {
                if(DepthTextureTemporary != null)
                    DepthTextureTemporary.Release();
                DepthTextureTemporary = new RenderTexture(MainCamera.scaledPixelWidth, MainCamera.scaledPixelHeight, 24, RenderTextureFormat.Depth);
                DepthTextureTemporary.name = "DepthTextureTemporary";
                DepthTextureTemporaryIdentifier = new RenderTargetIdentifier(DepthTextureTemporary);
                //Debug.Log("+++++++++++++++++++ Create : " + DepthTextureTemporary.name);
            }
            //Debug.Log("+++++++++++++++++++" + DepthTextureTemporary.name);
            RenderCommands.CopyTexture(DepthTextureIdentifier, DepthTextureTemporaryIdentifier);
        }

        if(EnableDebug && PivotCameraObject != null)
        {
            LinesRenderer MainCameraRenderer = PivotCameraObject.GetComponent<LinesRenderer>();
            if (MainCameraRenderer != null)
            {
                DepthTextureTemporaryIdentifier = MainCameraRenderer.GetDepthRenderTexture();
                //Debug.Log("====================" + DepthTextureTemporaryIdentifier);
            }
        }
    }

    public void Render(LineRuntimeContext Current, bool EnableDebug)
    {
        if (!Available)
            return;

        RenderCommands.SetKeyword(DebugKeyword, EnableDebug);

        //Reset
        //Temp, maybe do this in better way
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "ArgBuffer1", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "ArgBuffer2", Current.ArgBuffer2);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "HeadBuffer", Current.HeadBuffer);
        RenderCommands.SetComputeBufferParam(ResetPass.CoreShader, ResetPass.CoreShaderKernelId, "GroupBuffer", Current.GroupBuffer);
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
        RenderCommands.SetComputeVectorParam(ExtractPass.CoreShader, "LocalSpaceViewForward", EveryFrameParams.LocalCameraForward);
        RenderCommands.SetComputeFloatParam(ExtractPass.CoreShader, "CreaseAngleThreshold", EveryFrameParams.CreaseAngleThreshold); 
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "VertexBuffer", Current.VertexBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "FaceBuffer", Current.FaceBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "MeshletBuffer", Current.MeshletBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "ContourBuffer", Current.ContourBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "AnchorVertexBuffer", Current.AnchorVertexBuffer);
        RenderCommands.SetComputeBufferParam(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, "AnchorEdgeBuffer", Current.AnchorEdgeBuffer);
        RenderCommands.DispatchCompute(ExtractPass.CoreShader, ExtractPass.CoreShaderKernelId, (int)Current.MetaData.MeshletLayer0Num, 1, 1);


        /*
         * Contourize Pass
         * 
         */
        RenderCommands.SetComputeConstantBufferParam(ContourizePass.CoreShader, Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        RenderCommands.SetComputeVectorParam(ContourizePass.CoreShader, "LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        RenderCommands.SetComputeVectorParam(ContourizePass.CoreShader, "ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
        RenderCommands.SetComputeMatrixParam(ContourizePass.CoreShader, "WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrixForRenderCamera);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "VertexBuffer", Current.VertexBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "ContourBuffer", Current.ContourBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer2);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "AllSegmentBuffer", Current.AllSegmentBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "VisibleSegmentBuffer", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "AnchorVertexBuffer", Current.AnchorVertexBuffer);
        RenderCommands.SetComputeBufferParam(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, "AnchorEdgeBuffer", Current.AnchorEdgeBuffer);
        RenderCommands.DispatchCompute(ContourizePass.CoreShader, ContourizePass.CoreShaderKernelId, Current.ArgBuffer1, (uint)Current.ArgLayout.ContourizePassDispatchCount());


        /*
        * Slice Pass
        * Limited:
        * Max 1024 * 1024 -> 1,048,576 extracted edges
        */
        RenderCommands.SetComputeBufferParam(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, "InputArray", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer2);
        RenderCommands.DispatchCompute(SlicePassPart1.CoreShader, SlicePassPart1.CoreShaderKernelId, Current.ArgBuffer2, (uint)Current.ArgLayout.SlicePassDispatchCount());
            
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "InputArray", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer2);
        RenderCommands.SetComputeBufferParam(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, "BucketArray", Current.BucketBuffer);
        RenderCommands.DispatchCompute(SlicePassPart2.CoreShader, SlicePassPart2.CoreShaderKernelId, 1, 1, 1); //Max 1024 * 1024 segments

        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "InputArray", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "BucketArray", Current.BucketBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer2);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "SliceBuffer", Current.SliceBuffer);
        RenderCommands.SetComputeBufferParam(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, "AnchorSliceBuffer", Current.AnchorSliceBuffer);
        RenderCommands.DispatchCompute(SlicePassPart3.CoreShader, SlicePassPart3.CoreShaderKernelId, Current.ArgBuffer2, (uint)Current.ArgLayout.SlicePassDispatchCount());

        /*
        *  Visibility Pass
        *  Limited:
        *  Direct3D 11 DirectCompute 5.0:
        *  firstbithigh/firstbitlow, interlockedAnd, interlockedOr
        */
        RenderTargetIdentifier DepthRTIdentifier;
        if (!EnableDebug)
            DepthRTIdentifier = DepthTextureIdentifier;
        else
            DepthRTIdentifier = DepthTextureTemporaryIdentifier;
        RenderCommands.SetComputeIntParam(VisiblePass.CoreShader, "VisibilityFlag", (int)EveryFrameParams.VisibilityFlag);
        RenderCommands.SetComputeVectorParam(VisiblePass.CoreShader, "ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
        RenderCommands.SetComputeTextureParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "SceneDepthTexture", DepthRTIdentifier);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "VisibleSegmentBuffer", Current.VisibleSegmentBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "SliceBuffer", Current.SliceBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer2);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "AllSegmentBuffer", Current.AllSegmentBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "AnchorVertexBuffer", Current.AnchorVertexBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "AnchorEdgeBuffer", Current.AnchorEdgeBuffer);
        RenderCommands.SetComputeBufferParam(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, "AnchorSliceBuffer", Current.AnchorSliceBuffer);
        RenderCommands.DispatchCompute(VisiblePass.CoreShader, VisiblePass.CoreShaderKernelId, Current.ArgBuffer1, (uint)Current.ArgLayout.VisiblityPassDispatchCount());


        /*
        *  Generate Pass
        */
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer2);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer1);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "SliceBuffer", Current.SliceBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "AllSegmentBuffer", Current.AllSegmentBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "AnchorVertexBuffer", Current.AnchorVertexBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "AnchorEdgeBuffer", Current.AnchorEdgeBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "AnchorSliceBuffer", Current.AnchorSliceBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "LineMeshBuffer", Current.LineMeshBuffer);
        RenderCommands.SetComputeBufferParam(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, "HeadBuffer", Current.HeadBuffer);
        RenderCommands.SetComputeFloatParam(GeneratePass.CoreShader, "ChainningAngleThreshold", EveryFrameParams.ChainningAngleThreshold);
        RenderCommands.DispatchCompute(GeneratePass.CoreShader, GeneratePass.CoreShaderKernelId, Current.ArgBuffer2, (uint)Current.ArgLayout.GeneratePassDispatchCount());


        if (!EnableDebug)
        {
            /*
            *  Chainning Pass 1
            */
            RenderCommands.SetComputeBufferParam(ChainningPassLayer1.CoreShader, ChainningPassLayer1.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer1);
            RenderCommands.SetComputeBufferParam(ChainningPassLayer1.CoreShader, ChainningPassLayer1.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer2);
            RenderCommands.SetComputeBufferParam(ChainningPassLayer1.CoreShader, ChainningPassLayer1.CoreShaderKernelId, "HeadBuffer", Current.HeadBuffer);
            RenderCommands.SetComputeBufferParam(ChainningPassLayer1.CoreShader, ChainningPassLayer1.CoreShaderKernelId, "LineMeshBuffer", Current.LineMeshBuffer);
            RenderCommands.SetComputeBufferParam(ChainningPassLayer1.CoreShader, ChainningPassLayer1.CoreShaderKernelId, "GroupBuffer", Current.GroupBuffer);
            RenderCommands.DispatchCompute(ChainningPassLayer1.CoreShader, ChainningPassLayer1.CoreShaderKernelId, Current.ArgBuffer1, (uint)Current.ArgLayout.ChainningPass1DispatchCount());

            if (Current.MetaData.MeshletLayer1Num > 1)
            {
                /*
                *  Chainning Pass 2
                */
                RenderCommands.SetComputeBufferParam(ChainningPassLayer2.CoreShader, ChainningPassLayer2.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer2);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer2.CoreShader, ChainningPassLayer2.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer1);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer2.CoreShader, ChainningPassLayer2.CoreShaderKernelId, "HeadBuffer", Current.HeadBuffer);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer2.CoreShader, ChainningPassLayer2.CoreShaderKernelId, "LineMeshBuffer", Current.LineMeshBuffer);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer2.CoreShader, ChainningPassLayer2.CoreShaderKernelId, "GroupBuffer", Current.GroupBuffer);
                RenderCommands.DispatchCompute(ChainningPassLayer2.CoreShader, ChainningPassLayer2.CoreShaderKernelId, Current.ArgBuffer2, (uint)Current.ArgLayout.ChainningPass2DispatchCount());
            }

            if (Current.MetaData.MeshletLayer2Num > 1)
            {
                /*
                *  Chainning Pass 3
                */
                RenderCommands.SetComputeBufferParam(ChainningPassLayer3.CoreShader, ChainningPassLayer3.CoreShaderKernelId, "RArgBuffer", Current.ArgBuffer1);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer3.CoreShader, ChainningPassLayer3.CoreShaderKernelId, "WArgBuffer", Current.ArgBuffer2);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer3.CoreShader, ChainningPassLayer3.CoreShaderKernelId, "HeadBuffer", Current.HeadBuffer);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer3.CoreShader, ChainningPassLayer3.CoreShaderKernelId, "LineMeshBuffer", Current.LineMeshBuffer);
                RenderCommands.SetComputeBufferParam(ChainningPassLayer3.CoreShader, ChainningPassLayer3.CoreShaderKernelId, "GroupBuffer", Current.GroupBuffer);
                RenderCommands.DispatchCompute(ChainningPassLayer3.CoreShader, ChainningPassLayer3.CoreShaderKernelId, Current.ArgBuffer1, (uint)Current.ArgLayout.ChainningPass3DispatchCount());
            }
        }

        /*
         * Draw Mesh
         */
        Shader.SetKeyword(DebugKeyword, EnableDebug);
        if (EnableDebug)
        {
            Current.LineMaterialSetting.LineRenderMaterialForDebug.SetBuffer("LineMeshBuffer", Current.LineMeshBuffer);
            Current.LineMaterialSetting.LineRenderMaterialForDebug.SetMatrix("WorldViewProjectionForDebugCamera", EveryFrameParams.WorldViewProjectionMatrixForDebugCamera);
            Current.LineMaterialSetting.LineRenderMaterialForDebug.SetMatrix("InvWorldViewProjectionForRenderCamera", EveryFrameParams.InvWorldViewProjectionMatrixForRenderCamera);
            RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterialForDebug, -1, MeshTopology.Lines, Current.ArgBuffer2, (int)Current.ArgLayout.IndirectDrawStart());
        }
        else {
            Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("LineMeshBuffer", Current.LineMeshBuffer);
            Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("GroupBuffer", Current.GroupBuffer);
            Current.LineMaterialSetting.LineRenderMaterial.SetVector("ScreenScaledResolution", EveryFrameParams.ScreenScaledResolution);
            Current.LineMaterialSetting.LineRenderMaterial.SetFloat("LineWidthScale", EveryFrameParams.LineWidthScale);
            RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterial, -1, MeshTopology.Triangles, Current.ArgBuffer2, (int)Current.ArgLayout.IndirectDrawStart());
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

