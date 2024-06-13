using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public struct AdjVertex
{
    public float x;
    public float y;
    public float z;
    public uint adjSerializedId;
    public uint adjNum;

    //meshlet
    public uint IsInMeshletBorder1;
    public uint IsInMeshletBorder2;

    public static int Size()
    {
        return sizeof(float) * 3 + sizeof(uint) * 2 + sizeof(uint) * 2;
    }
}

public struct AdjFace
{
    public uint x;
    public uint y;
    public uint z;
    public uint xy;
    public uint yz;
    public uint zx;

    //meshlet
    public uint xyLayer1;
    public uint xyLayer2;
    public uint yzLayer1;
    public uint yzLayer2;
    public uint zxLayer1;
    public uint zxLayer2;

    public static int Size()
    {
        return sizeof(uint) * 6 + sizeof(uint) * 6;
    }
}

public class PackedLineData
{
    public AdjVertex[] VertexList;
    public uint[] AdjacencyVertexList;
    public AdjFace[] AdjacencyFaceList;
}


public class AdjacencyDataPool
{
    ///////////////////////////////////////////////////////////////////////////////////////////

    private static readonly AdjacencyDataPool instance = new AdjacencyDataPool();

    static AdjacencyDataPool()
    {
    }

    private AdjacencyDataPool()
    {
        AdjacencyList = new Dictionary<Guid, AdjacencyData>();
    }

    public static AdjacencyDataPool Instance
    {
        get
        {
            return instance;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// 
    class AdjacencyData
    {
        public Dictionary<string, int> NameMap;
        public List<PackedLineData> Data;
    }

    private Dictionary<Guid, AdjacencyData> AdjacencyList;

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// 
    public void Clear()
    {
        AdjacencyList.Clear();
    }
    
    public PackedLineData TryLoad(string Path, int SubMeshIndex)
    {
        Guid ID = GenerateID(Path);
        if (!AdjacencyList.ContainsKey(ID))
        {
            if (!DoLoad(Path, ID))
            {
                return null;
            }
        }

        List<PackedLineData> DataList = AdjacencyList[ID].Data;
        if (SubMeshIndex < DataList.Count && SubMeshIndex >= 0)
        {
            return DataList[SubMeshIndex];
        }

        return null;
    }

    public PackedLineData TryLoad(string Path, string SubMeshName)
    {
        Guid ID = GenerateID(Path);
        if (!AdjacencyList.ContainsKey(ID))
        {
            Debug.Log("True Load : " + Path);
            if (!DoLoad(Path, ID))
            {
                return null;
            }
        }

        AdjacencyData AdjData = AdjacencyList[ID];
        if (AdjData.NameMap.ContainsKey(SubMeshName)) { 
            int SubMeshIndex = AdjData.NameMap[SubMeshName];
            return AdjData.Data[SubMeshIndex];
        }
      
        return null;
    }

    public PackedLineData TryLoadAndGetSubMeshIndex(string Path, string SubMeshName, out int OutSubMeshIndex)
    {
        Guid ID = GenerateID(Path);
        if (!AdjacencyList.ContainsKey(ID))
        {
            Debug.Log("True Load : " + Path);
            if (!DoLoad(Path, ID))
            {
                OutSubMeshIndex = -1;
                return null;
            }
        }

        AdjacencyData AdjData = AdjacencyList[ID];
        if (AdjData.NameMap.ContainsKey(SubMeshName))
        {
            int SubMeshIndex = AdjData.NameMap[SubMeshName];
            OutSubMeshIndex = SubMeshIndex;
            return AdjData.Data[SubMeshIndex];
        }
        else
        {
            OutSubMeshIndex = -1;
        }

        return null;
    }

    private Guid GenerateID(string Source)
    {
        using (MD5 md5 = MD5.Create())
        {
            byte[] hash = md5.ComputeHash(Encoding.UTF8.GetBytes(Source));
            Guid result = new Guid(hash);
            return result;
        }
    }

    private AdjVertex[] ImportVertexData(BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        uint TotalVertexNum = Reader.ReadUInt32();
        uint PerStructSize = Reader.ReadUInt32();
        Debug.Log("Total Vertex Num & Per Struct Size: " + TotalVertexNum + ", " + PerStructSize);

        AdjVertex[] Vertices = new AdjVertex[TotalVertexNum];
        for (int j = 0; j < TotalVertexNum; j++)
        {
            AdjVertex Vertex = new AdjVertex();
            Vertex.x = Reader.ReadSingle();
            Vertex.y = Reader.ReadSingle();
            Vertex.z = Reader.ReadSingle();
            Vertex.adjSerializedId = Reader.ReadUInt32();
            Vertex.adjNum = Reader.ReadUInt32();

            Vertex.IsInMeshletBorder1 = Reader.ReadUInt32();
            Vertex.IsInMeshletBorder2 = Reader.ReadUInt32();

            Vertices[j] = Vertex;
        }

        return Vertices;
    }

    private uint[] ImportAdjacencyVertexData(BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        uint TotalAdjVertexNum = Reader.ReadUInt32();
        Debug.Log("Total AdjVertex Num: " + TotalAdjVertexNum);

        uint[] AdjVertices = new uint[TotalAdjVertexNum];
        for (int j = 0; j < TotalAdjVertexNum; j++)
        {
            AdjVertices[j] = Reader.ReadUInt32();
        }

        return AdjVertices;
    }

    private AdjFace[] ImportAdjacencyFaceData(BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        uint TotalAdjFaceNum = Reader.ReadUInt32();
        uint PerStructSize = Reader.ReadUInt32();
        Debug.Log("Total AdjFace Num & Per Struct Size: " + TotalAdjFaceNum + ", " + PerStructSize);

        AdjFace[] AdjacencyFaces = new AdjFace[TotalAdjFaceNum];
        for (int j = 0; j < TotalAdjFaceNum; j++)
        {
            AdjFace Face = new AdjFace();
            Face.x = Reader.ReadUInt32();
            Face.y = Reader.ReadUInt32();
            Face.z = Reader.ReadUInt32();
            Face.xy = Reader.ReadUInt32();
            Face.yz = Reader.ReadUInt32();
            Face.zx = Reader.ReadUInt32();

            Face.xyLayer1 = Reader.ReadUInt32();
            Face.xyLayer2 = Reader.ReadUInt32();
            Face.yzLayer1 = Reader.ReadUInt32();
            Face.yzLayer2 = Reader.ReadUInt32();
            Face.zxLayer1 = Reader.ReadUInt32();
            Face.zxLayer2 = Reader.ReadUInt32();


            AdjacencyFaces[j] = Face;
        }

        return AdjacencyFaces;
    }

    private bool DoLoad(string Path, Guid ID)
    {
        Debug.Log("Start Load Linemeta Data : " + Path);

        string FileName = Application.dataPath;
        int dotIndex = FileName.LastIndexOf('/');
        if (dotIndex != -1)
        {
            FileName = FileName.Substring(0, dotIndex+1);
        }
        FileName += Path;
        Debug.Log("Start Load Linemeta Data : " + FileName);
        if (File.Exists(FileName))
        {
            using (BinaryReader Reader = new BinaryReader(File.Open(FileName, FileMode.Open)))
            {
                uint MeshNum = Reader.ReadUInt32();
                uint ByteSizeFloat = Reader.ReadUInt32();
                uint ByteSizeUInt = Reader.ReadUInt32();
                Debug.Log("Mesh Num:" + MeshNum);
                Debug.Log("ByteSize float:" + ByteSizeFloat);
                Debug.Log("ByteSize uint:" + ByteSizeUInt);

                if (MeshNum > 0)
                {
                    AdjacencyData Current = new AdjacencyData();
                    Current.Data = new List<PackedLineData>();
                    Current.NameMap = new Dictionary<string, int>();

                    for (int i = 0; i < MeshNum; i++)
                    {
                        uint NameLength = Reader.ReadUInt32();

                        byte[] NameArray = new byte[NameLength];
                        Reader.Read(NameArray, 0, (int)NameLength);
                        string Name = Encoding.ASCII.GetString(NameArray);
                        Current.NameMap.Add(Name, i);

                        Debug.Log("Mesh " + i + ": " + Name);

                        PackedLineData LineData = new PackedLineData();
                        LineData.VertexList = ImportVertexData(Reader, ByteSizeFloat, ByteSizeUInt);
                        LineData.AdjacencyVertexList = ImportAdjacencyVertexData(Reader, ByteSizeFloat, ByteSizeUInt);
                        LineData.AdjacencyFaceList = ImportAdjacencyFaceData(Reader, ByteSizeFloat, ByteSizeUInt);

                        Current.Data.Add(LineData);

                    }
                    AdjacencyList.Add(ID, Current);

                    Debug.Log("Load Linemeta Data Completed.");
                    return true;
                }
                else
                {
                    Debug.Log("Bad Linemeta File...");
                }
            }
        }
        else
        {
            Debug.Log("Cannot Find Linemeta File.....");
        }

        Debug.Log("Load Linemeta Data Failed.");
        return false;
    }
}
