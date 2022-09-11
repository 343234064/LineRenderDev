

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

public struct LineTransformed
{
    public Vector3 LocalPosition1;
    public Vector3 LocalPosition2;
    public Vector4 NDCPositon1;
    public Vector4 NDCPositon2;

    public static int Size()
    {
        return sizeof(float) * 4 * 2 + sizeof(float) * 3 * 2;
    }
}

public struct LineSegment
{
    public Vector3 point2d1;
    public Vector3 point2d2;

    public static int Size()
    {
        return sizeof(float) * 3 * 2;
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

    public ComputeBuffer ExtractedLineBuffer;
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

        ExtractedLineBuffer = null;
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


        if (ExtractedLineBuffer != null)
            ExtractedLineBuffer.Release();
        ExtractedLineBuffer = new ComputeBuffer(AdjTris.Length * 3, LineTransformed.Size(), ComputeBufferType.Append);
        if (ExtractedLineArgBuffer != null)
            ExtractedLineArgBuffer.Release();
        ExtractedLineArgBuffer = new ComputeBuffer(3, sizeof(int), ComputeBufferType.IndirectArguments);
        int[] ExtractedLineArg = new int[3] { 0, 1, 1 }; // instance count, 1, 1(for next dispatch x y z)
        ExtractedLineArgBuffer.SetData(ExtractedLineArg);
        ExtractedLineArgBufferOffset = 0;

        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        VisibleLineBuffer = new ComputeBuffer(AdjTris.Length * 3, LineSegment.Size(), ComputeBufferType.Append);
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
        if (AdjacencyBuffer != null)
            AdjacencyBuffer.Release();
        if (VerticesBuffer != null)
            VerticesBuffer.Release();
        if (ConstantsBuffer != null)
            ConstantsBuffer.Release();
        if (ExtractedLineBuffer != null)
            ExtractedLineBuffer.Release();
        if (ExtractedLineArgBuffer != null)
            ExtractedLineArgBuffer.Release();
        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
    }

}


public class ComputePass
{
    public class BufferParam
    {
        public BufferParam(string ParamName, ComputeBuffer InBuffer, int Size)
        {
            PropertyName = ParamName;
            Buffer = InBuffer;
            BufferByteSize = Size;

            ArgBuffer = null;
            ArgBufferOffset = 0;
            NeedCopyValue = false;
        }

        public BufferParam(string ParamName, ComputeBuffer InBuffer, int Size, ComputeBuffer InArgBuffer, int ArgBufferCopyValueOffset)
        {
            PropertyName = ParamName;
            Buffer = InBuffer;
            BufferByteSize = Size;

            ArgBuffer = InArgBuffer;
            ArgBufferOffset = ArgBufferCopyValueOffset;
            NeedCopyValue = true;
        }

        public string PropertyName;
        public ComputeBuffer Buffer;
        public int BufferByteSize;

        public ComputeBuffer ArgBuffer;
        public int ArgBufferOffset;
        public bool NeedCopyValue;
    }

    public string Name;
    protected bool Available;

    public Array3<uint> CoreShaderGroupSize;
    protected int CoreShaderKernelId;
    protected ComputeShader CoreShader;

    protected List<BufferParam> InputBuffers;
    protected List<BufferParam> OutputBuffers;
    protected List<BufferParam> ConstantsBuffers;

    public ComputePass()
    {
        Available = false;
        CoreShaderGroupSize = new Array3<uint>(0,0,0);
        InputBuffers = new List<BufferParam>();
        OutputBuffers = new List<BufferParam>();
        ConstantsBuffers = new List<BufferParam>();
    }

    public virtual bool Init(string PassName, ComputeShader InputShader) {
        if (InputShader == null)
            return false;

        Name = PassName;

        CoreShader = InputShader;
        CoreShaderKernelId = CoreShader.FindKernel("CSMain");
        CoreShader.GetKernelThreadGroupSizes(CoreShaderKernelId, out CoreShaderGroupSize.x, out CoreShaderGroupSize.y, out CoreShaderGroupSize.z);

        Available = true;
        return true;
    }

    public virtual void Destroy() 
    {
        ClearConstantBuffer();
        ClearInputBuffer();
        ClearOutputBuffer();

        Available = false;
    }

    public virtual void InternalRender(CommandBuffer RenderCommands, Array3<int> DispatchGroupSize) 
    {
        foreach(BufferParam CBuffer in ConstantsBuffers)
        {
            RenderCommands.SetComputeConstantBufferParam(CoreShader, Shader.PropertyToID(CBuffer.PropertyName), CBuffer.Buffer, 0, CBuffer.BufferByteSize);
        }

        foreach(BufferParam IBuffer in InputBuffers)
        {
            RenderCommands.SetComputeBufferParam(CoreShader, CoreShaderKernelId, IBuffer.PropertyName, IBuffer.Buffer);
        }

        foreach (BufferParam OBuffer in OutputBuffers)
        {
            RenderCommands.SetComputeBufferParam(CoreShader, CoreShaderKernelId, OBuffer.PropertyName, OBuffer.Buffer);
            RenderCommands.SetBufferCounterValue(OBuffer.Buffer, 0);
        }

        RenderCommands.DispatchCompute(CoreShader, CoreShaderKernelId, DispatchGroupSize.x, DispatchGroupSize.y, DispatchGroupSize.z);

        foreach (BufferParam OBuffer in OutputBuffers)
        {
            if(OBuffer.NeedCopyValue)
                RenderCommands.CopyCounterValue(OBuffer.Buffer, OBuffer.ArgBuffer, (uint)OBuffer.ArgBufferOffset);
        }
       
    }

    public virtual void InternalRenderIndirect(CommandBuffer RenderCommands, ComputeBuffer DispatchArgBuffer, uint DispatchArgBufferOffset)
    {
        foreach (BufferParam CBuffer in ConstantsBuffers)
        {
            RenderCommands.SetComputeConstantBufferParam(CoreShader, Shader.PropertyToID(CBuffer.PropertyName), CBuffer.Buffer, 0, CBuffer.BufferByteSize);
        }

        foreach (BufferParam IBuffer in InputBuffers)
        {
            RenderCommands.SetComputeBufferParam(CoreShader, CoreShaderKernelId, IBuffer.PropertyName, IBuffer.Buffer);
        }

        foreach (BufferParam OBuffer in OutputBuffers)
        {
            RenderCommands.SetComputeBufferParam(CoreShader, CoreShaderKernelId, OBuffer.PropertyName, OBuffer.Buffer);
            RenderCommands.SetBufferCounterValue(OBuffer.Buffer, 0);
        }

        RenderCommands.DispatchCompute(CoreShader, CoreShaderKernelId, DispatchArgBuffer, DispatchArgBufferOffset);


        foreach (BufferParam OBuffer in OutputBuffers)
        {
            if (OBuffer.NeedCopyValue)
                RenderCommands.CopyCounterValue(OBuffer.Buffer, OBuffer.ArgBuffer, (uint)OBuffer.ArgBufferOffset);

        }

    }

    public virtual void SetConstantBuffer(string Name, ComputeBuffer Buffer, int BufferSize)
    {
        ConstantsBuffers.Add(new BufferParam(Name, Buffer, BufferSize));
    }

    public virtual void SetInputBuffer(string Name, ComputeBuffer Buffer)
    {
        InputBuffers.Add(new BufferParam(Name, Buffer, 0));
    }

    public virtual void SetOutputBuffer(string Name, ComputeBuffer Buffer) 
    {
        OutputBuffers.Add(new BufferParam(Name, Buffer, 0));
    }

    public virtual void SetOutputBufferWithArgBuffer(string Name, ComputeBuffer Buffer, ComputeBuffer ArgBuffer, int ArgBufferCopyValueOffset)
    {
        OutputBuffers.Add(new BufferParam(Name, Buffer, 0, ArgBuffer, ArgBufferCopyValueOffset));
    }

    public virtual void ClearConstantBuffer()
    {
        ConstantsBuffers.Clear();
    }

    public virtual void ClearInputBuffer()
    {
        InputBuffers.Clear();
    }

    public virtual void ClearOutputBuffer()
    {
        OutputBuffers.Clear();
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

    public VisibilityPass()
    {
        DispatchArguments = null;
        DispatchArgOffset = 0;
    }

    public void Render(CommandBuffer RenderCommands, RenderParams EveryFrameParams)
    {
        RenderTargetIdentifier DepthRTIdentifier = new RenderTargetIdentifier(BuiltinRenderTextureType.Depth);
        RenderCommands.SetComputeTextureParam(CoreShader, CoreShaderKernelId, "SceneDepthTexture", DepthRTIdentifier);

        base.InternalRenderIndirect(RenderCommands, DispatchArguments, DispatchArgOffset);
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
        ExtractPass.SetOutputBufferWithArgBuffer("Output3DLines", Current.ExtractedLineBuffer, Current.ExtractedLineArgBuffer, Current.ExtractedLineArgBufferOffset);
        ExtractPass.DispatchSize.x = ((int)(Current.AdjacencyBuffer.count / ExtractPass.CoreShaderGroupSize.x) + 1);
        ExtractPass.DispatchSize.y = 1;
        ExtractPass.DispatchSize.z = 1;

        VisiblePass.SetConstantBuffer("Constants", Current.ConstantsBuffer, RenderConstants.Size());
        VisiblePass.SetInputBuffer("Input3DLines", Current.ExtractedLineBuffer);
        VisiblePass.SetOutputBufferWithArgBuffer("Output2DLines", Current.VisibleLineBuffer, Current.VisibleLineArgBuffer, Current.VisibleLineArgBufferOffset);
        VisiblePass.DispatchArguments = Current.ExtractedLineArgBuffer;
        VisiblePass.DispatchArgOffset = (uint)Current.ExtractedLineArgBufferOffset;

        Current.LineMaterialSetting.LineRenderMaterial.SetMatrix("ObjectWorldMatrix", EveryFrameParams.ObjectWorldMatrix);
        Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("Lines", Current.VisibleLineBuffer);

        ExtractPass.Render(RenderCommands, EveryFrameParams);
        VisiblePass.Render(RenderCommands, EveryFrameParams);

        RenderCommands.DrawProceduralIndirect(Matrix4x4.identity, Current.LineMaterialSetting.LineRenderMaterial, -1, MeshTopology.Lines, Current.VisibleLineArgBuffer, 0); //VisibleLineArgBufferOffset

        /*
        ExtractPass.CoreShader.SetConstantBuffer(Shader.PropertyToID("Constants"), Current.ConstantsBuffer, 0, RenderConstants.Size());
        ExtractPass.CoreShader.SetBuffer(ExtractPass.CoreShaderKernelId, "AdjacencyTriangles", Current.AdjacencyBuffer);
        ExtractPass.CoreShader.SetBuffer(ExtractPass.CoreShaderKernelId, "Vertices", Current.VerticesBuffer);
        ExtractPass.CoreShader.SetBuffer(ExtractPass.CoreShaderKernelId, "Output3DLines", Current.ExtractedLineBuffer);

        ExtractPass.CoreShader.SetVector("LocalSpaceViewPosition", EveryFrameParams.LocalCameraPosition);
        ExtractPass.CoreShader.SetFloat("CreaseAngleThreshold", EveryFrameParams.CreaseAngleThreshold);
        ExtractPass.CoreShader.SetMatrix("WorldViewProjection", EveryFrameParams.WorldViewProjectionMatrix);
        ExtractPass.CoreShader.SetMatrix("WorldView", EveryFrameParams.WorldViewMatrix);

        Current.ExtractedLineBuffer.SetCounterValue(0);
        ExtractPass.CoreShader.Dispatch(ExtractPass.CoreShaderKernelId, ((int)(Current.AdjacencyBuffer.count / ExtractPass.CoreShaderGroupSize.x) + 1), 1, 1);
        //Debug.Log("Group Size : " + ExtractLinePassGroupSize);
        ComputeBuffer.CopyCount(Current.ExtractedLineBuffer, Current.VisibleLineArgBuffer, Current.VisibleLineArgBufferOffset);

        Current.LineMaterialSetting.LineRenderMaterial.SetMatrix("ObjectWorldMatrix", EveryFrameParams.ObjectWorldMatrix);
        Current.LineMaterialSetting.LineRenderMaterial.SetBuffer("Lines", Current.ExtractedLineBuffer);
        */
        /*
        Debug.Log("==============================");
        Debug.Log("==============================");
        int[] Args1 = new int[3] { 0, 0, 0 };
        Current.ExtractedLineArgBuffer.GetData(Args1);
        Debug.Log("Instance Count 1 " + Args1[0]);
        //Debug.Log("y Count 1 " + Args[1]);
        //Debug.Log("z Count 1 " + Args[2]);
        Debug.Log("==============================");
        
        int[] Args2 = new int[4] { 0, 0, 0, 0 };
        Current.VisibleLineArgBuffer.GetData(Args2);
        Debug.Log("Vertex Count 2 " + Args2[0]);
        Debug.Log("Instance Count 2 " + Args2[1]);
        Debug.Log("Start Vertex 2 " + Args2[2]);
        Debug.Log("Start Instance 2 " + Args2[3]);
        Debug.Log("==============================");
        Debug.Log("==============================");
         */
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

