using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;



public struct uint3
{
    public uint3(uint _x = 0, uint _y = 0, uint _z = 0)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    public uint x;
    public uint y;
    public uint z;
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

public struct LineConstants
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

    private ComputeBuffer AdjacencyBuffer;
    private ComputeBuffer VerticesBuffer;
    private ComputeBuffer ConstantsBuffer;

    private ComputeBuffer ExtractedLineBuffer;
    private ComputeBuffer ExtractedLineArgBuffer;
    private ComputeBuffer VisibleLineBuffer;
    private ComputeBuffer VisibleLineArgBuffer;

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
        ConstantsBuffer = new ComputeBuffer(1, LineConstants.Size(), ComputeBufferType.Constant);

        if (ExtractedLineBuffer != null)
            ExtractedLineBuffer.Release();
        ExtractedLineBuffer = new ComputeBuffer(AdjTris.Length * 3, LineTransformed.Size(), ComputeBufferType.Append);
        if (ExtractedLineArgBuffer != null)
            ExtractedLineArgBuffer.Release();
        ExtractedLineArgBuffer = new ComputeBuffer(3, sizeof(int), ComputeBufferType.IndirectArguments);
        int[] ExtractedLineArg = new int[3] { 0, 1, 1 }; // instance count, 1, 1(for next dispatch x y z)
        ExtractedLineArgBuffer.SetData(ExtractedLineArg);

        if (VisibleLineBuffer != null)
            VisibleLineBuffer.Release();
        VisibleLineBuffer = new ComputeBuffer(AdjTris.Length * 3, LineSegment.Size(), ComputeBufferType.Append);
        if (VisibleLineArgBuffer != null)
            VisibleLineArgBuffer.Release();
        VisibleLineArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        int[] VisibleLineArg = new int[4] { 2, 1, 0, 0 }; // vertex count, instance count, start vertex, start instance
        VisibleLineArgBuffer.SetData(VisibleLineArg);

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
        public BufferParam(string Name, ComputeBuffer CSBuffer, int Size)
        {
            PropertyName = Name;
            Buffer = CSBuffer;
            BufferSize = Size;
        }

        public string PropertyName;
        public ComputeBuffer Buffer;
        public int BufferSize;
    }

    private string Name;
    private bool Available;

    private int CoreShaderKernelId;
    private uint3 CoreShaderGroupSize;
    private uint3 DispatchGroupSize;
    private ComputeShader CoreShader;

    private List<BufferParam> InputBuffers;
    private List<BufferParam> OutputBuffers;
    private List<BufferParam> ConstantsBuffers;

    public ComputePass()
    {
        Available = false;
        CoreShaderGroupSize = new uint3();
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
        InputBuffers.Clear();
        OutputBuffers.Clear();
        ConstantsBuffers.Clear();

        Available = false;
    }

    public virtual void InternalRender(CommandBuffer RenderCommands, uint3 DispatchGroupSize) 
    {
        foreach(BufferParam CBuffer in ConstantsBuffers)
        {
            RenderCommands.SetComputeConstantBufferParam(CoreShader, Shader.PropertyToID(CBuffer.PropertyName), CBuffer.Buffer, 0, CBuffer.BufferSize);
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
            RenderCommands.CopyCounterValue(OBuffer.Buffer, OBuffer.ArgBuffer, OBuffer.ArgBufferOffset);
        }
       
    }

    public virtual void InternalRenderIndirect(CommandBuffer RenderCommands, ComputeBuffer DispatchArgBuffer, uint DispatchArgBufferOffset)
    {
        foreach (BufferParam CBuffer in ConstantsBuffers)
        {
            RenderCommands.SetComputeConstantBufferParam(CoreShader, Shader.PropertyToID(CBuffer.PropertyName), CBuffer.Buffer, 0, CBuffer.BufferSize);
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
            RenderCommands.CopyCounterValue(OBuffer.Buffer, OBuffer.ArgBuffer, OBuffer.ArgBufferOffset);
        }

    }

    public virtual void SetConstantBuffer(string Name, ComputeBuffer Buffer, int BufferSize)
    {
        ConstantsBuffers.Add(new BufferParam(Name, Buffer, BufferSize));
    }

    public virtual void SetInputBuffer(string Name, ComputeBuffer Buffer, int BufferSize)
    {
        InputBuffers.Add(new BufferParam(Name, Buffer, BufferSize));
    }

    public virtual void SetOutputBuffer(string Name, ComputeBuffer Buffer, int BufferSize) 
    {
        OutputBuffers.Add(new BufferParam(Name, Buffer, BufferSize));
    }
}

public class ExtractionPass : ComputePass
{
    public void Render(CommandBuffer RenderCommands)
    {

        base.InternalRender(RenderCommands);
    }
}


public class VisibilityPass : ComputePass
{
    public void Render(CommandBuffer RenderCommands)
    {


        base.InternalRenderIndirect(RenderCommands);
    }
}


public class RenderLayer
{
    private bool Available;
    private List<ComputePass> ComputePassList;
    private CommandBuffer RenderCommands;
   
    public RenderLayer()
    {
        Available = false;
        RenderCommands = new CommandBuffer();
        ComputePassList = new List<ComputePass>();
    }

    public bool Init(LineShader InputShaders)
    {
        Available = false;
        if (!InputShaders.IsValid())
            return false;

        RenderCommands.name = "Draw Lines";

        ComputePass ExtractPass = new ExtractionPass();
        if (!ExtractPass.Init("ExtractionPass", InputShaders.ExtractPassShader))
            return false;
        ComputePass VisiblePass = new VisibilityPass();
        if (!VisiblePass.Init("VisibilityPass", InputShaders.VisibilityPassShader))
            return false;

        ComputePassList.Add(ExtractPass);
        ComputePassList.Add(VisiblePass);

        Available = true;
        Debug.Log("Render Layer Init");

        return true;
    }

    public void Destroy()
    {
        RenderCommands.Release();

        foreach (ComputePass Pass in ComputePassList)
            Pass.Destroy();
        ComputePassList.Clear();

        Available = false;
        Debug.Log("Render Layer Destroy");
    }

    public void Render()
    {
        if (!Available)
            return;

    }

    public CommandBuffer GetRenderCommand()
    {
        return RenderCommands;
    }

    public void SetConstants()
    {

    }

    public void SetVariables()
    {

    }

}

