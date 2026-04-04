using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Ume;

namespace Sandbox
{
	public class PlayerController
	{
		public float maxSpeed = 3.4f;
		public float jumpHeight = 6.5f;
		public float gravityScale = 1.5f;
		public Camera mainCamera;

		bool facingRight = true;
		float moveDirection = 0;
		bool isGrounded = false;

		Vector3 cameraPos;
		Rigidbody2D r2d;

		void OnCreate()
		{
			//r2d = GetComponent<Rigidbody2D>();
		}

		void OnUpdate(float ts)
		{

		}
	}
}
