using System.Collections;
using System.Collections.Generic;
using UnityEngine;


// Continous rotation with some parameters

public class Turntable : MonoBehaviour
{
	public enum RotationAxis
	{
		All,
		Y,
		X,
		Z
	}

	public RotationAxis axis;
	public float speedRot = 0.3f;

	void Update()
	{
		float rot = Time.deltaTime * speedRot;

		//Debug.Log("Axis: "+axis);

		switch (axis)
		{
			default:
			case RotationAxis.All:
				// Debug.Log("Rotating All");
				transform.Rotate(new Vector3(rot, rot, rot));
				break;

			case RotationAxis.X:
				//Debug.Log("Rotating X");
				transform.Rotate(new Vector3(rot, 0f, 0f));
				break;

			case RotationAxis.Y:
				//Debug.Log("Rotating Y");
				transform.Rotate(new Vector3(0f, rot, 0f));
				break;

			case RotationAxis.Z:
				//Debug.Log("Rotating Z");
				transform.Rotate(new Vector3(0f, 0f, rot));
				break;

		}

	}
}