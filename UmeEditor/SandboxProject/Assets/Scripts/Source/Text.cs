using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Ume;

namespace Sandbox
{
	public class Text : Entity
	{
		private TextComponent m_TextComponent;
		public float floatTest = 0.0f;
		public double doubeTest = 0.0d;
		public bool boolTest = false;
		public char charTest = 'a';
		public short shortTest = 1;
		public int intTest = 1;
		public long longTest = 1;
		public uint uintTest = 1;
		public ulong ulongTest = 1;
		public String stringTest;
		public Vector2 vector2Tes = new Vector2();
		public Vector3 vector3Tes = new Vector3();
		public Vector4 vector4Tes = new Vector4();
		public ColorPicker colorPickerTes = new ColorPicker();

		void OnCreate()
		{
			m_TextComponent = GetComponent<TextComponent>();
			m_TextComponent.Text = "Test" + stringTest;
			m_TextComponent.Color = new Vector4(colorPickerTes);
			Console.WriteLine(colorPickerTes.X);
		}

		void OnUpdate(float ts)
		{
			if (Input.IsKeyDown(KeyCode.Space))
				boolTest = true;
			if (Input.IsKeyDown(KeyCode.Backspace))
				boolTest = false;
			if (boolTest)
				Console.WriteLine(intTest);
		}
	}
}
