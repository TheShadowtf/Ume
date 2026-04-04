using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Ume;

namespace Sandbox
{
	public class Ground : Entity
	{
		public float Timer = 0.0f;
		public float Test1 = 1.0f;
		public float Test2 = 1.0f;

		void OnCreate()
		{
			Console.WriteLine($"Ground.OnCreate - {ID}");

			Console.WriteLine("Ground created");
		}

		void OnUpdate(float ts)
		{

		}

	}
}
