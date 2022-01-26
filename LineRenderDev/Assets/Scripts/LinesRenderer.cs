using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;

using Sirenix.OdinInspector;



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
        Camera MainCamera = Camera.main;
        for (int i = 0; i < MeshList.Count; i++)
        {
            if (MeshList[i].LineMaterialSetting != null && MeshList[i].RumtimeTransform.gameObject.activeSelf)
            {
                ExtractPassParams.LocalCameraPosition = MeshList[i].RumtimeTransform.InverseTransformPoint(Camera.main.transform.position);
                ExtractPassParams.CreaseAngleThreshold = (180.0f - MeshList[i].LineMaterialSetting.CreaseAngleDegreeThreshold) * (0.017453292519943294f);
                MeshList[i].Renderer.ExtractPass.Render(ExtractPassParams);


#if UNITY_STANDALONE_WIN || UNITY_EDITOR_WIN
                //Reverse Z in DirectX 11, DirectX 12
                VisibilityPassParams.ZbufferParam[0] = (float)(-1.0f + MainCamera.farClipPlane / MainCamera.nearClipPlane);
                VisibilityPassParams.ZbufferParam[1] = 1.0f;
                VisibilityPassParams.ZbufferParam[2] = (float)(VisibilityPassParams.ZbufferParam[0] / MainCamera.farClipPlane);
                VisibilityPassParams.ZbufferParam[3] = (float)(1.0f / MainCamera.farClipPlane);
#else
                VisibilityPassParams.ZbufferParam[0] = (float)(1.0f - camera.farClipPlane / camera.nearClipPlane);
                VisibilityPassParams.ZbufferParam[1] = (float)(camera.farClipPlane / camera.nearClipPlane);
                VisibilityPassParams.ZbufferParam[2] = (float)(ZbufferParam[0] / camera.farClipPlane);
                VisibilityPassParams.ZbufferParam[3] = (float)(ZbufferParam[1] / camera.farClipPlane);
#endif
                VisibilityPassParams.RenderResolutionX = MainCamera.pixelWidth;
                VisibilityPassParams.RenderResolutionY = MainCamera.pixelHeight;
                MeshList[i].Renderer.VisibilityPass.Render(VisibilityPassParams);

                MaterialPassParams.ObjectWorldMatrix = MeshList[i].RumtimeTransform.localToWorldMatrix;
                MeshList[i].Renderer.MaterialPass.Render(MaterialPassParams);
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

    [AssetSelector(Paths = "Assets/Shaders")]
    [Required("Visible Line Shader Is Required")]
    public ComputeShader VisibleLineShader;
    /// ///////////////////////////////////////////////////////
    private List<RenderMeshContext> MeshList;
   
    private RenderExtractPass.RenderParams ExtractPassParams;
    private RenderMaterialPass.RenderParams MaterialPassParams;
    private RenderVisibilityPass.RenderParams VisibilityPassParams;
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
                            ToAdd.LineMaterialSetting.LineRenderMaterial.renderQueue = (int)RenderQueue.Geometry + 1;
                        }
                        else
                        {
                            ToAdd.LineMaterialSetting = null;
                            Debug.Log("=================Mesh: " + child.name + " has NO Line Material applied, the mesh will be ignored=================");
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
                Debug.Log("=================Mesh: " + gameObject.name + " has NO Line Material applied, the mesh will be ignored=================");
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
        ExtractPassParams = new RenderExtractPass.RenderParams();
        VisibilityPassParams = new RenderVisibilityPass.RenderParams();
        MaterialPassParams = new RenderMaterialPass.RenderParams();

        VisibilityPassParams.ZbufferParam = new float[4];


        for (int i = 0; i < MeshList.Count; i++)
        {
            if (!MeshList[i].RumtimeTransform.gameObject.activeSelf) continue;

            MeshList[i].Renderer.Init(ExtractLineShader, VisibleLineShader);
            MeshList[i].Renderer.InitInputOutputBuffer(MeshList[i].AdjacencyTriangles.Length);

            MeshList[i].Renderer.ExtractPass.SetAdjacencyIndicesBuffer(MeshList[i].AdjacencyTriangles.Length, MeshList[i].AdjacencyTriangles);
            MeshList[i].Renderer.ExtractPass.SetVerticesBuffer(MeshList[i].RumtimeMesh.vertices.Length, MeshList[i].RumtimeMesh.vertices);

            RenderExtractPass.RenderConstants[] Pass1Constants = new RenderExtractPass.RenderConstants[1];
            Pass1Constants[0].TotalAdjacencyTrianglesNum = (uint)MeshList[i].AdjacencyTriangles.Length;
            Pass1Constants[0].SilhouetteEnable = (uint)(MeshList[i].LineMaterialSetting.SilhouetteEnable ? 1 : 0);
            Pass1Constants[0].CreaseEnable = (uint)(MeshList[i].LineMaterialSetting.CreaseEnable ? 1 : 0);
            Pass1Constants[0].BorderEnable = (uint)(MeshList[i].LineMaterialSetting.BorderEnable ? 1 : 0);
            Pass1Constants[0].HideBackFaceEdge = (uint)(MeshList[i].LineMaterialSetting.HideBackFaceEdge ? 1 : 0);
            MeshList[i].Renderer.ExtractPass.SetConstantBuffer(Pass1Constants);

            RenderVisibilityPass.RenderConstants[] Pass2Constants = new RenderVisibilityPass.RenderConstants[1];
            Pass2Constants[0].HideOccludedEdge = (uint)(MeshList[i].LineMaterialSetting.HideOccludedEdge ? 1 : 0);
            MeshList[i].Renderer.VisibilityPass.SetConstantBuffer(Pass2Constants);

            MeshList[i].Renderer.MaterialPass.SetLineMaterialObject(MeshList[i].LineMaterialSetting);

            
        }

    }

   

}
