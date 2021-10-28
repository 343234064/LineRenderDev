using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LineMaterial : MonoBehaviour
{
    [TextArea]
    public string Notes = "Each line render material such be unique for a single mesh object.\nDO NOT apply a same material to multi mesh object.";

    [Tooltip("Material to render line")]
    public Material LineRenderMaterial;

    [Tooltip("Bounding volume for each line")]
    public Bounds BoundingVolume = new Bounds(Vector3.zero, Vector3.one * 20.0f);
}
