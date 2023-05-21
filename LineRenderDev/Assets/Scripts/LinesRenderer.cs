using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEditor;
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
    [Required("Extract Pass Shader Is Required")]
    public ComputeShader ExtractPassShader;

    [AssetSelector(Paths = "Assets/")]
    [Required("Slice Pass Shader Is Required")]
    public ComputeShader SlicePassShader;

    [AssetSelector(Paths = "Assets/")]
    [Required("Visible Pass Shader Is Required")]
    public ComputeShader VisiblePassShader;


    /// ///////////////////////////////////////////////////////
    [TitleGroup("Meshes")]
    public List<MeshInfoForDisplay> MeshInfoListForOnlyDisplay;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 
    [TitleGroup("Debug")]
    [Tooltip("This wii slow down the frame rate")]
    public bool ShowDebugStatistics = false;

    /// ////////////////////////////////////////////////////////
    private Camera RenderCamera;
    private Camera SceneEditorCamera;
    private Camera GamerCamera;

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

        SceneView EditorSceneView = SceneView.lastActiveSceneView;
        SceneEditorCamera = EditorSceneView.camera;
        GamerCamera = GetComponent<Camera>();

        RenderCamera = GamerCamera;
        // Debug clipping
        // RenderCamera = SceneEditorCamera;

        if (GamerCamera == null)
        {
            Debug.LogError("This (Line Renderer) script must be attached to a camera object.");
        }
        RenderCamera.depthTextureMode = DepthTextureMode.Depth;
        Camera.onPreRender += OnPreRender;

        InitMeshList();

        LineShader InputShaders = new LineShader();
        Renderer = new RenderLayer();

        InputShaders.ExtractPassShader = ExtractPassShader;
        InputShaders.SlicePassShader = SlicePassShader;
        InputShaders.VisibilityPassShader = VisiblePassShader;
        Renderer.Init(InputShaders);

        RenderCamera.AddCommandBuffer(CameraEvent.AfterForwardAlpha, Renderer.GetRenderCommand());
    }

    void Start()
    {
        Array2<int> ScreenResolution = new Array2<int>(Screen.currentResolution.width, Screen.currentResolution.height);
       for (int i = 0; i<MeshInfoListForRender.Count; i++)
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
        Matrix4x4 ViewProjectionMatrixForClipping = GL.GetGPUProjectionMatrix(GamerCamera.projectionMatrix, true) * GamerCamera.worldToCameraMatrix;
        foreach (MeshInfo Current in MeshInfoListForRender)
        {
            if(Current.Available)
            {
                //Debug.Log("Render: "+Current.Name);
                Renderer.ClearFrame();

                Renderer.EveryFrameParams.LocalCameraPosition = Current.Context.RumtimeTransform.InverseTransformPoint(Camera.main.transform.position);
                Renderer.EveryFrameParams.CreaseAngleThreshold = (180.0f - Current.Context.LineMaterialSetting.CreaseAngleDegreeThreshold) * (0.017453292519943294f);
                Renderer.EveryFrameParams.WorldViewProjectionMatrix = ViewProjectionMatrix * Current.Context.RumtimeTransform.localToWorldMatrix;
                Renderer.EveryFrameParams.WorldViewProjectionMatrixForClipping = ViewProjectionMatrixForClipping * Current.Context.RumtimeTransform.localToWorldMatrix;
                Renderer.EveryFrameParams.ScreenWidthScaled = camera.scaledPixelWidth;
                Renderer.EveryFrameParams.ScreenHeightScaled = camera.scaledPixelHeight;
                Renderer.EveryFrameParams.ScreenWidthFixed = Screen.currentResolution.width;
                Renderer.EveryFrameParams.ScreenHeightFixed = Screen.currentResolution.height;

                Renderer.Render(Current.Context);
            }
        }
    }


}
