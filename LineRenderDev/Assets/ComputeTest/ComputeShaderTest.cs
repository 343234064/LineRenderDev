using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ComputeShaderTest : MonoBehaviour
{
	public int SphereAmount = 17;
	public float Scale = 0.2f;
	public ComputeShader MyShader;

	public GameObject Prefab;

	private int Kernel;
	private uint ThreadGroupSize;

	private ComputeBuffer ResultBuffer;
	private Vector4[] Output;
	private Transform[] Instances;

    // Start is called before the first frame update
    void Start()
    {
        Kernel = MyShader.FindKernel("CSMain");
        MyShader.GetKernelThreadGroupSizes(Kernel, out ThreadGroupSize, out _, out _);

        ResultBuffer = new ComputeBuffer(SphereAmount, sizeof(float) * 4);
        Output = new Vector4[SphereAmount];

        Instances = new Transform[SphereAmount];
		for (int i = 0; i < SphereAmount; i++)
		{
    		Instances[i] = Instantiate(Prefab, transform).transform;
		}
    }

    // Update is called once per frame
    void Update()
    {
        MyShader.SetBuffer(Kernel, "Result", ResultBuffer);
        MyShader.SetFloat("Time", Time.time);
        int ThreadGroups = (int)((SphereAmount + (ThreadGroupSize-1))/ThreadGroupSize);
        MyShader.Dispatch(Kernel, ThreadGroups, 1, 1);
        ResultBuffer.GetData(Output);

        for (int i = 0; i < Instances.Length; i++){
    		Instances[i].localPosition = Output[i];
    		Instances[i].localScale = new Vector3(Scale,Scale,Scale);
    	}
    }

    void OnDestroy()
    {
    	ResultBuffer.Dispose();
    }
}
