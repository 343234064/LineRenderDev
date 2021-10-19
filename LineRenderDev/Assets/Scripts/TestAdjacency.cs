using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using UnityEditor;
using UnityEngine;
using Unity.Collections;




[ExecuteInEditMode]
public class TestAdjacency : MonoBehaviour
{

    public class MeshData
    {
        public Mesh RumtimeMesh;
        public Mesh CachedMesh;
        public int[] Triangles;

        public MeshData(Mesh meshObj)
        {
            RumtimeMesh = meshObj;
            CachedMesh = Mesh.Instantiate(RumtimeMesh);
        }

    }

    public bool LoadAdjacency = false;
    public bool ShowAdjacency = false;
    public bool ResetMesh = false;

    private List<MeshData> MeshList;

   void OnEnable()
    {
        Debug.Log("Test Script Enable");
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
                        MeshData ToAdd = new MeshData(SubMesh.sharedMesh);
                        MeshList.Add(ToAdd);
                    }
                }
            }
        }
        else
        {
            MeshData ToAdd = new MeshData(SelectedMesh.sharedMesh);
            MeshList.Add(ToAdd);
        }
    }

    void Update()
    {
        if(LoadAdjacency)
        {
            OnLoadAdjacency();
            LoadAdjacency = false;
        }

        if(ShowAdjacency)
        {
            OnShowAdjacency();
            ShowAdjacency = false;
        }

        if(ResetMesh)
        {
            OnResetMesh();
            ResetMesh = false;
        }
    }

    private void OnLoadAdjacency()
    {
        if (MeshList.Count == 0) return;

        Debug.Log("Start Load Adjacency.....");
        String FileName = AssetDatabase.GetAssetPath(MeshList[0].RumtimeMesh);
        int dotIndex = FileName.LastIndexOf('.');
        if (dotIndex != -1)
        {
            FileName = FileName.Substring(0, dotIndex);
        }
        FileName += ".adjacency";
        Debug.Log(FileName);

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
                    for(int i = 0; i < MeshNum; i++)
                    {
                        int TotalAdjFaceNum = Reader.ReadInt32();
                        Debug.Log("Total AdjFace Num:" + TotalAdjFaceNum);
                        MeshList[i].Triangles = new int[TotalAdjFaceNum*3];

                        int Offset = 0;
                        for (int j = 0; j < TotalAdjFaceNum; j++)
                        {
                            MeshList[i].Triangles[Offset] = Reader.ReadInt32() - 1; Offset++;
                            MeshList[i].Triangles[Offset] = Reader.ReadInt32() - 1; Offset++;
                            MeshList[i].Triangles[Offset] = Reader.ReadInt32() - 1; Offset++;
                            int xy = Reader.ReadInt32();
                            int yz = Reader.ReadInt32();
                            int zx = Reader.ReadInt32();
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

    private void OnShowAdjacency()
    {
        foreach (MeshData CurrentMeshData in MeshList)
        {
            CurrentMeshData.RumtimeMesh.triangles = CurrentMeshData.Triangles;
        }
    }

    private void OnResetMesh()
    {
        foreach (MeshData CurrentMeshData in MeshList)
        {
            CurrentMeshData.RumtimeMesh.triangles = CurrentMeshData.CachedMesh.triangles;
        }
    }
}
