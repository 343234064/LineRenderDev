using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


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
        CoreShaderGroupSize = new Array3<uint>(0, 0, 0);
        InputBuffers = new List<BufferParam>();
        OutputBuffers = new List<BufferParam>();
        ConstantsBuffers = new List<BufferParam>();
    }

    public virtual bool Init(string PassName, ComputeShader InputShader)
    {
        if (InputShader == null)
            return false;

        Name = PassName;

        CoreShader = InputShader;
        CoreShaderKernelId = CoreShader.FindKernel("CSMain");
        Debug.Log(CoreShaderKernelId);

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

        RenderCommands.DispatchCompute(CoreShader, CoreShaderKernelId, DispatchGroupSize.x, DispatchGroupSize.y, DispatchGroupSize.z);

        foreach (BufferParam OBuffer in OutputBuffers)
        {
            if (OBuffer.NeedCopyValue)
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