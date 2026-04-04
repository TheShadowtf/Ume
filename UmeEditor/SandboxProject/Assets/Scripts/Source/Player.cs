using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Ume;

namespace Sandbox
{
	public class Player : Entity
	{
		private TransformComponent m_Transform;
		private Rigidbody2D m_Rigidbody;
		private Entity m_Ground;
		public Entity m_Test;

		public float Speed = 2.0f;
		public float Time = 0.0f;
		public float JumpHeight = 20.0f;
		public float Gravity = 2f;
		public float Friction = 0.9f;

		void OnCreate()
		{
			Console.WriteLine($"Player.OnCreate - {ID}");

			m_Ground = FindEntityByName("Ground");
			m_Transform = GetComponent<TransformComponent>();
			m_Rigidbody = GetComponent<Rigidbody2D>();
		}

		void OnUpdate(float ts)
		{
			Time += ts;
			//Console.WriteLine($"Player.OnUpdate: {ts}");

			//if (m_Transform.Translation.X < m_Ground.Translation.X + m_Ground.GetComponent<TransformComponent>().Scale.X)
			//{
			//
			//}

			Vector3 velocity = Vector3.zero;
			Vector3 ground = m_Ground.GetComponent<TransformComponent>().Translation;
			float translation = m_Transform.Translation.X;
			float distance = (float)Math.Sqrt((-3.67 - 0) * 2 + (-0.89 - -2.18) * 2);

			if (Input.IsKeyDown(KeyCode.Space))
				velocity.Y = JumpHeight;
			else if (Input.IsKeyDown(KeyCode.S))
				velocity.Y = -JumpHeight;

			if (Input.IsKeyDown(KeyCode.A))
				velocity.X = -Speed;
			else if (Input.IsKeyDown(KeyCode.D))
				velocity.X = Speed;

			if (Input.IsKeyDown(KeyCode.G))
				m_Transform.Translation = new Vector3(5.0f, 0.0f, 0.0f);

			velocity *= Speed * Friction * ts;
			velocity.Y *= Gravity;

			m_Rigidbody.ApplyLinearImpulse(new Vector2(velocity.X, velocity.Y), true);

			//Vector3 translation = m_Transform.Translation;
			//translation += velocity * ts;
			//m_Transform.Translation = translation;
		}

	}
}
