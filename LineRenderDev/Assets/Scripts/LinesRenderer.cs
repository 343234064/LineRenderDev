using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEditor;
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

/// <summary>
/// 先完成静态Mesh版本的
/// 后续看是否能通过GPU Skin方式解决动画问题
/// </summary>
/// 

/// setbuffer移到start()
/// vertex采用append方式
public class LinesRenderer : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        GetMeshInformation();
        if (Enable)
        {
            LoadAdjacency();

            Setup();
        }
    }

    // Update is called once per frame
    void Update()
    {
        if (Enable)
        {
            for (int i = 0; i < MeshList.Count; i++)
            {
                ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "AdjacencyTriangles", MeshList[i].AdjacencyIndicesBuffer);
                ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "Vertices", MeshList[i].VerticesBuffer);
                ExtractLineShader.SetBuffer(ExtractLineShaderKernelId, "LineIndices", MeshList[i].ExtractLineBuffer);

                MeshList[i].ExtractLineBuffer.SetCounterValue(0);
               
                ExtractLineShader.SetInt("TotalAdjacencyTrianglesNum", MeshList[i].AdjacencyTriangles.Length);
                ExtractLineShader.Dispatch(ExtractLineShaderKernelId, MeshList[i].ExtractLinePassGroupSize, 1, 1);

                ComputeBuffer.CopyCount(MeshList[i].ExtractLineBuffer, MeshList[i].ExtractLineArgBuffer, 0);
                int[] Args = new int[] { 0 };
                MeshList[i].ExtractLineArgBuffer.GetData(Args);

                //Debug.Log("Group Size : " + MeshList[i].ExtractLinePassGroupSize);
                //Debug.Log("Instance Count " + Args[0]);
                //Debug.Log("Mesh: " + i);
                Matrix4x4 WorldMatrix = MeshList[i].RumtimeTransform.localToWorldMatrix;
                LineRenderMaterial.SetMatrix("_ObjectWorldMatrix", WorldMatrix);
                LineRenderMaterial.SetBuffer("LinesIndex", MeshList[i].ExtractLineBuffer);
                LineRenderMaterial.SetBuffer("Positions", MeshList[i].VerticesBuffer);

                //Graphics.DrawProceduralIndirect(LineRenderMaterial, BoundingVolume, MeshTopology.Lines, MeshList[i].ExtractLineArgBuffer, 0);
                Graphics.DrawProcedural(LineRenderMaterial, BoundingVolume, MeshTopology.Lines, 2, Args[0]);
            }
        }
    }


    void OnPostRender()
    {
        Debug.Log("XXXXXXXXXXXXXX");
    }

    void OnDestroy()
    {
        for (int i = 0; i < MeshList.Count; i++)
        {
            MeshList[i].Destroy();
        }
    }

    public class MeshData
    {
        public Mesh RumtimeMesh;
        public Transform RumtimeTransform;

        public AdjFace[] AdjacencyTriangles;

        public ComputeBuffer AdjacencyIndicesBuffer;
        public ComputeBuffer VerticesBuffer;
        public ComputeBuffer ExtractLineBuffer;
        public ComputeBuffer ExtractLineArgBuffer;

        public int ExtractLinePassGroupSize;

        public MeshData(Mesh meshObj, Transform transform)
        {
            RumtimeMesh = meshObj;
            RumtimeTransform = transform;
        }

        public void Destroy()
        {
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


    /// ///////////////////////////////////////////////////////
    public bool Enable;
    public Material LineRenderMaterial;
    public ComputeShader ExtractLineShader;
    /// ///////////////////////////////////////////////////////
    private List<MeshData> MeshList;

    private int ExtractLineShaderKernelId;
    private uint ExtractLineShaderGroupSize;

    private Bounds BoundingVolume;
    /// ///////////////////////////////////////////////////////


    private void GetMeshInformation()
    {
        MeshList = new List<MeshData>();

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
                    if (SubMesh == null) break;
                    else
                    {
                        MeshData ToAdd = new MeshData(SubMesh.sharedMesh, child.transform);
                        MeshList.Add(ToAdd);
                    }
                }
            }
        }
        else
        {
            MeshData ToAdd = new MeshData(SelectedMesh.sharedMesh, gameObject.transform);
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
            MeshList[i].AdjacencyIndicesBuffer = new ComputeBuffer(MeshList[i].AdjacencyTriangles.Length, sizeof(uint) * 6);
            MeshList[i].AdjacencyIndicesBuffer.SetData(MeshList[i].AdjacencyTriangles);

            MeshList[i].VerticesBuffer = new ComputeBuffer(MeshList[i].RumtimeMesh.vertices.Length, sizeof(float) * 3);
            MeshList[i].VerticesBuffer.SetData(MeshList[i].RumtimeMesh.vertices);

            MeshList[i].ExtractLineBuffer = new ComputeBuffer(MeshList[i].AdjacencyTriangles.Length * 3 * 2, sizeof(uint), ComputeBufferType.Append);
            

            MeshList[i].ExtractLineArgBuffer = new ComputeBuffer(1, sizeof(int), ComputeBufferType.IndirectArguments);

            int[] args = new int[] { 0 };
            MeshList[i].ExtractLineArgBuffer.SetData(args);

            ComputeBuffer.CopyCount(MeshList[i].ExtractLineBuffer, MeshList[i].ExtractLineArgBuffer, 0);

            MeshList[i].ExtractLinePassGroupSize = ((int)(MeshList[i].AdjacencyTriangles.Length / ExtractLineShaderGroupSize) + 1);

        }

        BoundingVolume = new Bounds(Vector3.zero, Vector3.one * 20);
    }

   

}
