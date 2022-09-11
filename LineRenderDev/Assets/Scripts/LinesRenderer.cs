using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

using Sirenix.OdinInspector;

//[ExecuteAlways]
[RequireComponent(typeof(Camera))]
public class LinesRenderer : MonoBehaviour
{
    [Serializable]
    public struct MeshInfoForDisplay
    {
        public string Name;
        //[FoldoutGroup("$Name", false)]
        public int SubMeshIndex;
        //[FoldoutGroup("$Name", false)]
        public int AdjFaceCount;
        public bool Available;
    }

    private class MeshInfo
    {
        public string Name;
        public int SubMeshIndex;
        public bool Available;
        public LineContext Context;
    }

    /// ///////////////////////////////////////////////////////
    [TitleGroup("Global Shaders")]

    [AssetSelector(Filter = "Assets/")]
    [Required("Extract Line Shader Is Required")]
    public ComputeShader ExtractLineShader;

    [AssetSelector(Paths = "Assets/")]
    [Required("Visible Line Shader Is Required")]
    public ComputeShader VisibleLineShader;


    /// ///////////////////////////////////////////////////////
    [TitleGroup("Meshes")]
    public List<MeshInfoForDisplay> MeshInfoListForOnlyDisplay;

    /// ////////////////////////////////////////////////////////
    private Camera RenderCamera;
    private List<MeshInfo> MeshInfoListForRender;
    private RenderLayer Renderer;




    private void InitMeshList()
    {
        MeshInfoListForOnlyDisplay.Clear();
        MeshInfoListForRender = new List<MeshInfo>();
        LineMaterial[] LineObjectMaterialList = UnityEngine.Object.FindObjectsOfType<LineMaterial>();

        foreach (LineMaterial LineObjectMaterial in LineObjectMaterialList)
        {
            GameObject LineObject = LineObjectMaterial.gameObject;

            MeshFilter SubMesh = LineObject.GetComponent<MeshFilter>();
            if (SubMesh == null)
            {
                Debug.LogWarning("A object will be ignored in Line Drawing because it isn't has a MeshFilter component.", LineObject);
                continue;
            }

            LineMaterial MatSetting = LineObject.GetComponent<LineMaterial>();
            MatSetting.LineRenderMaterial.renderQueue = (int)RenderQueue.Geometry + 1;

            LineContext Context = new LineContext(SubMesh.sharedMesh, LineObject.transform, MatSetting);

            MeshInfo ToAdd = new MeshInfo();
            ToAdd.Context = Context;
            ToAdd.Name = LineObject.name;
            ToAdd.SubMeshIndex = MatSetting.SubMeshIndex;
            ToAdd.Available = false;

            MeshInfoListForRender.Add(ToAdd);
        }
    }


    void OnEnable()
    {
        Debug.Log("Line Renderer Enabled.");

        RenderCamera = GetComponent<Camera>();
        if(RenderCamera == null)
        {
            Debug.LogError("This (Line Renderer) script must be attached to a camera object.");
        }
        RenderCamera.depthTextureMode = DepthTextureMode.Depth;
        Camera.onPreRender += OnPreRender;

        InitMeshList();

        LineShader InputShaders = new LineShader();
        Renderer = new RenderLayer();

        InputShaders.ExtractPassShader = ExtractLineShader;
        InputShaders.VisibilityPassShader = VisibleLineShader;
        Renderer.Init(InputShaders);

        RenderCamera.AddCommandBuffer(CameraEvent.AfterForwardAlpha, Renderer.GetRenderCommand());
    }

    void Start()
    {
       for(int i = 0; i<MeshInfoListForRender.Count; i++)
        {
           MeshInfo Current = MeshInfoListForRender[i];
            if (!Current.Context.RumtimeTransform.gameObject.activeSelf)
            {
                Current.Available = false;
                continue;
            }

            string AdjacencyPath = MeshInfoListForRender[i].Context.LineMaterialSetting.AdjacencyData;
            AdjFace[] AdjacencyTriangles = null;
            if (Current.Context.LineMaterialSetting.SubMeshIndex != -1)
            {
                AdjacencyTriangles = AdjacencyDataPool.Instance.TryLoad(AdjacencyPath, Current.Context.LineMaterialSetting.SubMeshIndex);
            }
            else
            {
                int SubMeshIndex = -1;
                AdjacencyTriangles = AdjacencyDataPool.Instance.TryLoadAndGetSubMeshIndex(AdjacencyPath, Current.Name, out SubMeshIndex);
                Current.SubMeshIndex = SubMeshIndex;
            }

            int AdjFaceCount = 0;
            if (AdjacencyTriangles != null)
            {
                Current.Available = true;
                AdjFaceCount = AdjacencyTriangles.Length;
                Debug.Log("Load Successed :" + Current.Name);
            }
            else
            {
                Current.Available = false;
            }

            MeshInfoForDisplay Info = new MeshInfoForDisplay();
            Info.SubMeshIndex = Current.SubMeshIndex;
            Info.AdjFaceCount = AdjFaceCount;
            Info.Name = Current.Name;
            Info.Available = Current.Available;
            MeshInfoListForOnlyDisplay.Add(Info);

            if(Current.Available)
                Current.Context.Init(AdjacencyTriangles);
        }


    }


    void OnDisable()
    {
        Debug.Log("Line Renderer Disabled.");

        foreach(MeshInfo Current in MeshInfoListForRender)
        {
            if (Current.Available)
                Current.Context.Destroy();
        }

        MeshInfoListForOnlyDisplay.Clear();
        MeshInfoListForRender.Clear();

        RenderCamera.RemoveCommandBuffer(CameraEvent.AfterForwardAlpha, Renderer.GetRenderCommand());
        Renderer.Destroy();

        Camera.onPreRender -= OnPreRender;
    }

    void OnPreRender(Camera camera)
    {
        Renderer.ClearCommandBuffer();

        Matrix4x4 ViewProjectionMatrix = GL.GetGPUProjectionMatrix(RenderCamera.projectionMatrix, true) * RenderCamera.worldToCameraMatrix;
        foreach (MeshInfo Current in MeshInfoListForRender)
        {
            if(Current.Available)
            {
                Debug.Log("Render: "+Current.Name);
                Renderer.ClearFrame();

                Renderer.EveryFrameParams.LocalCameraPosition = Current.Context.RumtimeTransform.InverseTransformPoint(Camera.main.transform.position);
                Renderer.EveryFrameParams.CreaseAngleThreshold = (180.0f - Current.Context.LineMaterialSetting.CreaseAngleDegreeThreshold) * (0.017453292519943294f);
                Renderer.EveryFrameParams.WorldViewProjectionMatrix = ViewProjectionMatrix * Current.Context.RumtimeTransform.localToWorldMatrix;
                Renderer.EveryFrameParams.WorldViewMatrix = RenderCamera.worldToCameraMatrix * Current.Context.RumtimeTransform.localToWorldMatrix;
                Renderer.EveryFrameParams.ObjectWorldMatrix = Current.Context.RumtimeTransform.localToWorldMatrix;

                Renderer.Render(Current.Context);
            }
        }
    }


}
