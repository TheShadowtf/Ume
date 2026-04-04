using System;

namespace Ume
{
	[AttributeUsage(AttributeTargets.Field, AllowMultiple = false)]
	public sealed class RangeAttribute : Attribute
	{
		public float Min;
		public float Max;

		public RangeAttribute(float min, float max)
		{
			Min = min;
			Max = max;
		}
	}

	[AttributeUsage(AttributeTargets.Field, AllowMultiple = false)]
	public sealed class MaxValueAttribute : Attribute
	{
		public float Max;

		public MaxValueAttribute(float max)
		{
			Max = max;
		}
	}
}
