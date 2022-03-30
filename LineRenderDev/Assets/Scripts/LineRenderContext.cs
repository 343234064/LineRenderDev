using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public struct AdjFace
{
    public uint x;
    public uint y;
    public uint z;
    public uint xy;
    public uint yz;
    public uint zx;
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



public class RenderMeshContext
{
    public Mesh RumtimeMesh;
    public LineMaterial LineMaterialSetting;
    public Transform RumtimeTransform;

    public AdjFace[] AdjacencyTriangles;

    public RenderLayer Renderer;

    public RenderMeshContext(Mesh meshObj, Transform transform)
    {
        RumtimeMesh = meshObj;
        RumtimeTransform = transform;
        Renderer = new RenderLayer();
    }

    public void Destroy()
    {
        Renderer.Destroy();
    }
}


public class RenderLayer
{
    public RenderExtractPass ExtractPass;
    public RenderVisibilityPass VisibilityPass;
    public RenderMaterialPass MaterialPass;

    private ComputeBuffer ExtractPassOutputBuffer;
    private ComputeBuffer ExtractPassOutputArgBuffer;

    private ComputeBuffer VisiblePassOutputBuffer;
    private ComputeBuffer VisiblePassOutputArgBuffer;

    public void Init(ComputeShader ExtractPassShader, ComputeShader VisiblePassShader)
    {
        ExtractPass = new RenderExtractPass();
        VisibilityPass = new RenderVisibilityPass();
        MaterialPass = new RenderMaterialPass();

        ExtractPass.Init(ExtractPassShader);
        VisibilityPass.Init(VisiblePassShader);
        MaterialPass.Init();
    }

    public void Destroy()
    {
        ExtractPass.Destroy();
        VisibilityPass.Destroy();
        MaterialPass.Destroy();

        if (ExtractPassOutputBuffer != null)
            ExtractPassOutputBuffer.Release();
        if (ExtractPassOutputArgBuffer != null)
            ExtractPassOutputArgBuffer.Release();

        if (VisiblePassOutputBuffer != null)
            VisiblePassOutputBuffer.Release();
        if (VisiblePassOutputArgBuffer != null)
            VisiblePassOutputArgBuffer.Release();
    }

    public void InitInputOutputBuffer(int AdjacencyTrianglesNum)
    {
        if (ExtractPassOutputBuffer != null)
            ExtractPassOutputBuffer.Release();
        ExtractPassOutputBuffer = new ComputeBuffer(AdjacencyTrianglesNum * 3, LineTransformed.Size(), ComputeBufferType.Append);

        if (ExtractPassOutputArgBuffer == null)
        {
            ExtractPassOutputArgBuffer = new ComputeBuffer(3, sizeof(int), ComputeBufferType.IndirectArguments);
        }
        int[] Args1 = new int[3] { 0, 1, 1}; // instance count, 1, 1(for next dispatch x y z)
        ExtractPassOutputArgBuffer.SetData(Args1);
        int ExtractPassOutputArgBufferOffset = 0;


        if (VisiblePassOutputBuffer != null)
            VisiblePassOutputBuffer.Release();
        VisiblePassOutputBuffer = new ComputeBuffer(AdjacencyTrianglesNum * 3, LineSegment.Size(), ComputeBufferType.Append);

        if (VisiblePassOutputArgBuffer == null)
        {
            VisiblePassOutputArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        }
        int[] Args2 = new int[4] { 2, 1, 0, 0 }; // vertex count, instance count, start vertex, start instance
        VisiblePassOutputArgBuffer.SetData(Args2);
        int VisiblePassOutputArgBufferOffset = sizeof(int);


        if (ExtractPass != null)
            ExtractPass.SetOutputBuffer(ExtractPassOutputBuffer, ExtractPassOutputArgBuffer, ExtractPassOutputArgBufferOffset);
        if (VisibilityPass != null)
        {
            VisibilityPass.SetInputBuffer(ExtractPassOutputBuffer, ExtractPassOutputArgBuffer, ExtractPassOutputArgBufferOffset);
            VisibilityPass.SetOutputBuffer(VisiblePassOutputBuffer, VisiblePassOutputArgBuffer, VisiblePassOutputArgBufferOffset);
        }
        if(MaterialPass != null)
            MaterialPass.SetInputBuffer(VisiblePassOutputBuffer, VisiblePassOutputArgBuffer, 0);
    }
}




public class RenderExtractPass
{
    public struct RenderConstants
    {
        public uint TotalAdjacencyTrianglesNum;
        public uint SilhouetteEnable;
        public uint CreaseEnable;
        public uint BorderEnable;
        public uint HideBackFaceEdge;

        public static int Size()
        {
            return sizeof(uint) + sizeof(uint) * 4;
        }
    }

    public struct RenderParams
    {
        public Vector3 LocalCameraPosition;
        public float CreaseAngleThreshold;
        public Matrix4x4 WorldViewProjectionMatrix;
        public Matrix4x4 WorldViewMatrix;
    }

    public ComputeShader ExtractLineShader;

    private ComputeBuffer ConstantBuffer;
    private ComputeBuffer AdjacencyIndicesBuffer;
    private ComputeBuffer VerticesBuffer;

    private ComputeBuffer ExtractLineBuffer;
    private ComputeBuffer ExtractLineArgBuffer;

    private int ExtractLineArgBufferOffset;

    private int ExtractLinePassGroupSize;
    private int ExtractLineShaderKernelId;
    private uint ExtractLineShaderGroupSize;

    public void Init(ComputeShader ShaderInstance)
    {
        ExtractLineShader = ShaderInstance;
        ExtractLineShaderKernelId = ExtractLineShader.FindKernel("CSMain");
        ExtractLineShader.GetKernelThreadGroupSizes(ExtractLineShaderKernelId, out ExtractLineShaderGroupSize, out _, out _);

        ExtractLineArgBufferOffset = 0;
        ExtractLinePassGroupSize = 1;
    }

    public void Destroy()
    {
        if (ConstantBuffer != null)
            ConstantBuffer.Release();
        if (AdjacencyIndicesBuffer != null)
            AdjacencyIndicesBuffer.Release();
        if (VerticesBuffer != null)
            VerticesBuffer.Release();
    }

    public void SetAdjacencyIndicesBuffer(int AdjacencyTrianglesNum, AdjFace[] Data)
    {
        bool NeedRebuild = false;
        if (AdjacencyIndicesBuffer != null) {
            if (AdjacencyTrianglesNum != AdjacencyIndicesBuffer.count)
            {
                AdjacencyIndicesBuffer.Release();
                NeedRebuild = true;
            }
        }
        else { NeedRebuild = true; }

        if(NeedRebuild)
            AdjacencyIndicesBuffer = new ComputeBuffer(AdjacencyTrianglesNum, sizeof(uint) * 6);

        ExtractLinePassGroupSize = ((int)(AdjacencyTrianglesNum / ExtractLineShaderGroupSize) + 1);
        AdjacencyIndicesBuffer.SetData(Data);
    }

    public void SetVerticesBuffer(int VerticesNum, Vector3[] Data)
    {
        bool NeedRebuild = false;
        if (VerticesBuffer != null)
        {
            if (VerticesNum != VerticesBuffer.count)
            {
                VerticesBuffer.Release();
                NeedRebuild = true;
            }
        }
        else { NeedRebuild = true; }

        if (NeedRebuild)
            VerticesBuffer = new ComputeBuffer(VerticesNum, sizeof(float) * 3);

        VerticesBuffer.SetData(Data);
    }

    public void SetConstantBuffer(RenderExtractPass.RenderConstants[] Constants)
    {
        if (ConstantBuffer == null)
        {
            ConstantBuffer = new ComputeBuffer(1, RenderExtractPass.RenderConstants.Size(), ComputeBufferType.Constant);
        }

        ConstantBuffer.SetData(Constants);
    }

    public void SetOutputBuffer(ComputeBuffer OutputBuffer, ComputeBuffer OutputArgBuffer, int ArgBufferOffset)
    {
        ExtractLineBuffer = OutputBuffer;
        ExtractLineArgBuffer = OutputArgBuffer;
        ExtractLineArgBufferOffset = ArgBufferOffset;
    }

    public void Render(RenderExtractPass.RenderParams Params)
    {
        ExtractLineShader.SetConstantBuffer(Shader.PropertyToID("Constants"), ConstantBuffer, 0, RenderConstants.Size());
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "AdjacencyTriangles", AdjacencyIndicesBuffer);
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Vertices", VerticesBuffer);
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Output3DLines", ExtractLineBuffer);

        ExtractLineShader.SetVector("LocalSpaceViewPosition", Params.LocalCameraPosition);
        ExtractLineShader.SetFloat("CreaseAngleThreshold", Params.CreaseAngleThreshold);
        ExtractLineShader.SetMatrix("WorldViewProjection", Params.WorldViewProjectionMatrix); 
        ExtractLineShader.SetMatrix("WorldView", Params.WorldViewMatrix);

        ExtractLineBuffer.SetCounterValue(0);
        ExtractLineShader.Dispatch(ExtractLineShaderKernelId, ExtractLinePassGroupSize, 1, 1);
        //Debug.Log("Group Size : " + ExtractLinePassGroupSize);
        ComputeBuffer.CopyCount(ExtractLineBuffer, ExtractLineArgBuffer, ExtractLineArgBufferOffset);
        
        /*
        Debug.Log("========================================");
        int[] Args = new int[3] { 0,0,0 };
        ExtractLineArgBuffer.GetData(Args);
        Debug.Log("Instance Count 1 " + Args[0]);
        //Debug.Log("y Count 1 " + Args[1]);
        //Debug.Log("z Count 1 " + Args[2]);
        Debug.Log("========================================");
        */

    }
}


public class RenderVisibilityPass
{
    public struct RenderConstants
    {
        public uint HideOccludedEdge;

        public static int Size()
        {
            return sizeof(uint);
        }
    }

    public struct RenderParams
    {
        public float[] ZbufferParam;
    }

    private ComputeShader VisibleLineShader;

    private ComputeBuffer ConstantBuffer;

    private ComputeBuffer InputLineBuffer;
    private ComputeBuffer InputLineArgBuffer;

    private ComputeBuffer VisibleLineBuffer;
    private ComputeBuffer VisibleLineArgBuffer;

    private int InputLineArgBufferOffset;
    private int VisibleLineArgBufferOffset;

    private int VisibleLineShaderKernelId;
    private uint VisibleLineShaderGroupSize;


    public void Init(ComputeShader ShaderInstance)
    {
        VisibleLineShader = ShaderInstance;
        VisibleLineShaderKernelId = VisibleLineShader.FindKernel("CSMain");
        VisibleLineShader.GetKernelThreadGroupSizes(VisibleLineShaderKernelId, out VisibleLineShaderGroupSize, out _, out _);
        InputLineArgBufferOffset = 0;
        VisibleLineArgBufferOffset = 0;
    }

    public void Destroy()
    {
        if (ConstantBuffer != null)
            ConstantBuffer.Release();
    }

    public void SetInputBuffer(ComputeBuffer InputBuffer, ComputeBuffer InputArgBuffer, int ArgBufferOffset)
    {
        InputLineBuffer = InputBuffer;
        InputLineArgBuffer = InputArgBuffer;
        InputLineArgBufferOffset = ArgBufferOffset;
    }

    public void SetOutputBuffer(ComputeBuffer OutputBuffer, ComputeBuffer OutputArgBuffer, int ArgBufferOffset)
    {
        VisibleLineBuffer = OutputBuffer;
        VisibleLineArgBuffer = OutputArgBuffer;
        VisibleLineArgBufferOffset = ArgBufferOffset;
    }

    public void SetConstantBuffer(RenderVisibilityPass.RenderConstants[] Constants)
    {
        if (ConstantBuffer == null)
        {
            ConstantBuffer = new ComputeBuffer(1, RenderVisibilityPass.RenderConstants.Size(), ComputeBufferType.Constant);
        }

        ConstantBuffer.SetData(Constants);
    }

    public void Render(RenderVisibilityPass.RenderParams Params)
    {
        /*
         * ***********************************************************************************
         * 前几帧出现空的问题可能跟渲染顺序有关，compute shader在前，depth update在后
         * 在完成可见性测试后再重新考虑是否移动到camera on render image进行或者看如何插入到depth update之后
         * **********************************************************************************
         */
        Texture DepthTexture = Shader.GetGlobalTexture("_CameraDepthTexture");
        if (DepthTexture != null)
        {
            VisibleLineShader.SetTexture(VisibleLineShaderKernelId, "SceneDepthTexture", DepthTexture);
        }
        else
            Debug.Log("NULL!!!!!!!!!");
        //VisibleLineShader.SetTextureFromGlobal(VisibleLineShaderKernelId, "SceneDepthTexture", "_CameraDepthTexture");

        VisibleLineShader.SetConstantBuffer(Shader.PropertyToID("Constants"), ConstantBuffer, 0, RenderConstants.Size());
        VisibleLineShader.SetBuffer(VisibleLineShaderKernelId, "Input3DLines", InputLineBuffer);
        VisibleLineShader.SetBuffer(VisibleLineShaderKernelId, "Output2DLines", VisibleLineBuffer);

        VisibleLineShader.SetFloats("ZbufferParam", Params.ZbufferParam);

        VisibleLineBuffer.SetCounterValue(0);
        VisibleLineShader.DispatchIndirect(VisibleLineShaderKernelId, InputLineArgBuffer, (uint)InputLineArgBufferOffset);

        ComputeBuffer.CopyCount(VisibleLineBuffer, VisibleLineArgBuffer, VisibleLineArgBufferOffset);
        /*
        int[] Args = new int[4] { 0, 0, 0, 0 };
        VisibleLineArgBuffer.GetData(Args);
        Debug.Log("Vertex Count 2 " + Args[0]);
        Debug.Log("Instance Count 2 " + Args[1]);
        Debug.Log("Start Vertex 2 " + Args[2]);
        Debug.Log("Start Instance 2 " + Args[3]);
        */
    }
}


public class RenderMaterialPass
{
    public struct RenderParams
    {
        public Matrix4x4 ObjectWorldMatrix;
    }

    private LineMaterial LineMaterialObject;
    
    private ComputeBuffer InputLineBuffer;
    private ComputeBuffer InputLineArgBuffer;

    private int InputLineArgBufferOffset;

    public void Init()
    {
        InputLineArgBufferOffset = 0;
    }

    public void Destroy()
    {

    }

    public void SetLineMaterialObject(LineMaterial Material)
    {
        LineMaterialObject = Material;
    }

    public void SetInputBuffer(ComputeBuffer InputBuffer, ComputeBuffer InputArgBuffer, int ArgBufferOffset)
    {
        InputLineBuffer = InputBuffer;
        InputLineArgBuffer = InputArgBuffer;
        InputLineArgBufferOffset = ArgBufferOffset;
    }

    public void Render(RenderMaterialPass.RenderParams Params)
    {
        LineMaterialObject.LineRenderMaterial.SetMatrix("_ObjectWorldMatrix", Params.ObjectWorldMatrix);
        LineMaterialObject.LineRenderMaterial.SetBuffer("Lines", InputLineBuffer);
        //MeshList[i].LineMaterialSetting.LineRenderMaterial.SetBuffer("Positions", MeshList[i].VerticesBuffer);

        Graphics.DrawProceduralIndirect(LineMaterialObject.LineRenderMaterial, LineMaterialObject.EdgeBoundingVolume, MeshTopology.Lines, InputLineArgBuffer, InputLineArgBufferOffset);
    }
}