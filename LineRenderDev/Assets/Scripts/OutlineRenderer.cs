using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;

public class OutlineRenderer : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        GetMeshInformation();
        LoadAdjacency();
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public class MeshData
    {
        public Mesh RumtimeMesh;
        public int[] AdjacencyTriangles;

        public MeshData(Mesh meshObj)
        {
            RumtimeMesh = meshObj;
        }

    }


    private List<MeshData> MeshList;

    private static string GetGameObjectPath(Transform transform)
    {
        string path = transform.name;
        while (transform.parent != null)
        {
            transform = transform.parent;
            path = transform.name + "/" + path;
        }
        return path;
    }

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
                        MeshList[i].AdjacencyTriangles = new int[TotalAdjFaceNum * 6];
                        
                        int Offset = 0;
                        for (int j = 0; j < TotalAdjFaceNum; j++)
                        {
                            MeshList[i].AdjacencyTriangles[Offset] = Reader.ReadInt32(); Offset++;
                            MeshList[i].AdjacencyTriangles[Offset] = Reader.ReadInt32(); Offset++;
                            MeshList[i].AdjacencyTriangles[Offset] = Reader.ReadInt32(); Offset++;

                            MeshList[i].AdjacencyTriangles[Offset] = Reader.ReadInt32(); Offset++;
                            MeshList[i].AdjacencyTriangles[Offset] = Reader.ReadInt32(); Offset++;
                            MeshList[i].AdjacencyTriangles[Offset] = Reader.ReadInt32(); Offset++;

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
}
