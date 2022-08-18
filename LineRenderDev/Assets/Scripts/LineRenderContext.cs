using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;



public class LineContext
{
    public Mesh RumtimeMesh;
    public LineMaterial LineMaterialSetting;
    public Transform RumtimeTransform;

    private AdjFace[] AdjacencyTriangles;

    public LineContext(Mesh meshObj, Transform transform, LineMaterial material)
    {
        RumtimeMesh = meshObj;
        RumtimeTransform = transform;
        LineMaterialSetting = material;

        AdjacencyTriangles = null;
    }

    public bool Init(AdjFace[] AdjTris)
    {
        if (AdjTris == null)
            return false;

        AdjacencyTriangles = AdjTris;

        return true;
    }

    public void Destroy()

    {
    }

}


public class RenderShader
{
    public ComputeShader ExtractPassShader;
    public ComputeShader VisibilityPassShader;

    public bool IsValid()
    {
        if (ExtractPassShader == null)
            return false;
        else if (VisibilityPassShader == null)
            return false;

        return true;
    }
}

public class RenderPass
{
    public virtual bool Init() { return false; }
    public virtual void Render() { }
    public virtual void Destroy() { }
    public virtual void SetShader() {}
}

public class RenderLayer
{
    private bool Available;
    private List<RenderPass> PassList;
    private CommandBuffer RenderCommands;
   
    public bool Init(RenderShader InputShaders)
    {
        Available = false;
        if (!InputShaders.IsValid())
            return false;

        RenderCommands = new CommandBuffer();
        RenderCommands.name = "Draw Lines";

        PassList = new List<RenderPass>();


        Available = true;
        Debug.Log("Render Layer Init");

        return true;
    }

    public void Destroy()
    {
        Debug.Log("Render Layer Destroy");

        if(RenderCommands != null)
            RenderCommands.Release();
        if (PassList != null)
        {
            foreach (RenderPass Pass in PassList)
                Pass.Destroy();
            PassList.Clear();
        }
        Available = false;
    }

    public void Render()
    {
        if (!Available)
            return;

    }

    public CommandBuffer GetRenderCommand()
    {
        return RenderCommands;
    }

    public void SetConstants()
    {

    }

    public void SetVariables()
    {

    }

}

