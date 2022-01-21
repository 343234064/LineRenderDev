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

public struct Line3D
{
    public Vector3 point3d1;
    public Vector3 point3d2;

    public static int Size()
    {
        return sizeof(float) * 3 * 2;
    }
}

public struct Line2D
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
        ExtractPassOutputBuffer = new ComputeBuffer(AdjacencyTrianglesNum * 3, Line3D.Size(), ComputeBufferType.Append);

        if (ExtractPassOutputArgBuffer == null)
        {
            ExtractPassOutputArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        }
        int[] Args1 = new int[3] { 0, 1, 1}; // instance count, 1, 1(for next dispatch x y z)
        ExtractPassOutputArgBuffer.SetData(Args1);


        if (VisiblePassOutputBuffer != null)
            VisiblePassOutputBuffer.Release();
        VisiblePassOutputBuffer = new ComputeBuffer(AdjacencyTrianglesNum * 3, Line2D.Size(), ComputeBufferType.Append);

        if (VisiblePassOutputArgBuffer == null)
        {
            VisiblePassOutputArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        }
        int[] Args2 = new int[4] { 2, 1, 0, 0 }; // vertex count, instance count, start vertex, start instance
        VisiblePassOutputArgBuffer.SetData(Args2);


        if (ExtractPass != null)
            ExtractPass.SetOutputBuffer(ExtractPassOutputBuffer, ExtractPassOutputArgBuffer);
        if (VisibilityPass != null)
        {
            VisibilityPass.SetInputBuffer(ExtractPassOutputBuffer, ExtractPassOutputArgBuffer);
            VisibilityPass.SetOutputBuffer(VisiblePassOutputBuffer, VisiblePassOutputArgBuffer);
        }
        if(MaterialPass != null)
            MaterialPass.SetInputLineBuffer(VisiblePassOutputBuffer, VisiblePassOutputArgBuffer);
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
    }

    public ComputeShader ExtractLineShader;

    private ComputeBuffer ConstantBuffer;
    private ComputeBuffer AdjacencyIndicesBuffer;
    private ComputeBuffer VerticesBuffer;

    private ComputeBuffer ExtractLineBuffer;
    private ComputeBuffer ExtractLineArgBuffer;

    private int ExtractLinePassGroupSize;
    private int ExtractLineShaderKernelId;
    private uint ExtractLineShaderGroupSize;

    public void Init(ComputeShader ShaderInstance)
    {
        ExtractLineShader = ShaderInstance;
        ExtractLineShaderKernelId = ExtractLineShader.FindKernel("CSMain");
        ExtractLineShader.GetKernelThreadGroupSizes(ExtractLineShaderKernelId, out ExtractLineShaderGroupSize, out _, out _);

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

    public void SetOutputBuffer(ComputeBuffer OutputBuffer, ComputeBuffer OutputArgBuffer)
    {
        ExtractLineBuffer = OutputBuffer;
        ExtractLineArgBuffer = OutputArgBuffer;
    }

    public void Render(RenderExtractPass.RenderParams Params)
    {
        ExtractLineShader.SetConstantBuffer(Shader.PropertyToID("Constants"), ConstantBuffer, 0, RenderConstants.Size());
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "AdjacencyTriangles", AdjacencyIndicesBuffer);
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Vertices", VerticesBuffer);
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Output3DLines", ExtractLineBuffer);

        ExtractLineShader.SetVector("LocalSpaceViewPosition", Params.LocalCameraPosition);
        ExtractLineShader.SetFloat("CreaseAngleThreshold", Params.CreaseAngleThreshold);

        ExtractLineBuffer.SetCounterValue(0);
        ExtractLineShader.Dispatch(ExtractLineShaderKernelId, ExtractLinePassGroupSize, 1, 1);
        //Debug.Log("Group Size : " + ExtractLinePassGroupSize);
        ComputeBuffer.CopyCount(ExtractLineBuffer, ExtractLineArgBuffer, 0);
        /*
               int[] Args = new int[3] { 0,0,0 };
               ExtractLineArgBuffer.GetData(Args);
               Debug.Log("Instance Count 1 " + Args[0]);
               Debug.Log("y Count 1 " + Args[1]);
               Debug.Log("z Count 1 " + Args[2]);
        */
    }
}


public class RenderVisibilityPass
{
    public struct RenderConstants
    {
        public uint HideOccludedEdge;
        public Vector2 RenderResolution;

        public static int Size()
        {
            return sizeof(uint) + sizeof(float) * 2;
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

    private int VisibleLineShaderKernelId;
    private uint VisibleLineShaderGroupSize;


    public void Init(ComputeShader ShaderInstance)
    {
        VisibleLineShader = ShaderInstance;
        VisibleLineShaderKernelId = VisibleLineShader.FindKernel("CSMain");
        VisibleLineShader.GetKernelThreadGroupSizes(VisibleLineShaderKernelId, out VisibleLineShaderGroupSize, out _, out _);
    }

    public void Destroy()
    {
        if (ConstantBuffer != null)
            ConstantBuffer.Release();
    }

    public void SetInputBuffer(ComputeBuffer InputBuffer, ComputeBuffer InputArgBuffer)
    {
        InputLineBuffer = InputBuffer;
        InputLineArgBuffer = InputArgBuffer;
    }

    public void SetOutputBuffer(ComputeBuffer OutputBuffer, ComputeBuffer OutputArgBuffer)
    {
        VisibleLineBuffer = OutputBuffer;
        VisibleLineArgBuffer = OutputArgBuffer;
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
        VisibleLineShader.SetConstantBuffer(Shader.PropertyToID("Constants"), ConstantBuffer, 0, RenderConstants.Size());
        VisibleLineShader.SetBuffer(VisibleLineShaderKernelId, "Input3DLines", InputLineBuffer);
        VisibleLineShader.SetBuffer(VisibleLineShaderKernelId, "Output2DLines", VisibleLineBuffer);

        VisibleLineShader.SetFloats("ZbufferParam", Params.ZbufferParam);

        VisibleLineBuffer.SetCounterValue(0);
        VisibleLineShader.DispatchIndirect(VisibleLineShaderKernelId, InputLineArgBuffer, 0);

        ComputeBuffer.CopyCount(VisibleLineBuffer, VisibleLineArgBuffer, sizeof(int));
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


    public void Init()
    {

    }

    public void Destroy()
    {

    }

    public void SetLineMaterialObject(LineMaterial Material)
    {
        LineMaterialObject = Material;
    }

    public void SetInputLineBuffer(ComputeBuffer InputBuffer, ComputeBuffer InputArgBuffer)
    {
        InputLineBuffer = InputBuffer;
        InputLineArgBuffer = InputArgBuffer;
    }

    public void Render(RenderMaterialPass.RenderParams Params)
    {
        LineMaterialObject.LineRenderMaterial.SetMatrix("_ObjectWorldMatrix", Params.ObjectWorldMatrix);
        LineMaterialObject.LineRenderMaterial.SetBuffer("Lines", InputLineBuffer);
        //MeshList[i].LineMaterialSetting.LineRenderMaterial.SetBuffer("Positions", MeshList[i].VerticesBuffer);

        Graphics.DrawProceduralIndirect(LineMaterialObject.LineRenderMaterial, LineMaterialObject.EdgeBoundingVolume, MeshTopology.Lines, InputLineArgBuffer, 0);
    }
}