using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LineMaterial : MonoBehaviour
{
    [TextArea]
    public string Notes = "Each material such be unique for a single mesh object.\nDO NOT apply a same material to multi mesh object.";
    public Material LineRenderMaterial;


}
