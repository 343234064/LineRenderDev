using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;




public class PackedLineData
{
    public uint VertexNum;
    public uint RepeatedVertexNum;
    public uint VertexStructSize;
    public uint FaceNum;
    public uint FaceStructSize;
    public uint EdgeNum;
    public uint EdgeStructSize;
    public uint MeshletNum;
    public uint MeshletStructSize;

    public Byte[] RepeatedVertexList;
    public Byte[] FaceList;
    public Byte[] MeshletList;
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

    private void ImportVertexData(PackedLineData LineData, BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        LineData.VertexNum = Reader.ReadUInt32();
        LineData.RepeatedVertexNum = Reader.ReadUInt32();
        LineData.VertexStructSize = Reader.ReadUInt32();
        LineData.RepeatedVertexList = Reader.ReadBytes((int)(LineData.RepeatedVertexNum * LineData.VertexStructSize));

        Debug.Log("Total Vertex Num & Repeated Vertex Num & Per Struct Size: " + LineData.VertexNum  + "," + LineData.RepeatedVertexNum + ", " + LineData.VertexStructSize);
    }

    private void ImportFaceData(PackedLineData LineData, BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        LineData.FaceNum = Reader.ReadUInt32();
        LineData.FaceStructSize = Reader.ReadUInt32();
        LineData.FaceList = Reader.ReadBytes((int)(LineData.FaceNum * LineData.FaceStructSize));

        Debug.Log("Total Face Num & Per Struct Size: " + LineData.FaceNum + ", " + LineData.FaceStructSize);
    }

    private void ImportEdgeData(PackedLineData LineData, BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        LineData.EdgeNum = Reader.ReadUInt32();
        LineData.EdgeStructSize = Reader.ReadUInt32();

        Debug.Log("Total Edge Num & Per Struct Size: " + LineData.EdgeNum + ", " + LineData.EdgeStructSize);
    }

    private void ImportMeshletData(PackedLineData LineData, BinaryReader Reader, uint ByteSizeFloat, uint ByteSizeUInt)
    {
        LineData.MeshletNum = Reader.ReadUInt32();
        LineData.MeshletStructSize = Reader.ReadUInt32();
        LineData.MeshletList = Reader.ReadBytes((int)(LineData.MeshletNum * LineData.MeshletStructSize));

        Debug.Log("Total Meshlet Num & Per Struct Size: " + LineData.MeshletNum + ", " + LineData.MeshletStructSize);
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
                        ImportVertexData(LineData, Reader, ByteSizeFloat, ByteSizeUInt);
                        ImportFaceData(LineData, Reader, ByteSizeFloat, ByteSizeUInt);
                        ImportEdgeData(LineData, Reader, ByteSizeFloat, ByteSizeUInt);
                        ImportMeshletData(LineData, Reader, ByteSizeFloat, ByteSizeUInt);

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
