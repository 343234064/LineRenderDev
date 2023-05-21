using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Linq;

public class TestScene : MonoBehaviour
{
    public ComputeShader Shader;

    private ComputeBuffer ResultBuffer;
    private int Kernel;
    private uint ThreadGroupSize;
    private uint DispatchThreadGroupSize;


    

    void Start()
    {
        Kernel = Shader.FindKernel("CSMain");
        Shader.GetKernelThreadGroupSizes(Kernel, out ThreadGroupSize, out _, out _);

        DispatchThreadGroupSize = 100;

        ResultBuffer = new ComputeBuffer((int)(ThreadGroupSize * DispatchThreadGroupSize), sizeof(uint) * 3);
        
    }

    void Update()
    {
        Shader.SetBuffer(Kernel, "Result", ResultBuffer);
        Debug.Log(DispatchThreadGroupSize);
      
        Shader.Dispatch(Kernel, (int)DispatchThreadGroupSize, 1, 1);

        ComputeBufferUtils.Instance.PrintInLine<Array3<uint>>(ResultBuffer, 0, 10);
    }

    void OnDestroy()
    {
        ResultBuffer.Dispose();
    }
}
