using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System;
using Ume;

namespace Ume
{
	[StructLayout(LayoutKind.Sequential)]
	public struct Vector3Int : IEquatable<Vector3Int>, IFormattable
	{
		public int X { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return m_X; } [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] set { m_X = value; } }
		public int Y { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return m_Y; } [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] set { m_Y = value; } }
		public int Z { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return m_Z; } [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] set { m_Z = value; } }

		private int m_X;
		private int m_Y;
		private int m_Z;

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector3Int(int x, int y)
		{
			m_X = x;
			m_Y = y;
			m_Z = 0;
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector3Int(int x, int y, int z)
		{
			m_X = x;
			m_Y = y;
			m_Z = z;
		}

		// Set x, y and z components of an existing Vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Set(int x, int y, int z)
		{
			m_X = x;
			m_Y = y;
			m_Z = z;
		}

		// Access the /x/, /y/ or /z/ component using [0], [1] or [2] respectively.
		public int this[int index]
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				switch (index)
				{
					case 0: return X;
					case 1: return Y;
					case 2: return Z;
					default:
						throw new IndexOutOfRangeException(UmeString.Format("Invalid Vector3Int index addressed: {0}!", index));
				}
			}

			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			set
			{
				switch (index)
				{
					case 0: X = value; break;
					case 1: Y = value; break;
					case 2: Z = value; break;
					default:
						throw new IndexOutOfRangeException(UmeString.Format("Invalid Vector3Int index addressed: {0}!", index));
				}
			}
		}

		// Returns the length of this vector (RO).
		public float magnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return Mathf.Sqrt((float)(X * X + Y * Y + Z * Z)); } }

		// Returns the squared length of this vector (RO).
		public int sqrMagnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return X * X + Y * Y + Z * Z; } }

		// Returns the distance between /a/ and /b/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Distance(Vector3Int a, Vector3Int b) { return (a - b).magnitude; }

		// Returns a vector that is made from the smallest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int Min(Vector3Int lhs, Vector3Int rhs) { return new Vector3Int(Mathf.Min(lhs.X, rhs.X), Mathf.Min(lhs.Y, rhs.Y), Mathf.Min(lhs.Z, rhs.Z)); }

		// Returns a vector that is made from the largest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int Max(Vector3Int lhs, Vector3Int rhs) { return new Vector3Int(Mathf.Max(lhs.X, rhs.X), Mathf.Max(lhs.Y, rhs.Y), Mathf.Max(lhs.Z, rhs.Z)); }

		// Multiplies two vectors component-wise.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int Scale(Vector3Int a, Vector3Int b) { return new Vector3Int(a.X * b.X, a.Y * b.Y, a.Z * b.Z); }

		// Multiplies every component of this vector by the same component of /scale/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Scale(Vector3Int scale) { X *= scale.X; Y *= scale.Y; Z *= scale.Z; }

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Clamp(Vector3Int min, Vector3Int max)
		{
			X = Math.Max(min.X, X);
			X = Math.Min(max.X, X);
			Y = Math.Max(min.Y, Y);
			Y = Math.Min(max.Y, Y);
			Z = Math.Max(min.Z, Z);
			Z = Math.Min(max.Z, Z);
		}

		// Converts a Vector3Int to a [[Vector3]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector3(Vector3Int v)
		{
			return new Vector3(v.X, v.Y, v.Z);
		}

		// Converts a Vector3Int to a [[Vector2Int]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static explicit operator Vector2Int(Vector3Int v)
		{
			return new Vector2Int(v.X, v.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int FloorToInt(Vector3 v)
		{
			return new Vector3Int(
				Mathf.FloorToInt(v.X),
				Mathf.FloorToInt(v.Y),
				Mathf.FloorToInt(v.Z)
			);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int CeilToInt(Vector3 v)
		{
			return new Vector3Int(
				Mathf.CeilToInt(v.X),
				Mathf.CeilToInt(v.Y),
				Mathf.CeilToInt(v.Z)
			);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int RoundToInt(Vector3 v)
		{
			return new Vector3Int(
				Mathf.RoundToInt(v.X),
				Mathf.RoundToInt(v.Y),
				Mathf.RoundToInt(v.Z)
			);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator +(Vector3Int a, Vector3Int b)
		{
			return new Vector3Int(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator -(Vector3Int a, Vector3Int b)
		{
			return new Vector3Int(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator *(Vector3Int a, Vector3Int b)
		{
			return new Vector3Int(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator -(Vector3Int a)
		{
			return new Vector3Int(-a.X, -a.Y, -a.Z);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator *(Vector3Int a, int b)
		{
			return new Vector3Int(a.X * b, a.Y * b, a.Z * b);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator *(int a, Vector3Int b)
		{
			return new Vector3Int(a * b.X, a * b.Y, a * b.Z);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3Int operator /(Vector3Int a, int b)
		{
			return new Vector3Int(a.X / b, a.Y / b, a.Z / b);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator ==(Vector3Int lhs, Vector3Int rhs)
		{
			return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator !=(Vector3Int lhs, Vector3Int rhs)
		{
			return !(lhs == rhs);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override bool Equals(object other)
		{
			if (!(other is Vector3Int)) return false;

			return Equals((Vector3Int)other);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public bool Equals(Vector3Int other)
		{
			return this == other;
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override int GetHashCode()
		{
			var yHash = Y.GetHashCode();
			var zHash = Z.GetHashCode();
			return X.GetHashCode() ^ (yHash << 4) ^ (yHash >> 28) ^ (zHash >> 4) ^ (zHash << 28);
		}

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
			return UmeString.Format("({0}, {1}, {2})", X.ToString(format, formatProvider), Y.ToString(format, formatProvider), Z.ToString(format, formatProvider));
		}

		public static Vector3Int zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Zero; } }
		public static Vector3Int one { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_One; } }
		public static Vector3Int up { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Up; } }
		public static Vector3Int down { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Down; } }
		public static Vector3Int left { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Left; } }
		public static Vector3Int right { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Right; } }
		public static Vector3Int forward { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Forward; } }
		public static Vector3Int back { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return s_Back; } }

		private static readonly Vector3Int s_Zero = new Vector3Int(0, 0, 0);
		private static readonly Vector3Int s_One = new Vector3Int(1, 1, 1);
		private static readonly Vector3Int s_Up = new Vector3Int(0, 1, 0);
		private static readonly Vector3Int s_Down = new Vector3Int(0, -1, 0);
		private static readonly Vector3Int s_Left = new Vector3Int(-1, 0, 0);
		private static readonly Vector3Int s_Right = new Vector3Int(1, 0, 0);
		private static readonly Vector3Int s_Forward = new Vector3Int(0, 0, 1);
		private static readonly Vector3Int s_Back = new Vector3Int(0, 0, -1);
	}
}
