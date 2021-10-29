using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LineMaterial : MonoBehaviour
{
    [Header("Base Setting")]
    [TextArea]
    public string BaseNotes = "Each line render material such be unique for a single mesh object.\nDO NOT apply a same material to multi mesh object.";

    [Tooltip("Material to render line")]
    public Material LineRenderMaterial;

    [Tooltip("Bounding volume for each line")]
    public Bounds BoundingVolume = new Bounds(Vector3.zero, Vector3.one * 20.0f);


    [Header("Silhouette Edge")]
    [Space]
    [TextArea]
    public string SilhouetteNotes = "Silhouette edge is the edges that base on the angle between view vector and surface normal";
    public bool SilhouetteEnable = true;
    public float SilhouetteAngleDegreeThreshold = 60.0f;

    [Header("Crease Edge")]
    [Space]
    [TextArea]
    public string CreaseNotes = "Crease edge is the edges that base on the angle between adjacent faces";
    public bool CreaseEnable = true;
    public float CreaseAngleDegreeThreshold = 60.0f;

    [Header("Border Edge")]
    [Space]
    [TextArea]
    public string BorderNotes = "Border edge is the edges that on open face border";
    public bool BorderEnable = true;
}
