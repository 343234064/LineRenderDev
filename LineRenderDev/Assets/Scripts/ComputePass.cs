using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


public class ComputeBufferUtils
{
    private static readonly ComputeBufferUtils instance = new ComputeBufferUtils();
    private ComputeBuffer ArgBuffer;
    private int[] Args;

    static ComputeBufferUtils()
    {
    }

    private ComputeBufferUtils()
    {
        ArgBuffer = new ComputeBuffer(1, sizeof(int), ComputeBufferType.IndirectArguments);
        Args = new int[1] { 0 };
        ArgBuffer.SetData(Args);
    }

    public static ComputeBufferUtils Instance
    {
        get
        {
            return instance;
        }
    }


    public int GetCount(ComputeBuffer Buffer)
    {
        ComputeBuffer.CopyCount(Buffer, ArgBuffer, 0);
        ArgBuffer.GetData(Args, 0, 0, 1);
        Debug.Log("++++++++++++++" + Args[0]);
        return Args[0];
    }

    public void Print<T>(ComputeBuffer Buffer, int StartIndex, int EndIndex) where T : struct
    {
        int N = EndIndex - StartIndex;
        T[] Array = new T[N];
        Buffer.GetData(Array, 0, StartIndex, N);
        for (int i = 0; i < N; i++)
        {
            Debug.LogFormat("index={0}: {1}", StartIndex + i, Array[i]);
        }
    }

    public void PrintInLine<T>(ComputeBuffer Buffer, int StartIndex, int EndIndex) where T : struct
    {
        int N = EndIndex - StartIndex;
        T[] Array = new T[N];
        Buffer.GetData(Array, 0, StartIndex, N);
        Debug.Log(System.String.Join("|", new List<T>(Array).ConvertAll(i => i.ToString()).ToArray()));
    }

    public void Print<T>(ComputeBuffer Buffer) where T : struct
    {
        int N = GetCount(Buffer);
        T[] Array = new T[N];
        Buffer.GetData(Array, 0, 0, N);
        for (int i = 0; i < N; i++)
        {
            Debug.LogFormat("index={0}: {1}",  i, Array[i]);
        }
    }

    public void PrintInLine<T>(ComputeBuffer Buffer) where T : struct
    {
        int N = GetCount(Buffer);
        T[] Array = new T[N];
        Buffer.GetData(Array, 0, 0, N);
        Debug.Log(System.String.Join("|", new List<T>(Array).ConvertAll(i => i.ToString()).ToArray()));
    }
}

public class ComputePass
{
    public string Name;
    protected bool Available;

    public Array3<uint> CoreShaderGroupSize;
    public int CoreShaderKernelId;
    public ComputeShader CoreShader;


    public ComputePass()
    {
        Name = "ComputePass";
        Available = false;
        CoreShaderGroupSize = new Array3<uint>(0, 0, 0);
        CoreShaderKernelId = 0;
        CoreShader = null;
    }

    public virtual bool Init(string PassName, string KernelName, ComputeShader InputShader)
    {
        if (InputShader == null)
            return false;

        Name = PassName;

        CoreShader = InputShader;
        CoreShaderKernelId = CoreShader.FindKernel(KernelName);

        CoreShader.GetKernelThreadGroupSizes(CoreShaderKernelId, out CoreShaderGroupSize.x, out CoreShaderGroupSize.y, out CoreShaderGroupSize.z);

        Available = true;
        return true;
    }

    public virtual void Destroy()
    {
        Available = false;
    }


}


