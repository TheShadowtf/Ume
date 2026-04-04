using System;
using System.IO;
using System.Text;

namespace Ume
{
	internal static class RuntimeConsole
	{
		private sealed class EditorConsoleWriter : TextWriter
		{
			private readonly int m_Level;

			public EditorConsoleWriter(int level)
			{
				m_Level = level;
			}

			public override Encoding Encoding => Encoding.UTF8;

			public override void Write(string value)
			{
				if (!string.IsNullOrEmpty(value))
					InternalCalls.NativeLog(value, m_Level);
			}

			public override void WriteLine(string value)
			{
				InternalCalls.NativeLog(value ?? string.Empty, m_Level);
			}
		}

		private static bool s_Installed;

		public static void Install()
		{
			if (s_Installed)
				return;

			s_Installed = true;
			Console.SetOut(new EditorConsoleWriter(0));
			Console.SetError(new EditorConsoleWriter(1));
		}
	}
}
