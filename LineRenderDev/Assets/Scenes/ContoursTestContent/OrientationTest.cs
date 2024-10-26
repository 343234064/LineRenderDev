using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class OrientationTest : MonoBehaviour
{
    
    public Camera RenderCamera;
    Renderer rend;

    void Start()
    {
        rend = GetComponent<Renderer>();
        rend.material.shader = Shader.Find("OrientationTest");
        if (rend.material.shader == null)
            Debug.Log("nulllll");
    }

    void Update()
    {
        Vector3 position = RenderCamera.transform.position;
        rend.material.SetVector("_CameraPosition", position);
    }
}
