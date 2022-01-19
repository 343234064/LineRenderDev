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

public struct LineSegment
{
    public Vector3 point3d1;
    public Vector3 point3d2;

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

    public void Init(ComputeShader ExtractPassShader)
    {
        ExtractPass = new RenderExtractPass();
        VisibilityPass = new RenderVisibilityPass();
        MaterialPass = new RenderMaterialPass();

        ExtractPass.Init(ExtractPassShader);
        VisibilityPass.Init();
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

    }

    public void InitExtractPassOutputBuffer(int AdjacencyTrianglesNum)
    {
        if (ExtractPassOutputBuffer != null)
            ExtractPassOutputBuffer.Release();

        ExtractPassOutputBuffer = new ComputeBuffer(AdjacencyTrianglesNum * 3, LineSegment.Size(), ComputeBufferType.Append);

        if (ExtractPassOutputArgBuffer == null)
        {
            ExtractPassOutputArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
        }
        int[] args = new int[4] { 2, 1, 0, 0 };
        ExtractPassOutputArgBuffer.SetData(args);

        ExtractPass.SetOutputBuffer(ExtractPassOutputBuffer, ExtractPassOutputArgBuffer);
        //temp
        MaterialPass.SetInputLineBuffer(ExtractPassOutputBuffer, ExtractPassOutputArgBuffer);
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
        ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Lines", ExtractLineBuffer);

        ExtractLineShader.SetVector("LocalSpaceViewPosition", Params.LocalCameraPosition);
        ExtractLineShader.SetFloat("CreaseAngleThreshold", Params.CreaseAngleThreshold);

        ExtractLineBuffer.SetCounterValue(0);
        ExtractLineShader.Dispatch(ExtractLineShaderKernelId, ExtractLinePassGroupSize, 1, 1);
        //Debug.Log("Group Size : " + ExtractLinePassGroupSize);
        ComputeBuffer.CopyCount(ExtractLineBuffer, ExtractLineArgBuffer, sizeof(int));
        /*
               int[] Args = new int[4] { 0,0,0,0 };
               MeshList[i].ExtractLineArgBuffer.GetData(Args);
               Debug.Log("Vertex Count " + Args[0]);
               Debug.Log("Instance Count " + Args[1]);
               Debug.Log("Start Vertex " + Args[2]);
               Debug.Log("Start Instance " + Args[3]);
        */
    }
}


public class RenderVisibilityPass
{
    public struct RenderParams
    {
    }

    public void Init()
    {

    }

    public void Destroy()
    {

    }

    public void Render(RenderVisibilityPass.RenderParams Params)
    {

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