using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

public class TestSlice : MonoBehaviour
{
    public ComputeShader SliceShader;
    private int SumSingleBucketInclusiveKernelId;
    private int SumAllBucketKernelId;
    private int SumTotalKernelId;
    private uint SumThreadGroupSize;

    private const int ArrayNum = 10000;
    private uint[] RandomArray;

    private ComputeBuffer InputArrayBuffer;
    private ComputeBuffer BucketBuffer;
    private ComputeBuffer OutputArrayBuffer;

    void Start()
    {
        RandomArray = new uint[ArrayNum];
        for(int i = 0; i < ArrayNum; i++)
        {
            RandomArray[i] = (uint)UnityEngine.Random.Range(0, ArrayNum);
        }
        Debug.Log(String.Join("|", RandomArray));

        SumSingleBucketInclusiveKernelId = SliceShader.FindKernel("SumSingleBucketInclusive");
        SliceShader.GetKernelThreadGroupSizes(SumSingleBucketInclusiveKernelId, out SumThreadGroupSize, out _, out _);

        SumAllBucketKernelId = SliceShader.FindKernel("SumAllBucket");
        SumTotalKernelId = SliceShader.FindKernel("SumTotal");

        InputArrayBuffer = new ComputeBuffer(ArrayNum, sizeof(uint));
        InputArrayBuffer.SetData(RandomArray);

        BucketBuffer = new ComputeBuffer((int)SumThreadGroupSize, sizeof(uint));

        OutputArrayBuffer = new ComputeBuffer(ArrayNum, sizeof(uint));

    }


    void Update()
    {
        int ThreadGroupCount = (int)(Mathf.Ceil((float)ArrayNum / (float)SumThreadGroupSize));

        SliceShader.SetInt("InputSize", ArrayNum);
        SliceShader.SetInt("OutputSize", ArrayNum);
        SliceShader.SetBuffer(SumSingleBucketInclusiveKernelId, "InputArray", InputArrayBuffer);
        SliceShader.SetBuffer(SumSingleBucketInclusiveKernelId, "OutputArray", OutputArrayBuffer);
        SliceShader.Dispatch(SumSingleBucketInclusiveKernelId, ThreadGroupCount, 1, 1);

        //ComputePass.PrintComputeBufferInLine<uint>(OutputArrayBuffer, 0, ArrayNum);

        SliceShader.SetInt("InputSize", ArrayNum);
        SliceShader.SetInt("OutputSize", (int)SumThreadGroupSize);
        SliceShader.SetBuffer(SumAllBucketKernelId, "InputArray", OutputArrayBuffer);
        SliceShader.SetBuffer(SumAllBucketKernelId, "BucketArray", BucketBuffer);
        // Bottleneck:
        // You can only have a max num of SumThreadGroupSize of bucket num
        // And the num of elements that every bucket can handle : SumThreadGroupSize
        // So the max num of elements : SumThreadGroupSize * SumThreadGroupSize
        SliceShader.Dispatch(SumAllBucketKernelId, 1, 1, 1); 
        
        //ComputePass.PrintComputeBufferInLine<uint>(BucketBuffer, 0, ThreadGroupCount);

        SliceShader.SetInt("OutputSize", ArrayNum);
        SliceShader.SetBuffer(SumTotalKernelId, "BucketArray", BucketBuffer);
        SliceShader.SetBuffer(SumTotalKernelId, "OutputArray", OutputArrayBuffer);
        SliceShader.Dispatch(SumTotalKernelId, ThreadGroupCount, 1, 1);

       // ComputePass.PrintComputeBufferInLine<uint>(OutputArrayBuffer, 0, ArrayNum);

    }

    void OnDestroy()
    {
        if (InputArrayBuffer != null)
            InputArrayBuffer.Release();
        if (BucketBuffer != null)
            BucketBuffer.Release();
        if (OutputArrayBuffer != null)
            OutputArrayBuffer.Release();

    }
}
