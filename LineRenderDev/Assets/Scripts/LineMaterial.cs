using Sirenix.OdinInspector;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LineMaterial : MonoBehaviour
{
    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    [TitleGroup("Base Setting")]
    [InfoBox("Each line render material such be unique for a single mesh object.\nDO NOT apply a same material to multi mesh object.")]

    [Required("Line Material Is Required")]
    [Tooltip("Material to render line")]
    public Material LineRenderMaterial;

    [Tooltip("Bounding volume for each single edge")]
    public Bounds EdgeBoundingVolume = new Bounds(Vector3.zero, Vector3.one * 20.0f);

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    [TitleGroup("Silhouette Edge")]
    [InfoBox("Silhouette edge is the edges that base on the angle between view vector and surface normal.")]

    public bool SilhouetteEnable = true;

    [Range(0, 180)]
    public float SilhouetteAngleDegreeThreshold = 60.0f;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    [TitleGroup("Crease Edge")]
    [InfoBox("Crease edge is the edges that base on the angle between adjacent faces.")]

    public bool CreaseEnable = true;

    [Range(0, 180)]
    public float CreaseAngleDegreeThreshold = 45.0f;

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    [TitleGroup("Border Edge")]
    [InfoBox("Border edge is the edges that on open face border.")]

    public bool BorderEnable = true;
}
