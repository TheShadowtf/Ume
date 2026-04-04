using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System;
using Ume;

namespace Ume
{
	// Representation of 2D vectors and points.
	[StructLayout(LayoutKind.Sequential)]
	[NativeClass("Vector2Int")]
	public struct Vector2Int : IEquatable<Vector2Int>, IFormattable
	{
		public int X
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return m_X; }

			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			set { m_X = value; }
		}


		public int Y
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return m_Y; }

			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			set { m_Y = value; }
		}

		private int m_X;
		private int m_Y;

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector2Int(int x, int y)
		{
			m_X = x;
			m_Y = y;
		}

		// Set x and y components of an existing Vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Set(int x, int y)
		{
			m_X = x;
			m_Y = y;
		}

		// Access the /x/ or /y/ component using [0] or [1] respectively.
		public int this[int index]
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				switch (index)
				{
					case 0: return X;
					case 1: return Y;
					default:
						throw new IndexOutOfRangeException(String.Format("Invalid Vector2Int index addressed: {0}!", index));
				}
			}

			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			set
			{
				switch (index)
				{
					case 0: X = value; break;
					case 1: Y = value; break;
					default:
						throw new IndexOutOfRangeException(String.Format("Invalid Vector2Int index addressed: {0}!", index));
				}
			}
		}

		// Returns the length of this vector (RO).
		public float magnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return Mathf.Sqrt((float)(X * X + Y * Y)); } }

		// Returns the squared length of this vector (RO).
		public int sqrMagnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return X * X + Y * Y; } }

		// Returns the distance between /a/ and /b/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Distance(Vector2Int a, Vector2Int b)
		{
			float diff_x = a.X - b.X;
			float diff_y = a.Y - b.Y;

			return (float)Math.Sqrt(diff_x * diff_x + diff_y * diff_y);
		}

		// Returns a vector that is made from the smallest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int Min(Vector2Int lhs, Vector2Int rhs) { return new Vector2Int(Mathf.Min(lhs.X, rhs.X), Mathf.Min(lhs.Y, rhs.Y)); }

		// Returns a vector that is made from the largest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int Max(Vector2Int lhs, Vector2Int rhs) { return new Vector2Int(Mathf.Max(lhs.X, rhs.X), Mathf.Max(lhs.Y, rhs.Y)); }

		// Multiplies two vectors component-wise.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int Scale(Vector2Int a, Vector2Int b) { return new Vector2Int(a.X * b.X, a.Y * b.Y); }

		// Multiplies every component of this vector by the same component of /scale/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Scale(Vector2Int scale) { X *= scale.X; Y *= scale.Y; }

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Clamp(Vector2Int min, Vector2Int max)
		{
			X = Math.Max(min.X, X);
			X = Math.Min(max.X, X);
			Y = Math.Max(min.Y, Y);
			Y = Math.Min(max.Y, Y);
		}

		// Converts a Vector2Int to a [[Vector2]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector2(Vector2Int v)
		{
			return new Vector2(v.X, v.Y);
		}

		// Converts a Vector2Int to a [[Vector3Int]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static explicit operator Vector3Int(Vector2Int v)
		{
			return new Vector3Int(v.X, v.Y, 0);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int FloorToInt(Vector2 v)
		{
			return new Vector2Int(
				Mathf.FloorToInt(v.X),
				Mathf.FloorToInt(v.Y)
			);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int CeilToInt(Vector2 v)
		{
			return new Vector2Int(
				Mathf.CeilToInt(v.X),
				Mathf.CeilToInt(v.Y)
			);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int RoundToInt(Vector2 v)
		{
			return new Vector2Int(
				Mathf.RoundToInt(v.X),
				Mathf.RoundToInt(v.Y)
			);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator -(Vector2Int v)
		{
			return new Vector2Int(-v.X, -v.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator +(Vector2Int a, Vector2Int b)
		{
			return new Vector2Int(a.X + b.X, a.Y + b.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator -(Vector2Int a, Vector2Int b)
		{
			return new Vector2Int(a.X - b.X, a.Y - b.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator *(Vector2Int a, Vector2Int b)
		{
			return new Vector2Int(a.X * b.X, a.Y * b.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator *(int a, Vector2Int b)
		{
			return new Vector2Int(a * b.X, a * b.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator *(Vector2Int a, int b)
		{
			return new Vector2Int(a.X * b, a.Y * b);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2Int operator /(Vector2Int a, int b)
		{
			return new Vector2Int(a.X / b, a.Y / b);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator ==(Vector2Int lhs, Vector2Int rhs)
		{
			return lhs.X == rhs.X && lhs.Y == rhs.Y;
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator !=(Vector2Int lhs, Vector2Int rhs)
		{
			return !(lhs == rhs);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override bool Equals(object other)
		{
			if (!(other is Vector2Int)) return false;

			return Equals((Vector2Int)other);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public bool Equals(Vector2Int other)
		{
			return X == other.X && Y == other.Y;
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override int GetHashCode()
		{
			return X.GetHashCode() ^ (Y.GetHashCode() << 2);
		}

		/// *listonly*
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override string ToString()
		{
			return ToString(null, null);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public string ToString(string format)
		{
			return ToString(format, null);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public string ToString(string format, IFormatProvider formatProvider)
		{
			if (formatProvider == null)
				formatProvider = CultureInfo.InvariantCulture.NumberFormat;
			return UmeString.Format("({0}, {1})", X.ToString(format, formatProvider), Y.ToString(format, formatProvider));
		}

		public static Vector2Int zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Zero; } }
		public static Vector2Int one { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_One; } }
		public static Vector2Int up { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Up; } }
		public static Vector2Int down { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Down; } }
		public static Vector2Int left { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Left; } }
		public static Vector2Int right { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Right; } }

		private static readonly Vector2Int s_Zero = new Vector2Int(0, 0);
		private static readonly Vector2Int s_One = new Vector2Int(1, 1);
		private static readonly Vector2Int s_Up = new Vector2Int(0, 1);
		private static readonly Vector2Int s_Down = new Vector2Int(0, -1);
		private static readonly Vector2Int s_Left = new Vector2Int(-1, 0);
		private static readonly Vector2Int s_Right = new Vector2Int(1, 0);
	}
}
