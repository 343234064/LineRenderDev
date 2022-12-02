

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


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


public struct Bucket
{
    public int CapacityInPixel;
    public int TargetLineIndex;

    public float StartTPosition;
    public float EndTPosition;
}

public struct Stripe
{
    public int LineIndex;

    public float LengthInPixel;
    public float LengthInPixelTotalStripe;

    public static int Size()
    {
        return sizeof(int) + sizeof(float) * 2;
    }
}

public struct LineSegment
{
    public Vector3 LocalPosition1;
    public Vector3 LocalPosition2;
    public Vector4 NDCPositon1;
    public Vector4 NDCPositon2;

    public int Extracted;
    public int Visible;

    public static int Size()
    {
        return sizeof(float) * 4 * 2 + sizeof(float) * 3 * 2 + sizeof(int) * 2 ;
    }
}



public class LineShader
{
    public ComputeShader ExtractPassShader;
    public ComputeShader VisibilityPassShader;

    public bool IsValid()
    {
        if (ExtractPassShader == null)
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

    public ComputeBuffer AdjacencyBuffer;
    public ComputeBuffer VerticesBuffer;

    public ComputeBuffer ConstantsBuffer;
    public ComputeBuffer SegmentBuffer;

    public ComputeBuffer ExtractedLineArgBuffer;
    public int ExtractedLineArgBufferOffset;

    public ComputeBuffer VisibleLineBuffer;
    public ComputeBuffer VisibleLineArgBuffer;
    public int VisibleLineArgBufferOffset;

    public LineContext(Mesh meshObj, Transform transform, LineMaterial material)
    {
        RumtimeMesh = meshObj;
        RumtimeTransform = transform;
        LineMaterialSetting = material;

        AdjacencyBuffer = null;
        VerticesBuffer = null;

        ConstantsBuffer = null;
        SegmentBuffer = null;

        ExtractedLineArgBuffer = null;
        VisibleLineBuffer = null;
        VisibleLineArgBuffer = null;
        ExtractedLineArgBufferOffset = 0;
        VisibleLineArgBufferOffset = 0;
    }

    public bool Init(AdjFace[] AdjTris)
    {
        if (AdjTris == null)
            return false;

        if (AdjacencyBuffer != null)
            AdjacencyBuffer.Release();
        AdjacencyBuffer = new ComputeBuffer(AdjTris.Length, AdjFace.Size());
        AdjacencyBuffer.SetData(AdjTris);

        if (VerticesBuffer != null)
            VerticesBuffer.Release();
        VerticesBuffer = new ComputeBuffer(RumtimeMesh.vertices.Length, sizeof(float) * 3);
        VerticesBuffer.SetData(RumtimeMesh.vertices);

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

        if (SegmentBuffer != null)
            SegmentBuffer.Release();
        SegmentBuffer = new ComputeBuffer(AdjTris.Length * 3, LineSegment.Size());
        

        if (ExtractedLineArgBuffer != null)
            ExtractedLineArgBuffer.Release();
        ExtractedLineArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        //int[] ExtractedLineArg = new int[3] { 0, 1, 1 }; // instance count, 1, 1(for next dispatch x y z)
        int[] ExtractedLineArg = new int[4] { 2, AdjTris.Length * 3, 0, 0 }; // vertex count, instance count, start vertex, start instance
        ExtractedLineArgBuffer.SetData(ExtractedLineArg);
        //ExtractedLineArgBufferOffset = 0;
        ExtractedLineArgBufferOffset = sizeof(int);

        /*
        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        VisibleLineBuffer = new ComputeBuffer(AdjTris.Length * 3, LineSegment.Size(), ComputeBufferType.Append);
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
        VisibleLineArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        int[] VisibleLineArg = new int[4] { 2, 1, 0, 0 }; // vertex count, instance count, start vertex, start instance
        VisibleLineArgBuffer.SetData(VisibleLineArg);
        VisibleLineArgBufferOffset = sizeof(int);
        */

        return true;
    }

    public void Destroy()
    {
        if (AdjacencyBuffer != null)
            AdjacencyBuffer.Release();
        if (VerticesBuffer != null)
            VerticesBuffer.Release();

        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();
        if (SegmentBuffer != null)
            SegmentBuffer.Release();

        if (ExtractedLineArgBuffer != null)
            ExtractedLineArgBuffer.Release();
        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
    }

}




public class ExtractionPass : ComputePass
{
    public Array3<int> DispatchSize;

    public ExtractionPass()
    {
        DispatchSize = new Array3<int>(1, 1, 1);
    }

    public void Render(CommandBuffer RenderCommands, RenderParams EveryFrameParams)
    {
        RenderCommands.SetComputeVectorParam(CoreShader, "LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        RenderCommands.SetComputeFloatParam(CoreShader, "CreaseAngleThreshold", EveryFrameParams.CreaseAngleThreshold);
        RenderCommands.SetComputeMatrixParam(CoreShader, "WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrix);
        RenderCommands.SetComputeMatrixParam(CoreShader, "WorldView", EveryFrameParams.WorldViewMatrix);

        base.InternalRender(RenderCommands, DispatchSize);
    }
}


public class VisibilityPass : ComputePass
{
    public ComputeBuffer DispatchArguments;
    public uint DispatchArgOffset;
    //temp
    public Array3<int> DispatchSize;

    public VisibilityPass()
    {
        DispatchArguments = null;
        DispatchArgOffset = 0;

        //temp
        DispatchSize = new Array3<int>(1, 1, 1);
    }

    public void Render(CommandBuffer RenderCommands, RenderParams EveryFrameParams)
    {
        RenderTargetIdentifier DepthRTIdentifier = new RenderTargetIdentifier(BuiltinRenderTextureType.Depth);
        RenderCommands.SetComputeTextureParam(CoreShader, CoreShaderKernelId, "SceneDepthTexture", DepthRTIdentifier);

        //base.InternalRenderIndirect(RenderCommands, DispatchArguments, DispatchArgOffset);
        base.InternalRender(RenderCommands, DispatchSize);
    }
}


public class RenderLayer
{
    public RenderParams EveryFrameParams;

    private bool Available;
    private ExtractionPass ExtractPass;
    private VisibilityPass VisiblePass;

    private CommandBuffer RenderCommands;

    public RenderLayer()
    {
        Available = false;
        RenderCommands = new CommandBuffer();
        EveryFrameParams = new RenderParams();

        ExtractPass = new ExtractionPass();
        VisiblePass = new VisibilityPass();
    }

    public bool Init(LineShader InputShaders)
    {
        Available = false;
        if (!InputShaders.IsValid())
            return false;

        RenderCommands.name = "Draw Lines";

        if (!ExtractPass.Init("ExtractionPass", InputShaders.ExtractPassShader))
            return false;
        if (!VisiblePass.Init("VisibilityPass", InputShaders.VisibilityPassShader))
            return false;

        Available = true;
        Debug.Log("Render Layer Init");

        return true;
    }

    public void Destroy()
    {
        RenderCommands.Release();

        ExtractPass.Destroy();
        VisiblePass.Destroy();

        Available = false;
        Debug.Log("Render Layer Destroy");
    }

    public void Render(LineContext Current)
    {
        if (!Available)
            return;

        ExtractPass.SetConstantBuffer("Constants", Current.ConstantsBuffer, RenderConstants.Size());
        ExtractPass.SetInputBuffer("AdjacencyTriangles", Current.AdjacencyBuffer);
        ExtractPass.SetInputBuffer("Vertices", Current.VerticesBuffer);
        ExtractPass.SetInputBuffer("Segments", Current.SegmentBuffer);
        ExtractPass.DispatchSize.x = ((int)(Current.AdjacencyBuffer.count / ExtractPass.CoreShaderGroupSize.x) + 1); //Max 65535 * 65535 * 65535 * 1024  ->16,776,960 x 2 tris?
        ExtractPass.DispatchSize.y = 1;
        ExtractPass.DispatchSize.z = 1;
 
        VisiblePass.SetConstantBuffer("Constants", Current.ConstantsBuffer, RenderConstants.Size());
        VisiblePass.SetInputBuffer("Segments", Current.SegmentBuffer);
        //VisiblePass.SetOutputBufferWithArgBuffer("Output2DLines", Current.VisibleLineBuffer, Current.VisibleLineArgBuffer, Current.VisibleLineArgBufferOffset);
        //VisiblePass.DispatchArguments = Current.ExtractedLineArgBuffer;
        //VisiblePass.DispatchArgOffset = (uint)Current.ExtractedLineArgBufferOffset;
        VisiblePass.DispatchSize.x = (int)(Current.SegmentBuffer.count);
        VisiblePass.DispatchSize.y = 1;
        VisiblePass.DispatchSize.z = 1;

        Current.LineMaterialSetting.LineRenderMaterial.SetMatrix("ObjectWorldMatrix", EveryFrameParams.ObjectWorldMatrix);
        Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("Lines", Current.SegmentBuffer);

        ExtractPass.Render(RenderCommands, EveryFrameParams);
        VisiblePass.Render(RenderCommands, EveryFrameParams);

        RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterial, -1, MeshTopology.Lines, Current.ExtractedLineArgBuffer, 0); //VisibleLineArgBufferOffset

 
    }

    public CommandBuffer GetRenderCommand()
    {
        return RenderCommands;
    }

    public void ClearFrame()
    {
        
        ExtractPass.ClearConstantBuffer();
        ExtractPass.ClearInputBuffer();
        ExtractPass.ClearOutputBuffer();

        VisiblePass.ClearConstantBuffer();
        VisiblePass.ClearInputBuffer();
        VisiblePass.ClearOutputBuffer();
    }

    public void ClearCommandBuffer()
    {
        RenderCommands.Clear();
    }

}

