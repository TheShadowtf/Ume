using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System;

namespace Ume
{
	[StructLayout(LayoutKind.Sequential)]
	public partial struct Quaternion : IEquatable<Quaternion>, IFormattable
	{
		// X component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
		public float X;
		// Y component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
		public float Y;
		// Z component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
		public float Z;
		// W component of the Quaternion. Don't modify this directly unless you know quaternions inside out.
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
						throw new IndexOutOfRangeException("Invalid Quaternion index!");
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
						throw new IndexOutOfRangeException("Invalid Quaternion index!");
				}
			}
		}

		// Constructs new Quaternion with given x,y,z,w components.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Quaternion(float x, float y, float z, float w) { this.X = x; this.Y = y; this.Z = z; this.W = w; }

		// Set x, y, z and w components of an existing Quaternion.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Set(float newX, float newY, float newZ, float newW)
		{
			X = newX;
			Y = newY;
			Z = newZ;
			W = newW;
		}

		static readonly Quaternion identityQuaternion = new Quaternion(0F, 0F, 0F, 1F);

		// The identity rotation (RO). This quaternion corresponds to "no rotation": the object
		public static Quaternion identity
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				return identityQuaternion;
			}
		}

		// Combines rotations /lhs/ and /rhs/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Quaternion operator *(Quaternion lhs, Quaternion rhs)
		{
			return new Quaternion(
				lhs.W * rhs.X + lhs.X * rhs.W + lhs.Y * rhs.Z - lhs.Z * rhs.Y,
				lhs.W * rhs.Y + lhs.Y * rhs.W + lhs.Z * rhs.X - lhs.X * rhs.Z,
				lhs.W * rhs.Z + lhs.Z * rhs.W + lhs.X * rhs.Y - lhs.Y * rhs.X,
				lhs.W * rhs.W - lhs.X * rhs.X - lhs.Y * rhs.Y - lhs.Z * rhs.Z);
		}

		// Rotates the point /point/ with /rotation/.
		public static Vector3 operator *(Quaternion rotation, Vector3 point)
		{
			float x = rotation.X * 2F;
			float y = rotation.Y * 2F;
			float z = rotation.Z * 2F;
			float xx = rotation.X * x;
			float yy = rotation.Y * y;
			float zz = rotation.Z * z;
			float xy = rotation.X * y;
			float xz = rotation.X * z;
			float yz = rotation.Y * z;
			float wx = rotation.W * x;
			float wy = rotation.W * y;
			float wz = rotation.W * z;

			Vector3 res;
			res.X = (1F - (yy + zz)) * point.X + (xy - wz) * point.Y + (xz + wy) * point.Z;
			res.Y = (xy + wz) * point.X + (1F - (xx + zz)) * point.Y + (yz - wx) * point.Z;
			res.Z = (xz - wy) * point.X + (yz + wx) * point.Y + (1F - (xx + yy)) * point.Z;
			return res;
		}

		// *undocumented*
		public const float kEpsilon = 0.000001F;

		// Is the dot product of two quaternions within tolerance for them to be considered equal?
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		private static bool IsEqualUsingDot(float dot)
		{
			// Returns false in the presence of NaN values.
			return dot > 1.0f - kEpsilon;
		}

		// Are two quaternions equal to each other?
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator ==(Quaternion lhs, Quaternion rhs)
		{
			return IsEqualUsingDot(Dot(lhs, rhs));
		}

		// Are two quaternions different from each other?
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator !=(Quaternion lhs, Quaternion rhs)
		{
			// Returns true in the presence of NaN values.
			return !(lhs == rhs);
		}

		// The dot product between two rotations.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Dot(Quaternion a, Quaternion b)
		{
			return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
		}

		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetLookRotation(Vector3 view)
		//{
		//	Vector3 up = Vector3.up;
		//	SetLookRotation(view, up);
		//}

		// Creates a rotation with the specified /forward/ and /upwards/ directions.
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetLookRotation(Vector3 view, Vector3 up)
		//{
		//	this = LookRotation(view, up);
		//}

		// Returns the angle in degrees between two rotations /a/ and /b/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Angle(Quaternion a, Quaternion b)
		{
			float dot = Mathf.Min(Mathf.Abs(Dot(a, b)), 1.0F);
			return IsEqualUsingDot(dot) ? 0.0f : Mathf.Acos(dot) * 2.0F * Mathf.Rad2Deg;
		}

		// Makes euler angles positive 0/360 with 0.0001 hacked to support old behaviour of QuaternionToEuler
		private static Vector3 Internal_MakePositive(Vector3 euler)
		{
			float negativeFlip = -0.0001f * Mathf.Rad2Deg;
			float positiveFlip = 360.0f + negativeFlip;

			if (euler.X < negativeFlip)
				euler.X += 360.0f;
			else if (euler.X > positiveFlip)
				euler.X -= 360.0f;

			if (euler.Y < negativeFlip)
				euler.Y += 360.0f;
			else if (euler.Y > positiveFlip)
				euler.Y -= 360.0f;

			if (euler.Z < negativeFlip)
				euler.Z += 360.0f;
			else if (euler.Z > positiveFlip)
				euler.Z -= 360.0f;

			return euler;
		}

		//public Vector3 eulerAngles
		//{
		//	[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//	get { return Internal_MakePositive(Internal_ToEulerRad(this) * Mathf.Rad2Deg); }
		//	[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//	set { this = Internal_FromEulerRad(value * Mathf.Deg2Rad); }
		//}
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Quaternion Euler(float x, float y, float z) { return Internal_FromEulerRad(new Vector3(x, y, z) * Mathf.Deg2Rad); }
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Quaternion Euler(Vector3 euler) { return Internal_FromEulerRad(euler * Mathf.Deg2Rad); }
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void ToAngleAxis(out float angle, out Vector3 axis) { Internal_ToAxisAngleRad(this, out axis, out angle); angle *= Mathf.Rad2Deg; }
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetFromToRotation(Vector3 fromDirection, Vector3 toDirection) { this = FromToRotation(fromDirection, toDirection); }
		//
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Quaternion RotateTowards(Quaternion from, Quaternion to, float maxDegreesDelta)
		//{
		//	float angle = Quaternion.Angle(from, to);
		//	if (angle == 0.0f) return to;
		//	return SlerpUnclamped(from, to, Mathf.Min(1.0f, maxDegreesDelta / angle));
		//}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Quaternion Normalize(Quaternion q)
		{
			float mag = Mathf.Sqrt(Dot(q, q));

			if (mag < Mathf.Epsilon)
				return Quaternion.identity;

			return new Quaternion(q.X / mag, q.Y / mag, q.Z / mag, q.W / mag);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Normalize()
		{
			this = Normalize(this);
		}

		public Quaternion normalized
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return Normalize(this); }
		}

		// used to allow Quaternions to be used as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override int GetHashCode()
		{
			return X.GetHashCode() ^ (Y.GetHashCode() << 2) ^ (Z.GetHashCode() >> 2) ^ (W.GetHashCode() >> 1);
		}

		// also required for being able to use Quaternions as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override bool Equals(object other)
		{
			if (!(other is Quaternion)) return false;

			return Equals((Quaternion)other);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public bool Equals(Quaternion other)
		{
			return X.Equals(other.X) && Y.Equals(other.Y) && Z.Equals(other.Z) && W.Equals(other.W);
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
				format = "F5";
			if (formatProvider == null)
				formatProvider = CultureInfo.InvariantCulture.NumberFormat;
			return UmeString.Format("({0}, {1}, {2}, {3})", X.ToString(format, formatProvider), Y.ToString(format, formatProvider), Z.ToString(format, formatProvider), W.ToString(format, formatProvider));
		}

		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//static public Quaternion EulerRotation(float x, float y, float z) { return Internal_FromEulerRad(new Vector3(x, y, z)); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Quaternion EulerRotation(Vector3 euler) { return Internal_FromEulerRad(euler); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetEulerRotation(float x, float y, float z) { this = Internal_FromEulerRad(new Vector3(x, y, z)); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetEulerRotation(Vector3 euler) { this = Internal_FromEulerRad(euler); }
		//[System.Obsolete("Use Quaternion.eulerAngles instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public Vector3 ToEuler() { return Internal_ToEulerRad(this); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//static public Quaternion EulerAngles(float x, float y, float z) { return Internal_FromEulerRad(new Vector3(x, y, z)); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Quaternion EulerAngles(Vector3 euler) { return Internal_FromEulerRad(euler); }
		//[System.Obsolete("Use Quaternion.ToAngleAxis instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void ToAxisAngle(out Vector3 axis, out float angle) { Internal_ToAxisAngleRad(this, out axis, out angle); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetEulerAngles(float x, float y, float z) { SetEulerRotation(new Vector3(x, y, z)); }
		//[System.Obsolete("Use Quaternion.Euler instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetEulerAngles(Vector3 euler) { this = EulerRotation(euler); }
		//[System.Obsolete("Use Quaternion.eulerAngles instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Vector3 ToEulerAngles(Quaternion rotation) { return Quaternion.Internal_ToEulerRad(rotation); }
		//[System.Obsolete("Use Quaternion.eulerAngles instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public Vector3 ToEulerAngles() { return Quaternion.Internal_ToEulerRad(this); }
		//[System.Obsolete("Use Quaternion.AngleAxis instead. This function was deprecated because it uses radians instead of degrees.")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public void SetAxisAngle(Vector3 axis, float angle) { this = AxisAngle(axis, angle); }
		//[System.Obsolete("Use Quaternion.AngleAxis instead. This function was deprecated because it uses radians instead of degrees")]
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Quaternion AxisAngle(Vector3 axis, float angle) { return AngleAxis(Mathf.Rad2Deg * angle, axis); }
	}
} //namespace
