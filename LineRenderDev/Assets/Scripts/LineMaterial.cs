using Sirenix.OdinInspector;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;


//[ExecuteInEditMode]
public class LineMaterial : MonoBehaviour
{
    [FilePath]
    [BoxGroup("Source")]
    public string LineMetaData = "None";

    [BoxGroup("Source")]
    [Tooltip("Which linemeta data will be used (If has multiple data for different sub mesh in the source)， -1 means auto.")]
    public int SubMeshIndex = -1;
    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    [TitleGroup("Base Setting")]
    [InfoBox("Each line render material such be unique for a single mesh object.\nDO NOT apply a same material to multi mesh object.")]

    [Required("Line Material Is Required")]
    [Tooltip("Material to render line")]
    public Material LineRenderMaterial;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    [TitleGroup("Visibility")]

    [Tooltip("Hide edge that visible")]
    public bool HideVisibleEdge = false;

    [Tooltip("Hide edge that lies on back face")]
    public bool HideBackFaceEdge = true;

    [Tooltip("Hide edge that is occluded by other face")]
    public bool HideOccludedEdge = true;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    [TitleGroup("Chain")]

    [Tooltip("Link unvisible and visible edges when showing both.")]
    [LabelWidth(200)]
    public bool LinkUnvisibleAndVisibleEdge = false;

    [Tooltip("Edges whose angles to each other are below this threshold will be unlinked.")]
    [Range(0, 180)]
    [LabelWidth(200)]
    public float UnlinkedEdgeAngleThreshold = 90.0f;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    [TitleGroup("Silhouette Edge")]
    [InfoBox("Draw edges that base on the angle between view vector and surface normal.")]

    public bool SilhouetteEnable = true;

    [Tooltip("Draw edges around each object only(Contour Only).")]
    public bool OutineOnly = false;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    [TitleGroup("Crease Edge")]
    [InfoBox("Draw edges that base on the angle between adjacent faces.")]

    public bool CreaseEnable = true;

    [Range(0, 180)]
    [LabelWidth(200)]
    public float CreaseAngleDegreeThreshold = 45.0f;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    [TitleGroup("Border Edge")]
    [InfoBox("Draw edges that on open face border.")]
    public bool BorderEnable = true;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    [TitleGroup("Marked Edge")]
    [InfoBox("Draw edges that marked by user.")]
    public bool MarkedEdgeEnable = true;


    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    void Awake()
    {
        
    }
}
