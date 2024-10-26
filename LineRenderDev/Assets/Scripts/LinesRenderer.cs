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
        public bool Available;
    }

    private class MeshInfo
    {
        public string Name;
        public int SubMeshIndex;
        public bool Available;
        public LineRuntimeContext Context;
    }

    /// ///////////////////////////////////////////////////////
    [TitleGroup("Global Shaders")]

    [AssetSelector(Filter = "Assets/")]
    [Required("Reset Pass Shader Is Required")]
    public ComputeShader ResetPassShader;

    [AssetSelector(Filter = "Assets/")]
    [Required("Extract Pass Shader Is Required")]
    public ComputeShader ExtractPassShader;

    [AssetSelector(Filter = "Assets/")]
    [Required("Contourize Pass Shader Is Required")]
    public ComputeShader ContourizePassShader;

    [AssetSelector(Paths = "Assets/")]
    [Required("Slice Pass Shader Is Required")]
    public ComputeShader SlicePassShader;

    [AssetSelector(Paths = "Assets/")]
    [Required("Visible Pass Shader Is Required")]
    public ComputeShader VisiblePassShader;

    [AssetSelector(Paths = "Assets/")]
    [Required("Generate Pass Shader Is Required")]
    public ComputeShader GeneratePassShader;

    /// ///////////////////////////////////////////////////////
    [TitleGroup("Meshes")]
    public List<MeshInfoForDisplay> MeshInfoListForOnlyDisplay;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 
    [TitleGroup("Debug")]
    [Tooltip("This wii slow down the frame rate")]
    public bool ShowDebugStatistics = false;
    public bool RenderAsDebugCamera = false;
    [Tooltip("Set main camera here if enable RenderAsDebugCamera tag")]
    public Camera MainViewCameraInDebugView;

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

            LineRuntimeContext Context = new LineRuntimeContext(LineObject.transform, MatSetting);

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
        if (RenderCamera == null)
        {
            Debug.LogError("This (Line Renderer) script must be attached to a camera object.");
        }
        RenderCamera.depthTextureMode = DepthTextureMode.Depth;
        Camera.onPreRender += OnPreRender;

        if(RenderAsDebugCamera && MainViewCameraInDebugView == null)
        {
            Debug.LogError("Please set main camera to DebugViewMainCamera if enable RenderAsDebugCamera");
        }
        if (!RenderAsDebugCamera)
            MainViewCameraInDebugView = RenderCamera;

        InitMeshList();

        LineShader InputShaders = new LineShader();
        Renderer = new RenderLayer();

        InputShaders.ResetPassShader = ResetPassShader;
        InputShaders.ExtractPassShader = ExtractPassShader;
        InputShaders.ContourizePassShader = ContourizePassShader;
        InputShaders.SlicePassShader = SlicePassShader;
        InputShaders.VisibilityPassShader = VisiblePassShader;
        InputShaders.GeneratePassShader = GeneratePassShader;
        if(!Renderer.Init(InputShaders))
            Debug.LogError("Renderer init failed, some shader is missing.");

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

            string LineMetaDataPath = MeshInfoListForRender[i].Context.LineMaterialSetting.LineMetaData;
            PackedLineData MetaData = null;
            if (Current.Context.LineMaterialSetting.SubMeshIndex != -1)
            {
                MetaData = AdjacencyDataPool.Instance.TryLoad(LineMetaDataPath, Current.Context.LineMaterialSetting.SubMeshIndex);
            }
            else
            {
                int SubMeshIndex = -1;
                MetaData = AdjacencyDataPool.Instance.TryLoadAndGetSubMeshIndex(LineMetaDataPath, Current.Name, out SubMeshIndex);
                Current.SubMeshIndex = SubMeshIndex;
            }

            if (MetaData != null)
            {
                Current.Available = true;
                Debug.Log("Load Successed :" + Current.Name);
            }
            else
            {
                Current.Available = false;
            }

            MeshInfoForDisplay Info = new MeshInfoForDisplay();
            Info.SubMeshIndex = Current.SubMeshIndex;
            Info.Name = Current.Name;
            Info.Available = Current.Available;
            MeshInfoListForOnlyDisplay.Add(Info);

            if(Current.Available)
                Current.Context.Init(MetaData);
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

        /*
            main camera ->  clipping : main camera matrix
				rendering : main camera matrix
            debug camera -> clipping : main camera matrix
				rendering : debug camera matrix
         */
        Matrix4x4 ProjectionMatrix = GL.GetGPUProjectionMatrix(RenderCamera.projectionMatrix, true);
        Matrix4x4 ViewProjectionMatrix = ProjectionMatrix * RenderCamera.worldToCameraMatrix;
        Matrix4x4 ProjectionMatrixForDebug = ProjectionMatrix;
        Matrix4x4 ViewProjectionMatrixForDebug = ViewProjectionMatrix;
        if (RenderAsDebugCamera)
        {
            ProjectionMatrixForDebug = GL.GetGPUProjectionMatrix(MainViewCameraInDebugView.projectionMatrix, true);
            ViewProjectionMatrixForDebug = ProjectionMatrixForDebug * MainViewCameraInDebugView.worldToCameraMatrix;
        }

        Matrix4x4 LineWidthScaleMatrix = ProjectionMatrix * Matrix4x4.LookAt(new Vector3(0.0f, 0.0f, 1.0f), new Vector3(0.0f, 0.0f, 0.0f), new Vector3(0.0f, 1.0f, 0.0f));
        Vector2 LineWidthScaleVec = new Vector2(
            (2.0f * LineWidthScaleMatrix[0, 0] + LineWidthScaleMatrix[0, 3]) / (2.0f * LineWidthScaleMatrix[3, 0] + LineWidthScaleMatrix[3, 3]) - (LineWidthScaleMatrix[0, 0] + LineWidthScaleMatrix[0, 3]) / ( LineWidthScaleMatrix[3, 0] + LineWidthScaleMatrix[3, 3]),
            (2.0f * LineWidthScaleMatrix[1, 0] + LineWidthScaleMatrix[1, 3]) / (2.0f * LineWidthScaleMatrix[3, 0] + LineWidthScaleMatrix[3, 3]) - (LineWidthScaleMatrix[1, 0] + LineWidthScaleMatrix[1, 3]) / (LineWidthScaleMatrix[3, 0] + LineWidthScaleMatrix[3, 3])
            );
        float LineWidthScale = LineWidthScaleVec.magnitude * 0.001f;

        foreach (MeshInfo Current in MeshInfoListForRender)
        {
            if(Current.Available)
            {
                //Debug.Log("Render: "+Current.Name);
                Renderer.ClearFrame();

                Renderer.EveryFrameParams.WorldCameraPosition = Camera.main.transform.position;
                Renderer.EveryFrameParams.LocalCameraPosition = Current.Context.RumtimeTransform.InverseTransformPoint(Camera.main.transform.position);
                Renderer.EveryFrameParams.CreaseAngleThreshold = (180.0f - Current.Context.LineMaterialSetting.CreaseAngleDegreeThreshold) * (0.017453292519943294f);
                Renderer.EveryFrameParams.WorldViewProjectionMatrix = ViewProjectionMatrix * Current.Context.RumtimeTransform.localToWorldMatrix;
                Renderer.EveryFrameParams.WorldViewProjectionMatrixForDebug = ViewProjectionMatrixForDebug * Current.Context.RumtimeTransform.localToWorldMatrix;
                Renderer.EveryFrameParams.ScreenScaledResolution = new Vector4(camera.scaledPixelWidth, camera.scaledPixelHeight, 1.0f / camera.scaledPixelWidth, 1.0f / camera.scaledPixelHeight);
                Renderer.EveryFrameParams.LineWidth = LineWidthScale * Current.Context.LineMaterialSetting.LineWidth;
                Renderer.EveryFrameParams.LineCenterOffset = Current.Context.LineMaterialSetting.LineCenterOffset;
                Renderer.EveryFrameParams.ObjectScale = Current.Context.RumtimeTransform.lossyScale;

                Renderer.Render(Current.Context, RenderAsDebugCamera);
            }
        }
    }


}
