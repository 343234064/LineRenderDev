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




public class MeshData
{
    public Mesh MeshObj;
    

    public MeshData(Mesh meshObj)
    {
        this.MeshObj = meshObj;
    }

    ~MeshData()
    {
        Triangles.Dispose();
    }

    ////////////////////////////////////////////////////
    /// <Job>
    /// 
    ////////////////////////////////////////////////////
    public NativeArray<int> Triangles;
    private JobHandle WorkerHandle;

    //[BurstCompile]
    private struct InternalWorker : IJobParallelFor
    {
        [ReadOnly] public NativeArray<int> Triangles;

        public void Execute(int i)
        {
            Debug.Log(Triangles[i]);
        }
    }

    public void GenerateAdjacency(StreamWriter writer)
    {
        /*
        this.Triangles = new NativeArray<int>(this.MeshObj.triangles, Allocator.Persistent);


        InternalWorker Worker = new InternalWorker()
        {
            Triangles = this.Triangles
        };

        WorkerHandle = Worker.Schedule(this.Triangles.Length, 1);

        WorkerHandle.Complete();


        Triangles.Dispose();
        */
    }
}



public class GenerateAdjacencyEdge : ScriptableObject
{
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

    static void WirteIntToMemmoryBuffer(MemoryStream memStream, int value)
    {
        byte[] valueInBytes= BitConverter.GetBytes(value);
        memStream.Write(valueInBytes, 0, valueInBytes.Length);
        return ;
    }

    static void WirteLongToMemmoryBuffer(MemoryStream memStream, long value)
    {
        byte[] valueInBytes = BitConverter.GetBytes(value);
        memStream.Write(valueInBytes, 0, valueInBytes.Length);
        return;
    }

    [MenuItem("Tools/Create Adjacency Data")]
    static void GenerateAdjacencyData()
    {
        if (!GetReady()) return;

        String filename = AssetDatabase.GetAssetPath(MeshList[0].MeshObj);
        int dotIndex = filename.LastIndexOf('.');
        if(dotIndex != -1)
        {
            filename = filename.Substring(0, dotIndex);
        }
        filename += ".triangles";
        Debug.Log(filename);

        FileStream writer = new FileStream(filename, FileMode.Create);
        MemoryStream memStream = new MemoryStream();


        // head 
        // 4 bytes: num of mesh
        // 4 bytes: strip per element
        WirteIntToMemmoryBuffer(memStream, MeshList.Count);
        WirteIntToMemmoryBuffer(memStream, sizeof(int));

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
            // 4 bytes: num of triangles
            WirteIntToMemmoryBuffer(memStream, CurrentMeshData.MeshObj.triangles.Length);
            // elements
            byte[] result = new byte[CurrentMeshData.MeshObj.triangles.Length * sizeof(int)];
            Buffer.BlockCopy(CurrentMeshData.MeshObj.triangles, 0, result, 0, result.Length);
            memStream.Write(result, 0, result.Length);
            Debug.Log("Write Bytes length : " + result.Length);


        }

        memStream.WriteTo(writer);

        memStream.Close();
        writer.Close();

        MeshList.Clear();
    }


}
