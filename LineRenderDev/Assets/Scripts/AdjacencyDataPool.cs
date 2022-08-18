using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;
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
    struct AdjacencyData
    {
        public Dictionary<string, int> NameMap;
        public List<AdjFace[]> Data;
    }

    private Dictionary<Guid, AdjacencyData> AdjacencyList;

    ///////////////////////////////////////////////////////////////////////////////////////////
    /// 
    public void Clear()
    {
        AdjacencyList.Clear();
    }
    
    public AdjFace[] TryLoad(string Path, int SubMeshIndex)
    {
        Guid ID = GenerateID(Path);
        if (!AdjacencyList.ContainsKey(ID))
        {
            if (!DoLoad(Path, ID))
            {
                return null;
            }
        }

        List<AdjFace[]> Adjacency = AdjacencyList[ID].Data;
        if (SubMeshIndex < Adjacency.Count && SubMeshIndex >= 0)
        {
            return Adjacency[SubMeshIndex];
        }

        return null;
    }

    public AdjFace[] TryLoad(string Path, string SubMeshName)
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

    public AdjFace[] TryLoadAndGetSubMeshIndex(string Path, string SubMeshName, out int OutSubMeshIndex)
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

    private bool DoLoad(string Path, Guid ID)
    {
        Debug.Log("Start Load Adjacency : " + Path);

        string FileName = Application.dataPath;
        int dotIndex = FileName.LastIndexOf('/');
        if (dotIndex != -1)
        {
            FileName = FileName.Substring(0, dotIndex+1);
        }
        FileName += Path;
        Debug.Log("Start Load Adjacency : " + FileName);
        if (File.Exists(FileName))
        {
            using (BinaryReader Reader = new BinaryReader(File.Open(FileName, FileMode.Open)))
            {
                int MeshNum = Reader.ReadInt32();
                int OffsetPerData = Reader.ReadInt32();
                Debug.Log("Mesh Num:" + MeshNum);
                Debug.Log("Offset Per Data:" + OffsetPerData);

                if (MeshNum > 0)
                {
                    AdjacencyData Current = new AdjacencyData();
                    Current.Data = new List<AdjFace[]>();
                    Current.NameMap = new Dictionary<string, int>();

                    for (int i = 0; i < MeshNum; i++)
                    {

                        int NameLength = Reader.ReadInt32();

                        byte[] NameArray = new byte[NameLength];
                        Reader.Read(NameArray, 0, NameLength);
                        string Name = Encoding.ASCII.GetString(NameArray);
                        Current.NameMap.Add(Name, i);

                        Debug.Log("Mesh " + i + ": " + Name);

                        int TotalAdjFaceNum = Reader.ReadInt32();
                        Debug.Log("Total AdjFace Num:" + TotalAdjFaceNum);

                        AdjFace[] AdjacencyTriangles = new AdjFace[TotalAdjFaceNum];
                        for (int j = 0; j < TotalAdjFaceNum; j++)
                        {
                            AdjFace Face = new AdjFace();
                            Face.x = (uint)Reader.ReadInt32();
                            Face.y = (uint)Reader.ReadInt32();
                            Face.z = (uint)Reader.ReadInt32();
                            Face.xy = (uint)Reader.ReadInt32();
                            Face.yz = (uint)Reader.ReadInt32();
                            Face.zx = (uint)Reader.ReadInt32();
                            AdjacencyTriangles[j] = Face;
                        }
                        Current.Data.Add(AdjacencyTriangles);

                    }
                    AdjacencyList.Add(ID, Current);

                    Debug.Log("Load Adjacency Completed.");
                    return true;
                }
                else
                {
                    Debug.Log("Bad Adjacency File...");
                }
            }
        }
        else
        {
            Debug.Log("Cannot Find Adjacency File.....");
        }

        Debug.Log("Load Adjacency Failed.");
        return false;
    }
}
