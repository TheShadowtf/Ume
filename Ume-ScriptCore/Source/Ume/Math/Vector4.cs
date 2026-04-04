using System.Globalization;
using System.Runtime.CompilerServices;
using System;
using Ume;

namespace Ume
{
	[NativeClass("Vector4f")]
	// Representation of four-dimensional vectors.
	public partial struct Vector4 : IEquatable<Vector4>, IFormattable
	{
		// *undocumented*
		public const float kEpsilon = 0.00001F;

		// X component of the vector.
		public float X;
		// Y component of the vector.
		public float Y;
		// Z component of the vector.
		public float Z;
		// W component of the vector.
		public float W;

		// Access the x, y, z, w components using [0], [1], [2], [3] respectively.
		public float this[int index]
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				switch (index)
				{
					case 0: return X;
					case 1: return Y;
					case 2: return Z;
					case 3: return W;
					default:
						throw new IndexOutOfRangeException("Invalid Vector4 index!");
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
					case 3: W = value; break;
					default:
						throw new IndexOutOfRangeException("Invalid Vector4 index!");
				}
			}
		}

		// Creates a new vector with given x, y, z, w components.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector4(float x, float y, float z, float w) { this.X = x; this.Y = y; this.Z = z; this.W = w; }
		// Creates a new vector with given x, y, z components and sets /w/ to zero.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector4(float x, float y, float z) { this.X = x; this.Y = y; this.Z = z; this.W = 0F; }
		// Creates a new vector with given x, y components and sets /z/ and /w/ to zero.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector4(float x, float y) { this.X = x; this.Y = y; this.Z = 0F; this.W = 0F; }
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector4(float x) { this.X = x; this.Y = 0F; this.Z = 0F; this.W = 0F; }
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector4(ColorPicker picker) { this.X = picker.X; this.Y = picker.Y; this.Z = picker.Z; this.W = picker.W; }

		// Set x, y, z and w components of an existing Vector4.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Set(float newX, float newY, float newZ, float newW) { X = newX; Y = newY; Z = newZ; W = newW; }

		// Linearly interpolates between two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 Lerp(Vector4 a, Vector4 b, float t)
		{
			t = Mathf.Clamp01(t);
			return new Vector4(
				a.X + (b.X - a.X) * t,
				a.Y + (b.Y - a.Y) * t,
				a.Z + (b.Z - a.Z) * t,
				a.W + (b.W - a.W) * t
			);
		}

		// Linearly interpolates between two vectors without clamping the interpolant
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 LerpUnclamped(Vector4 a, Vector4 b, float t)
		{
			return new Vector4(
				a.X + (b.X - a.X) * t,
				a.Y + (b.Y - a.Y) * t,
				a.Z + (b.Z - a.Z) * t,
				a.W + (b.W - a.W) * t
			);
		}

		// Moves a point /current/ towards /target/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 MoveTowards(Vector4 current, Vector4 target, float maxDistanceDelta)
		{
			float toVector_x = target.X - current.X;
			float toVector_y = target.Y - current.Y;
			float toVector_z = target.Z - current.Z;
			float toVector_w = target.W - current.W;

			float sqdist = (toVector_x * toVector_x +
				toVector_y * toVector_y +
				toVector_z * toVector_z +
				toVector_w * toVector_w);

			if (sqdist == 0 || (maxDistanceDelta >= 0 && sqdist <= maxDistanceDelta * maxDistanceDelta))
				return target;

			var dist = (float)Math.Sqrt(sqdist);

			return new Vector4(current.X + toVector_x / dist * maxDistanceDelta,
				current.Y + toVector_y / dist * maxDistanceDelta,
				current.Z + toVector_z / dist * maxDistanceDelta,
				current.W + toVector_w / dist * maxDistanceDelta);
		}

		// Multiplies two vectors component-wise.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 Scale(Vector4 a, Vector4 b)
		{
			return new Vector4(a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W);
		}

		// Multiplies every component of this vector by the same component of /scale/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Scale(Vector4 scale)
		{
			X *= scale.X;
			Y *= scale.Y;
			Z *= scale.Z;
			W *= scale.W;
		}

		// used to allow Vector4s to be used as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override int GetHashCode()
		{
			return X.GetHashCode() ^ (Y.GetHashCode() << 2) ^ (Z.GetHashCode() >> 2) ^ (W.GetHashCode() >> 1);
		}

		// also required for being able to use Vector4s as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override bool Equals(object other)
		{
			if (!(other is Vector4)) return false;

			return Equals((Vector4)other);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public bool Equals(Vector4 other)
		{
			return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
		}

		// *undoc* --- we have normalized property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 Normalize(Vector4 a)
		{
			float mag = Magnitude(a);
			if (mag > kEpsilon)
				return a / mag;
			else
				return zero;
		}

		// Makes this vector have a ::ref::magnitude of 1.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Normalize()
		{
			float mag = Magnitude(this);
			if (mag > kEpsilon)
				this = this / mag;
			else
				this = zero;
		}

		// Returns this vector with a ::ref::magnitude of 1 (RO).
		public Vector4 normalized
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				return Vector4.Normalize(this);
			}
		}

		// Dot Product of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Dot(Vector4 a, Vector4 b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W; }

		// Projects a vector onto another vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 Project(Vector4 a, Vector4 b) { return b * (Dot(a, b) / Dot(b, b)); }

		// Returns the distance between /a/ and /b/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Distance(Vector4 a, Vector4 b) { return Magnitude(a - b); }

		// *undoc* --- there's a property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Magnitude(Vector4 a) { return (float)Math.Sqrt(Dot(a, a)); }

		// Returns the length of this vector (RO).
		public float magnitude
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return (float)Math.Sqrt(Dot(this, this)); }
		}

		// Returns the squared length of this vector (RO).
		public float sqrMagnitude
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return Dot(this, this); }
		}

		// Returns a vector that is made from the smallest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 Min(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(Mathf.Min(lhs.X, rhs.X), Mathf.Min(lhs.Y, rhs.Y), Mathf.Min(lhs.Z, rhs.Z), Mathf.Min(lhs.W, rhs.W));
		}

		// Returns a vector that is made from the largest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 Max(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(Mathf.Max(lhs.X, rhs.X), Mathf.Max(lhs.Y, rhs.Y), Mathf.Max(lhs.Z, rhs.Z), Mathf.Max(lhs.W, rhs.W));
		}

		static readonly Vector4 zeroVector = new Vector4(0F, 0F, 0F, 0F);
		static readonly Vector4 oneVector = new Vector4(1F, 1F, 1F, 1F);
		static readonly Vector4 positiveInfinityVector = new Vector4(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
		static readonly Vector4 negativeInfinityVector = new Vector4(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);

		// Shorthand for writing @@Vector4(0,0,0,0)@@
		public static Vector4 zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return zeroVector; } }
		// Shorthand for writing @@Vector4(1,1,1,1)@@
		public static Vector4 one { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return oneVector; } }
		// Shorthand for writing @@Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity)@@
		public static Vector4 positiveInfinity { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return positiveInfinityVector; } }
		// Shorthand for writing @@Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity)@@
		public static Vector4 negativeInfinity { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return negativeInfinityVector; } }

		// Adds two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 operator +(Vector4 a, Vector4 b) { return new Vector4(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W); }
		// Subtracts one vector from another.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 operator -(Vector4 a, Vector4 b) { return new Vector4(a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W); }
		// Negates a vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 operator -(Vector4 a) { return new Vector4(-a.X, -a.Y, -a.Z, -a.W); }
		// Multiplies a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 operator *(Vector4 a, float d) { return new Vector4(a.X * d, a.Y * d, a.Z * d, a.W * d); }
		// Multiplies a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 operator *(float d, Vector4 a) { return new Vector4(a.X * d, a.Y * d, a.Z * d, a.W * d); }
		// Divides a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector4 operator /(Vector4 a, float d) { return new Vector4(a.X / d, a.Y / d, a.Z / d, a.W / d); }

		// Returns true if the vectors are equal.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator ==(Vector4 lhs, Vector4 rhs)
		{
			// Returns false in the presence of NaN values.
			float diffx = lhs.X - rhs.X;
			float diffy = lhs.Y - rhs.Y;
			float diffz = lhs.Z - rhs.Z;
			float diffw = lhs.W - rhs.W;
			float sqrmag = diffx * diffx + diffy * diffy + diffz * diffz + diffw * diffw;
			return sqrmag < kEpsilon * kEpsilon;
		}

		// Returns true if vectors are different.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator !=(Vector4 lhs, Vector4 rhs)
		{
			// Returns true in the presence of NaN values.
			return !(lhs == rhs);
		}

		// Converts a [[Vector3]] to a Vector4.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector4(Vector3 v)
		{
			return new Vector4(v.X, v.Y, v.Z, 0.0F);
		}

		// Converts a Vector4 to a [[Vector3]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector3(Vector4 v)
		{
			return new Vector3(v.X, v.Y, v.Z);
		}

		// Converts a [[Vector2]] to a Vector4.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector4(Vector2 v)
		{
			return new Vector4(v.X, v.Y, 0.0F, 0.0F);
		}

		// Converts a Vector4 to a [[Vector2]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector2(Vector4 v)
		{
			return new Vector2(v.X, v.Y);
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
			if (string.IsNullOrEmpty(format))
				format = "F2";
			if (formatProvider == null)
				formatProvider = CultureInfo.InvariantCulture.NumberFormat;
			return UmeString.Format("({0}, {1}, {2}, {3})", X.ToString(format, formatProvider), Y.ToString(format, formatProvider), Z.ToString(format, formatProvider), W.ToString(format, formatProvider));
		}

		// *undoc* --- there's a property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float SqrMagnitude(Vector4 a) { return Vector4.Dot(a, a); }
		// *undoc* --- there's a property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public float SqrMagnitude() { return Dot(this, this); }
	}
} // namespace
