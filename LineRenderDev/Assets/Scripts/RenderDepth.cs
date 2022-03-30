using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RenderDepth : MonoBehaviour
{
    public ComputeShader RenderDepthShader;

    private RenderTexture ResultDepthTexture;
    private int RenderDepthShaderKernelId;
    private float[] RenderResolution;
    private float[] ZbufferParam;

    private void Start()
    {
        RenderResolution = new float[2];
        Camera camera = GetComponent<Camera>();
        RenderResolution[0] = camera.pixelWidth;
        RenderResolution[1] = camera.pixelHeight;

        //////////////////////////////////////////////////////////////////////
        ///Reversed Z in DirectX 11, DirectX 12
        //////////////////////////////////////////////////////////////////////
        ZbufferParam = new float[4];
        //Not reverse
        //ZbufferParam[0] = (float)(1.0f - camera.farClipPlane / camera.nearClipPlane);
        //ZbufferParam[1] = (float)(camera.farClipPlane / camera.nearClipPlane);
        //ZbufferParam[2] = (float)(ZbufferParam[0] / camera.farClipPlane);
        //ZbufferParam[3] = (float)(ZbufferParam[1] / camera.farClipPlane);

        //Reverse
        ZbufferParam[0] = (float)(-1.0f + camera.farClipPlane / camera.nearClipPlane);
        ZbufferParam[1] = 1.0f;
        ZbufferParam[2] = (float)(ZbufferParam[0] / camera.farClipPlane);
        ZbufferParam[3] = (float)(1.0f / camera.farClipPlane);


        RenderDepthShaderKernelId = RenderDepthShader.FindKernel("CSMain");
        
        // turn on depth rendering for the camera so that the shader can access it via _CameraDepthTexture
        GetComponent<Camera>().depthTextureMode = DepthTextureMode.Depth;

    }

    private void OnRenderImage(RenderTexture src, RenderTexture dest)
    {
        if (ResultDepthTexture == null)
        {
            ResultDepthTexture = new RenderTexture((int)RenderResolution[0], (int)RenderResolution[1], 24);
            ResultDepthTexture.enableRandomWrite = true;
            ResultDepthTexture.Create();
        }

        /*
         *  Texture DepthTexture = Shader.GetGlobalTexture("_CameraDepthTexture");
        if (DepthTexture != null)
            Debug.Log("NOT NULL!!!!!!!!!");
        */
        RenderDepthShader.SetTexture(RenderDepthShaderKernelId, "ResultDepthTexture", ResultDepthTexture);
        RenderDepthShader.SetTextureFromGlobal(RenderDepthShaderKernelId, "DepthTexture", "_CameraDepthTexture");
        RenderDepthShader.SetFloats("Resolution", RenderResolution);
        RenderDepthShader.SetFloats("ZbufferParam", ZbufferParam);

        RenderDepthShader.Dispatch(RenderDepthShaderKernelId, ResultDepthTexture.width / 8, ResultDepthTexture.height / 8, 1);

        Graphics.Blit(ResultDepthTexture, dest);
        
    }
}