using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;

using Sirenix.OdinInspector;

public struct AdjFace
{
    public uint x;
    public uint y;
    public uint z;
    public uint xy;
    public uint yz;
    public uint zx;
}

public struct RenderConstants
{
    public uint TotalAdjacencyTrianglesNum;
    public int SilhouetteEnable;
    public int CreaseEnable;
    public int BorderEnable;

    public static int Size()
    {
        return sizeof(uint) + sizeof(int) * 3;
    }
}

public class RenderMeshContext
{
    public Mesh RumtimeMesh;
    public LineMaterial LineMaterialSetting;
    public Transform RumtimeTransform;

    public AdjFace[] AdjacencyTriangles;

    public ComputeBuffer ConstantBuffer;
    public ComputeBuffer AdjacencyIndicesBuffer;
    public ComputeBuffer VerticesBuffer;
    public ComputeBuffer ExtractLineBuffer;
    public ComputeBuffer ExtractLineArgBuffer;


    public int ExtractLinePassGroupSize;

    public RenderMeshContext(Mesh meshObj, Transform transform)
    {
        RumtimeMesh = meshObj;
        RumtimeTransform = transform;
    }

    public void Destroy()
    {
        if (ConstantBuffer != null)
            ConstantBuffer.Release();
        if (AdjacencyIndicesBuffer != null)
            AdjacencyIndicesBuffer.Release();
        if (VerticesBuffer != null)
            VerticesBuffer.Release();
        if (ExtractLineBuffer != null)
            ExtractLineBuffer.Release();
        if (ExtractLineArgBuffer != null)
            ExtractLineArgBuffer.Release();
    }
}


/// <summary>
/// Static Mesh Ver
/// 
/// </summary>
/// 

/// 
/// vertex也采用append方式，不必全部传进去
/// 
public class LinesRenderer : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        GetMeshInformation();
        LoadAdjacency();

        Setup();
        
    }

    // Update is called once per frame
    void Update()
    {
        for (int i = 0; i < MeshList.Count; i++)
        {
            if (MeshList[i].LineMaterialSetting != null && MeshList[i].RumtimeTransform.gameObject.activeSelf)
            {
                ExtractLineShader.SetConstantBuffer(Shader.PropertyToID("Constants"), MeshList[i].ConstantBuffer, 0, RenderConstants.Size());
                ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "AdjacencyTriangles", MeshList[i].AdjacencyIndicesBuffer);
                ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Vertices", MeshList[i].VerticesBuffer);
                ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "LineIndices", MeshList[i].ExtractLineBuffer);

                Vector3 LocalCameraPosition = MeshList[i].RumtimeTransform.InverseTransformPoint(Camera.main.transform.position);
                ExtractLineShader.SetVector("LocalSpaceViewPosition", LocalCameraPosition);

                float CreaseAngleThreshold = (180.0f - MeshList[i].LineMaterialSetting.CreaseAngleDegreeThreshold) * (0.017453292519943294f);
                ExtractLineShader.SetFloat("CreaseAngleThreshold", CreaseAngleThreshold);

                MeshList[i].ExtractLineBuffer.SetCounterValue(0);
                ExtractLineShader.Dispatch(ExtractLineShaderKernelId, MeshList[i].ExtractLinePassGroupSize, 1, 1);
                //Debug.Log("Group Size : " + MeshList[i].ExtractLinePassGroupSize);
                ComputeBuffer.CopyCount(MeshList[i].ExtractLineBuffer, MeshList[i].ExtractLineArgBuffer, sizeof(int));
                /*
                int[] Args = new int[4] { 0,0,0,0 };
                MeshList[i].ExtractLineArgBuffer.GetData(Args);
                Debug.Log("Vertex Count " + Args[0]);
                Debug.Log("Instance Count " + Args[1]);
                Debug.Log("Start Vertex " + Args[2]);
                Debug.Log("Start Instance " + Args[3]);
                */


                Matrix4x4 WorldMatrix = MeshList[i].RumtimeTransform.localToWorldMatrix;
                MeshList[i].LineMaterialSetting.LineRenderMaterial.SetMatrix("_ObjectWorldMatrix", WorldMatrix);
                MeshList[i].LineMaterialSetting.LineRenderMaterial.SetBuffer("LinesIndex", MeshList[i].ExtractLineBuffer);
                MeshList[i].LineMaterialSetting.LineRenderMaterial.SetBuffer("Positions", MeshList[i].VerticesBuffer);
               
                Graphics.DrawProceduralIndirect(MeshList[i].LineMaterialSetting.LineRenderMaterial, MeshList[i].LineMaterialSetting.EdgeBoundingVolume, MeshTopology.Lines, MeshList[i].ExtractLineArgBuffer, 0);
                //Graphics.DrawProcedural(MeshList[i].LineMaterial, BoundingVolume, MeshTopology.Lines, 2, Args[0]);
            }
        }
 
    }

    void LateUpdate()
    {

    }

    void OnDestroy()
    {
        if (MeshList != null)
        {
            for (int i = 0; i < MeshList.Count; i++)
            {
                MeshList[i].Destroy();
            }
        }
    }


    /// ///////////////////////////////////////////////////////
    [AssetSelector(Paths = "Assets/Shaders")]
    [Required("Extract Line Shader Is Required")]
    public ComputeShader ExtractLineShader;
    /// ///////////////////////////////////////////////////////
    private List<RenderMeshContext> MeshList;
    private int ExtractLineShaderKernelId;
    private uint ExtractLineShaderGroupSize;
    /// ///////////////////////////////////////////////////////


    private void GetMeshInformation()
    {
        MeshList = new List<RenderMeshContext>();

        MeshFilter SelectedMesh = gameObject.GetComponent<MeshFilter>();
        if (SelectedMesh == null)
        {
            int SubNodesNum = gameObject.transform.childCount;
            if (SubNodesNum != 0)
            {
                for (int i = 0; i < SubNodesNum; i++)
                {
                    GameObject child = gameObject.transform.GetChild(i).gameObject;
                    MeshFilter SubMesh = child.GetComponent<MeshFilter>();
                    LineMaterial MatSetting = child.GetComponent<LineMaterial>();

                    if (SubMesh == null) break;
                    else
                    {
                        RenderMeshContext ToAdd = new RenderMeshContext(SubMesh.sharedMesh, child.transform);
                        if (MatSetting != null)
                        {
                            ToAdd.LineMaterialSetting = MatSetting;
                        }
                        else
                        {
                            ToAdd.LineMaterialSetting = null;
                            Debug.Log("===============Mesh: " + child.name + " has NO Line Material applied, the mesh will be ignored=================");
                        }
                        MeshList.Add(ToAdd);
                    }
                }
            }
        }
        else
        {
            RenderMeshContext ToAdd = new RenderMeshContext(SelectedMesh.sharedMesh, gameObject.transform);
            LineMaterial MatSetting = gameObject.GetComponent<LineMaterial>();
            if (MatSetting != null)
            {
                ToAdd.LineMaterialSetting = MatSetting;
            }
            else
            {
                ToAdd.LineMaterialSetting = null;
                Debug.Log("===============Mesh: " + gameObject.name + " has NO Line Material applied, the mesh will be ignored=================");
            }
            MeshList.Add(ToAdd);
        }

        Debug.Log("Mesh Name: " + gameObject.name);
        Debug.Log("Mesh Count: " + MeshList.Count);
    }


    private void LoadAdjacency()
    {
        if (MeshList.Count == 0) return;

        string FileName = AssetDatabase.GetAssetPath(MeshList[0].RumtimeMesh);
        int dotIndex = FileName.LastIndexOf('.');
        if (dotIndex != -1)
        {
            FileName = FileName.Substring(0, dotIndex);
        }
        FileName += ".adjacency";
        Debug.Log("Start Load Adjacency : " + FileName);

        if (File.Exists(FileName))
        {
            using (BinaryReader Reader = new BinaryReader(File.Open(FileName, FileMode.Open)))
            {
                int MeshNum = Reader.ReadInt32();
                int OffsetPerData = Reader.ReadInt32();
                Debug.Log("Mesh Num:" + MeshNum);
                Debug.Log("Offset Per Data:" + OffsetPerData);

                if (MeshNum == MeshList.Count)
                {
                    for (int i = 0; i < MeshNum; i++)
                    {
                        int TotalAdjFaceNum = Reader.ReadInt32();
                        Debug.Log("Mesh : " + i + " | Total AdjFace Num:" + TotalAdjFaceNum);
                        MeshList[i].AdjacencyTriangles = new AdjFace[TotalAdjFaceNum];
                        

                        int Offset = 0;
                        for (int j = 0; j < TotalAdjFaceNum; j++)
                        {
                            AdjFace Face = new AdjFace();
                            Face.x = (uint)Reader.ReadInt32();
                            Face.y = (uint)Reader.ReadInt32();
                            Face.z = (uint)Reader.ReadInt32();
                            Face.xy = (uint)Reader.ReadInt32();
                            Face.yz = (uint)Reader.ReadInt32();
                            Face.zx = (uint)Reader.ReadInt32();

                            MeshList[i].AdjacencyTriangles[Offset] = Face; 
                            Offset++;
                        }

                    }
                }
                else
                {
                    Debug.Log("Mesh Num From File Not Equal To Current Mesh.");
                }
            }

        }
        else
        {
            Debug.Log("Cannot Find Adjacency File.....");
        }

        Debug.Log("Load Adjacency Completed.");
    }

    void Setup()
    { 
        /*
        ComputeShader[] ComputeShaderList = (ComputeShader[])Resources.FindObjectsOfTypeAll(typeof(ComputeShader));
        for (int i = 0; i < ComputeShaderList.Length; i++)
        {
            Debug.Log(ComputeShaderList[i].name);
            if (ComputeShaderList[i].name == "ExtractLine")
                ExtractLineShader = ComputeShaderList[i];
        }*/

        ExtractLineShaderKernelId = ExtractLineShader.FindKernel("CSMain");
        ExtractLineShader.GetKernelThreadGroupSizes(ExtractLineShaderKernelId, out ExtractLineShaderGroupSize, out _, out _);


        for (int i = 0; i < MeshList.Count; i++)
        {
            if (!MeshList[i].RumtimeTransform.gameObject.activeSelf) continue;

            MeshList[i].AdjacencyIndicesBuffer = new ComputeBuffer(MeshList[i].AdjacencyTriangles.Length, sizeof(uint) * 6);
            MeshList[i].AdjacencyIndicesBuffer.SetData(MeshList[i].AdjacencyTriangles);

            MeshList[i].VerticesBuffer = new ComputeBuffer(MeshList[i].RumtimeMesh.vertices.Length, sizeof(float) * 3);
            MeshList[i].VerticesBuffer.SetData(MeshList[i].RumtimeMesh.vertices);

            MeshList[i].ExtractLineBuffer = new ComputeBuffer(MeshList[i].AdjacencyTriangles.Length * 3, sizeof(uint) * 2, ComputeBufferType.Append);

            MeshList[i].ExtractLineArgBuffer = new ComputeBuffer(4, sizeof(int), ComputeBufferType.IndirectArguments);
            int[] args = new int[4] { 2, 1, 0, 0 };
            MeshList[i].ExtractLineArgBuffer.SetData(args);

            MeshList[i].ExtractLinePassGroupSize = ((int)(MeshList[i].AdjacencyTriangles.Length / ExtractLineShaderGroupSize) + 1);

            MeshList[i].LineMaterialSetting.LineRenderMaterial.renderQueue = (int)RenderQueue.Geometry + 1;

            RenderConstants[] Constants = new RenderConstants[1];
            Constants[0].TotalAdjacencyTrianglesNum = (uint)MeshList[i].AdjacencyTriangles.Length;
            Constants[0].SilhouetteEnable = MeshList[i].LineMaterialSetting.SilhouetteEnable ? 2 : 0;
            Constants[0].CreaseEnable = MeshList[i].LineMaterialSetting.CreaseEnable ? 2 : 0;
            Constants[0].BorderEnable = MeshList[i].LineMaterialSetting.BorderEnable ? 2 : 0;
            MeshList[i].ConstantBuffer = new ComputeBuffer(1, RenderConstants.Size(), ComputeBufferType.Constant);
            MeshList[i].ConstantBuffer.SetData(Constants);

        }

    }

   

}
