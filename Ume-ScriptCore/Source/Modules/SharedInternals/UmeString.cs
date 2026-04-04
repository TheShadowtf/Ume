using System;
using System.Globalization;

namespace Ume
{
	[VisibleToOtherModules]
	internal sealed partial class UmeString
	{
		public static string Format(string fmt, params object[] args)
		{
			return String.Format(CultureInfo.InvariantCulture.NumberFormat, fmt, args);
		}
	}
}
