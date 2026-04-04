namespace Ume
{
	public static class Screen
	{
		public static int Width => (int)InternalCalls.Screen_GetWidth();
		public static int Height => (int)InternalCalls.Screen_GetHeight();

		public static float WorldLeft => InternalCalls.Screen_GetWorldLeft();
		public static float WorldRight => InternalCalls.Screen_GetWorldRight();
		public static float WorldBottom => InternalCalls.Screen_GetWorldBottom();
		public static float WorldTop => InternalCalls.Screen_GetWorldTop();
	}
}
