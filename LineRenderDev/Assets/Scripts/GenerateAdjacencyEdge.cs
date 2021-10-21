using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Jobs;
using Unity.Collections;
using Unity.Jobs;







public class GenerateAdjacencyEdge : ScriptableObject
{
    public class MeshData
    {
        public Mesh MeshObj;


        public MeshData(Mesh meshObj)
        {
            this.MeshObj = meshObj;
        }

        ~MeshData()
        {
        }
    }


    private static List<MeshData> MeshList;

    static bool GetReady()
    {
        GameObject Selected = Selection.activeGameObject;
        if (Selected == null)
        {
            Debug.Log("Select a mesh object first!");
            return false;
        }

        Debug.Log("Selected : " + Selected.ToString());

        if (MeshList != null) MeshList.Clear();
        else
            MeshList = new List<MeshData>();

        MeshFilter SelectedMesh = Selected.GetComponent<MeshFilter>();
        if (SelectedMesh == null)
        {
            int SubNodesNum = Selected.transform.childCount;
            if (SubNodesNum != 0)
            {
                for (int i = 0; i < SubNodesNum; i++)
                {
                    GameObject child = Selected.transform.GetChild(i).gameObject;
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


        if (MeshList.Count == 0)
        {
            Debug.Log("Please select a mesh object!");
            return false;
        }
        else
        {
            Debug.Log("Ready getting mesh : " + MeshList.Count);
            return true;
        }
    }

    static void WriteIntToMemmoryBuffer(MemoryStream memStream, int value)
    {
        byte[] valueInBytes= BitConverter.GetBytes(value);
        memStream.Write(valueInBytes, 0, valueInBytes.Length);
        return ;
    }

    static void WriteLongToMemmoryBuffer(MemoryStream memStream, long value)
    {
        byte[] valueInBytes = BitConverter.GetBytes(value);
        memStream.Write(valueInBytes, 0, valueInBytes.Length);
        return;
    }

    static void WriteVector3ToMemmoryBuffer(MemoryStream memStream, Vector3 value)
    {
        byte[] valueInBytes = new byte[sizeof(float) * 3];
        Buffer.BlockCopy(BitConverter.GetBytes(value.x), 0, valueInBytes, 0 * sizeof(float), sizeof(float));
        Buffer.BlockCopy(BitConverter.GetBytes(value.y), 0, valueInBytes, 1 * sizeof(float), sizeof(float));
        Buffer.BlockCopy(BitConverter.GetBytes(value.z), 0, valueInBytes, 2 * sizeof(float), sizeof(float));

        memStream.Write(valueInBytes, 0, valueInBytes.Length);
        return;
    }


    [MenuItem("Tools/Create Vertex and Index Data")]
    static void GenerateAdjacencyData()
    {
        if (!GetReady()) return;

        String filename = AssetDatabase.GetAssetPath(MeshList[0].MeshObj);
        int dotIndex = filename.LastIndexOf('.');
        if(dotIndex != -1)
        {
            filename = filename.Substring(0, dotIndex);
        }
        String triangle_filename = filename;
        String vertex_filename = filename;
        triangle_filename += ".triangles";
        vertex_filename += ".vertices";
        Debug.Log(triangle_filename);
        Debug.Log(vertex_filename);

        FileStream triangle_writer = new FileStream(triangle_filename, FileMode.Create);
        MemoryStream triangle_memStream = new MemoryStream();

        // head 
        // 4 bytes: num of mesh
        // 4 bytes: strip per element
        WriteIntToMemmoryBuffer(triangle_memStream, MeshList.Count);
        WriteIntToMemmoryBuffer(triangle_memStream, sizeof(int));

        foreach (MeshData CurrentMeshData in MeshList)
        {
            int[] Triangles = CurrentMeshData.MeshObj.triangles;
            int FaceNum = Triangles.Length / 3;
            int VertNum = CurrentMeshData.MeshObj.vertexCount;
            Debug.Log("Current mesh : " + CurrentMeshData.MeshObj.ToString());
            Debug.Log("Sub mesh count : " + CurrentMeshData.MeshObj.subMeshCount);
            Debug.Log("Faces : " + FaceNum);
            Debug.Log("Indices : " + CurrentMeshData.MeshObj.triangles.Length);
            Debug.Log("Vertices : " + VertNum);

            if (CurrentMeshData.MeshObj.vertexBufferCount > 1)
            {
                Debug.Log("=====WARNING Vertex buffer NUM > 1=====");
            }

            // head of block 
            // 4 bytes: num of triangle list
            WriteIntToMemmoryBuffer(triangle_memStream, CurrentMeshData.MeshObj.triangles.Length);
            // elements
            byte[] result1 = new byte[CurrentMeshData.MeshObj.triangles.Length * sizeof(int)];
            Buffer.BlockCopy(CurrentMeshData.MeshObj.triangles, 0, result1, 0, result1.Length);
            triangle_memStream.Write(result1, 0, result1.Length);
            Debug.Log("Triangle Write Bytes length : " + result1.Length);

        }

        triangle_memStream.WriteTo(triangle_writer);
        triangle_memStream.Close();
        triangle_writer.Close();

        Debug.Log("=================================================");
        FileStream vertex_writer = new FileStream(vertex_filename, FileMode.Create);
        MemoryStream vertex_memStream = new MemoryStream();

        // head 
        // 4 bytes: num of mesh
        // 4 bytes: strip per element
        WriteIntToMemmoryBuffer(vertex_memStream, MeshList.Count);
        WriteIntToMemmoryBuffer(vertex_memStream, sizeof(float));

        foreach (MeshData CurrentMeshData in MeshList)
        {
            // head of block 
            // 4 bytes: num of vertex list
            WriteIntToMemmoryBuffer(vertex_memStream, CurrentMeshData.MeshObj.vertices.Length);

            byte[] result2 = new byte[CurrentMeshData.MeshObj.vertices.Length * sizeof(float) * 3];
            // elements
            foreach (Vector3 vertex in CurrentMeshData.MeshObj.vertices)
            {
                WriteVector3ToMemmoryBuffer(vertex_memStream, vertex);
                Debug.Log(vertex.x + "," + vertex.y + "," + vertex.z);
            } 
            
            Debug.Log("Vertex Write Bytes length : " + result2.Length);
        }
        vertex_memStream.WriteTo(vertex_writer);
        vertex_memStream.Close();
        vertex_writer.Close();

        MeshList.Clear();
    }


}
