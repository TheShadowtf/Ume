using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Ume;

namespace Ume
{
	// Representation of 3D vectors and points.
	[NativeClass("Vector3f")]
	[StructLayout(LayoutKind.Sequential)]
	public partial struct Vector3 : IEquatable<Vector3>, IFormattable
	{
		// *Undocumented*
		public const float kEpsilon = 0.00001F;
		// *Undocumented*
		public const float kEpsilonNormalSqrt = 1e-15F;

		// X component of the vector.
		public float X;
		// Y component of the vector.
		public float Y;
		// Z component of the vector.
		public float Z;

		// Linearly interpolates between two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Lerp(Vector3 a, Vector3 b, float t)
		{
			t = Mathf.Clamp01(t);
			return new Vector3(
				a.X + (b.X - a.X) * t,
				a.Y + (b.Y - a.Y) * t,
				a.Z + (b.Z - a.Z) * t
			);
		}

		// Linearly interpolates between two vectors without clamping the interpolant
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 LerpUnclamped(Vector3 a, Vector3 b, float t)
		{
			return new Vector3(
				a.X + (b.X - a.X) * t,
				a.Y + (b.Y - a.Y) * t,
				a.Z + (b.Z - a.Z) * t
			);
		}

		// Moves a point /current/ in a straight line towards a /target/ point.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta)
		{
			// avoid vector ops because current scripting backends are terrible at inlining
			float toVector_x = target.X - current.X;
			float toVector_y = target.Y - current.Y;
			float toVector_z = target.Z - current.Z;

			float sqdist = toVector_x * toVector_x + toVector_y * toVector_y + toVector_z * toVector_z;

			if (sqdist == 0 || (maxDistanceDelta >= 0 && sqdist <= maxDistanceDelta * maxDistanceDelta))
				return target;
			var dist = (float)Math.Sqrt(sqdist);

			return new Vector3(current.X + toVector_x / dist * maxDistanceDelta,
				current.Y + toVector_y / dist * maxDistanceDelta,
				current.Z + toVector_z / dist * maxDistanceDelta);
		}

		// Todo:

		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Vector3 SmoothDamp(Vector3 current, Vector3 target, ref Vector3 currentVelocity, float smoothTime, float maxSpeed)
		//{
		//	float deltaTime = Time.deltaTime;
		//	return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
		//}
		//
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Vector3 SmoothDamp(Vector3 current, Vector3 target, ref Vector3 currentVelocity, float smoothTime)
		//{
		//	float deltaTime = Time.deltaTime;
		//	float maxSpeed = Mathf.Infinity;
		//	return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
		//}

		// Gradually changes a vector towards a desired goal over time.
		public static Vector3 SmoothDamp(Vector3 current, Vector3 target, ref Vector3 currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
		{
			float output_x = 0f;
			float output_y = 0f;
			float output_z = 0f;

			// Based on Game Programming Gems 4 Chapter 1.10
			smoothTime = Mathf.Max(0.0001F, smoothTime);
			float omega = 2F / smoothTime;

			float x = omega * deltaTime;
			float exp = 1F / (1F + x + 0.48F * x * x + 0.235F * x * x * x);

			float change_x = current.X - target.X;
			float change_y = current.Y - target.Y;
			float change_z = current.Z - target.Z;
			Vector3 originalTo = target;

			// Clamp maximum speed
			float maxChange = maxSpeed * smoothTime;

			float maxChangeSq = maxChange * maxChange;
			float sqrmag = change_x * change_x + change_y * change_y + change_z * change_z;
			if (sqrmag > maxChangeSq)
			{
				var mag = (float)Math.Sqrt(sqrmag);
				change_x = change_x / mag * maxChange;
				change_y = change_y / mag * maxChange;
				change_z = change_z / mag * maxChange;
			}

			target.X = current.X - change_x;
			target.Y = current.Y - change_y;
			target.Z = current.Z - change_z;

			float temp_x = (currentVelocity.X + omega * change_x) * deltaTime;
			float temp_y = (currentVelocity.Y + omega * change_y) * deltaTime;
			float temp_z = (currentVelocity.Z + omega * change_z) * deltaTime;

			currentVelocity.X = (currentVelocity.X - omega * temp_x) * exp;
			currentVelocity.Y = (currentVelocity.Y - omega * temp_y) * exp;
			currentVelocity.Z = (currentVelocity.Z - omega * temp_z) * exp;

			output_x = target.X + (change_x + temp_x) * exp;
			output_y = target.Y + (change_y + temp_y) * exp;
			output_z = target.Z + (change_z + temp_z) * exp;

			// Prevent overshooting
			float origMinusCurrent_x = originalTo.X - current.X;
			float origMinusCurrent_y = originalTo.Y - current.Y;
			float origMinusCurrent_z = originalTo.Z - current.Z;
			float outMinusOrig_x = output_x - originalTo.X;
			float outMinusOrig_y = output_y - originalTo.Y;
			float outMinusOrig_z = output_z - originalTo.Z;

			if (origMinusCurrent_x * outMinusOrig_x + origMinusCurrent_y * outMinusOrig_y + origMinusCurrent_z * outMinusOrig_z > 0)
			{
				output_x = originalTo.X;
				output_y = originalTo.Y;
				output_z = originalTo.Z;

				currentVelocity.X = (output_x - originalTo.X) / deltaTime;
				currentVelocity.Y = (output_y - originalTo.Y) / deltaTime;
				currentVelocity.Z = (output_z - originalTo.Z) / deltaTime;
			}

			return new Vector3(output_x, output_y, output_z);
		}

		// Access the x, y, z components using [0], [1], [2] respectively.
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
					default:
						throw new IndexOutOfRangeException("Invalid Vector3 index!");
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
						throw new IndexOutOfRangeException("Invalid Vector3 index!");
				}
			}
		}

		// Floats

		// Creates a new vector with given x, y, z components.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector3(float x, float y, float z) { this.X = x; this.Y = y; this.Z = z; }
		// Creates a new vector with given x, y components and sets /z/ to zero.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector3(float x, float y) { this.X = x; this.Y = y; Z = 0F; }
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector3(float x) { this.X = x; this.Y = 0F; Z = 0F; }

		// Vectors
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector3(Vector2 v, float z) { this.X = v.X; this.Y = v.Y; this.Z = z; }

		// Set x, y and z components of an existing Vector3.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Set(float newX, float newY, float newZ) { X = newX; Y = newY; Z = newZ; }

		// Multiplies two vectors component-wise.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Scale(Vector3 a, Vector3 b) { return new Vector3(a.X * b.X, a.Y * b.Y, a.Z * b.Z); }

		// Multiplies every component of this vector by the same component of /scale/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Scale(Vector3 scale) { X *= scale.X; Y *= scale.Y; Z *= scale.Z; }

		// Cross Product of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Cross(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(
				lhs.Y * rhs.Z - lhs.Z * rhs.Y,
				lhs.Z * rhs.X - lhs.X * rhs.Z,
				lhs.X * rhs.Y - lhs.Y * rhs.X);
		}

		// used to allow Vector3s to be used as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override int GetHashCode()
		{
			return X.GetHashCode() ^ (Y.GetHashCode() << 2) ^ (Z.GetHashCode() >> 2);
		}

		// also required for being able to use Vector3s as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override bool Equals(object other)
		{
			if (!(other is Vector3)) return false;

			return Equals((Vector3)other);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public bool Equals(Vector3 other)
		{
			return X == other.X && Y == other.Y && Z == other.Z;
		}

		// Reflects a vector off the plane defined by a normal.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Reflect(Vector3 inDirection, Vector3 inNormal)
		{
			float factor = -2F * Dot(inNormal, inDirection);
			return new Vector3(factor * inNormal.X + inDirection.X,
				factor * inNormal.Y + inDirection.Y,
				factor * inNormal.Z + inDirection.Z);
		}

		// *undoc* --- we have normalized property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Normalize(Vector3 value)
		{
			float mag = Magnitude(value);
			if (mag > kEpsilon)
				return value / mag;
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
		public Vector3 normalized
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return Vector3.Normalize(this); }
		}

		// Dot Product of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Dot(Vector3 lhs, Vector3 rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z; }

		// Projects a vector onto another vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Project(Vector3 vector, Vector3 onNormal)
		{
			float sqrMag = Dot(onNormal, onNormal);
			if (sqrMag < Mathf.Epsilon)
				return zero;
			else
			{
				var dot = Dot(vector, onNormal);
				return new Vector3(onNormal.X * dot / sqrMag,
					onNormal.Y * dot / sqrMag,
					onNormal.Z * dot / sqrMag);
			}
		}

		// Projects a vector onto a plane defined by a normal orthogonal to the plane.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 ProjectOnPlane(Vector3 vector, Vector3 planeNormal)
		{
			float sqrMag = Dot(planeNormal, planeNormal);
			if (sqrMag < Mathf.Epsilon)
				return vector;
			else
			{
				var dot = Dot(vector, planeNormal);
				return new Vector3(vector.X - planeNormal.X * dot / sqrMag,
					vector.Y - planeNormal.Y * dot / sqrMag,
					vector.Z - planeNormal.Z * dot / sqrMag);
			}
		}

		// Returns the angle in degrees between /from/ and /to/. This is always the smallest
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Angle(Vector3 from, Vector3 to)
		{
			// sqrt(a) * sqrt(b) = sqrt(a * b) -- valid for real numbers
			float denominator = (float)Math.Sqrt(from.sqrMagnitude * to.sqrMagnitude);
			if (denominator < kEpsilonNormalSqrt)
				return 0F;

			float dot = Mathf.Clamp(Dot(from, to) / denominator, -1F, 1F);
			return ((float)Math.Acos(dot)) * Mathf.Rad2Deg;
		}

		// The smaller of the two possible angles between the two vectors is returned, therefore the result will never be greater than 180 degrees or smaller than -180 degrees.
		// If you imagine the from and to vectors as lines on a piece of paper, both originating from the same point, then the /axis/ vector would point up out of the paper.
		// The measured angle between the two vectors would be positive in a clockwise direction and negative in an anti-clockwise direction.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float SignedAngle(Vector3 from, Vector3 to, Vector3 axis)
		{
			float unsignedAngle = Angle(from, to);

			float cross_x = from.Y * to.Z - from.Z * to.Y;
			float cross_y = from.Z * to.X - from.X * to.Z;
			float cross_z = from.X * to.Y - from.Y * to.X;
			float sign = Mathf.Sign(axis.X * cross_x + axis.Y * cross_y + axis.Z * cross_z);
			return unsignedAngle * sign;
		}

		// Returns the distance between /a/ and /b/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Distance(Vector3 a, Vector3 b)
		{
			float diff_x = a.X - b.X;
			float diff_y = a.Y - b.Y;
			float diff_z = a.Z - b.Z;
			return (float)Math.Sqrt(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);
		}

		// Returns a copy of /vector/ with its magnitude clamped to /maxLength/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 ClampMagnitude(Vector3 vector, float maxLength)
		{
			float sqrmag = vector.sqrMagnitude;
			if (sqrmag > maxLength * maxLength)
			{
				float mag = (float)Math.Sqrt(sqrmag);
				//these intermediate variables force the intermediate result to be
				//of float precision. without this, the intermediate result can be of higher
				//precision, which changes behavior.
				float normalized_x = vector.X / mag;
				float normalized_y = vector.Y / mag;
				float normalized_z = vector.Z / mag;
				return new Vector3(normalized_x * maxLength,
					normalized_y * maxLength,
					normalized_z * maxLength);
			}
			return vector;
		}

		// *undoc* --- there's a property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Magnitude(Vector3 vector) { return (float)Math.Sqrt(vector.X * vector.X + vector.Y * vector.Y + vector.Z * vector.Z); }

		// Returns the length of this vector (RO).
		public float magnitude
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return (float)Math.Sqrt(X * X + Y * Y + Z * Z); }
		}

		// *undoc* --- there's a property now
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float SqrMagnitude(Vector3 vector) { return vector.X * vector.X + vector.Y * vector.Y + vector.Z * vector.Z; }

		// Returns the squared length of this vector (RO).
		public float sqrMagnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return X * X + Y * Y + Z * Z; } }

		// Returns a vector that is made from the smallest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Min(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(Mathf.Min(lhs.X, rhs.X), Mathf.Min(lhs.Y, rhs.Y), Mathf.Min(lhs.Z, rhs.Z));
		}

		// Returns a vector that is made from the largest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 Max(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(Mathf.Max(lhs.X, rhs.X), Mathf.Max(lhs.Y, rhs.Y), Mathf.Max(lhs.Z, rhs.Z));
		}

		static readonly Vector3 zeroVector = new Vector3(0F, 0F, 0F);
		static readonly Vector3 oneVector = new Vector3(1F, 1F, 1F);
		static readonly Vector3 upVector = new Vector3(0F, 1F, 0F);
		static readonly Vector3 downVector = new Vector3(0F, -1F, 0F);
		static readonly Vector3 leftVector = new Vector3(-1F, 0F, 0F);
		static readonly Vector3 rightVector = new Vector3(1F, 0F, 0F);
		static readonly Vector3 forwardVector = new Vector3(0F, 0F, 1F);
		static readonly Vector3 backVector = new Vector3(0F, 0F, -1F);
		static readonly Vector3 positiveInfinityVector = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
		static readonly Vector3 negativeInfinityVector = new Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity);

		// Shorthand for writing @@Vector3(0, 0, 0)@@
		public static Vector3 zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return zeroVector; } }
		// Backward-compatible alias used by older scripts.
		public static Vector3 Zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return zero; } }

		// Backward-compatible XY swizzle used by older scripts.
		public Vector2 XY
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get { return new Vector2(X, Y); }
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			set
			{
				X = value.X;
				Y = value.Y;
			}
		}
		// Shorthand for writing @@Vector3(1, 1, 1)@@
		public static Vector3 one { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return oneVector; } }
		// Shorthand for writing @@Vector3(0, 0, 1)@@
		public static Vector3 forward { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return forwardVector; } }
		public static Vector3 back { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return backVector; } }
		// Shorthand for writing @@Vector3(0, 1, 0)@@
		public static Vector3 up { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return upVector; } }
		public static Vector3 down { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return downVector; } }
		public static Vector3 left { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return leftVector; } }
		// Shorthand for writing @@Vector3(1, 0, 0)@@
		public static Vector3 right { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return rightVector; } }
		// Shorthand for writing @@Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity)@@
		public static Vector3 positiveInfinity { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return positiveInfinityVector; } }
		// Shorthand for writing @@Vector3(float.NegativeInfinity, float.NegativeInfinity, float.NegativeInfinity)@@
		public static Vector3 negativeInfinity { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return negativeInfinityVector; } }

		// Adds two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 operator +(Vector3 a, Vector3 b) { return new Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z); }
		// Subtracts one vector from another.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 operator -(Vector3 a, Vector3 b) { return new Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z); }
		// Negates a vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 operator -(Vector3 a) { return new Vector3(-a.X, -a.Y, -a.Z); }
		// Multiplies a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 operator *(Vector3 a, float d) { return new Vector3(a.X * d, a.Y * d, a.Z * d); }
		// Multiplies a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 operator *(float d, Vector3 a) { return new Vector3(a.X * d, a.Y * d, a.Z * d); }
		// Divides a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector3 operator /(Vector3 a, float d) { return new Vector3(a.X / d, a.Y / d, a.Z / d); }

		// Returns true if the vectors are equal.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator ==(Vector3 lhs, Vector3 rhs)
		{
			// Returns false in the presence of NaN values.
			float diff_x = lhs.X - rhs.X;
			float diff_y = lhs.Y - rhs.Y;
			float diff_z = lhs.Z - rhs.Z;
			float sqrmag = diff_x * diff_x + diff_y * diff_y + diff_z * diff_z;
			return sqrmag < kEpsilon * kEpsilon;
		}

		// Returns true if vectors are different.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator !=(Vector3 lhs, Vector3 rhs)
		{
			// Returns true in the presence of NaN values.
			return !(lhs == rhs);
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
			return UmeString.Format("({0}, {1}, {2})", X.ToString(format, formatProvider), Y.ToString(format, formatProvider), Z.ToString(format, formatProvider));
		}

		[System.Obsolete("Use Vector3.forward instead.")]
		public static Vector3 fwd { get { return new Vector3(0F, 0F, 1F); } }
		[System.Obsolete("Use Vector3.Angle instead. AngleBetween uses radians instead of degrees and was deprecated for this reason")]
		public static float AngleBetween(Vector3 from, Vector3 to) { return (float)Math.Acos(Mathf.Clamp(Vector3.Dot(from.normalized, to.normalized), -1F, 1F)); }
		[System.Obsolete("Use Vector3.ProjectOnPlane instead.")]
		public static Vector3 Exclude(Vector3 excludeThis, Vector3 fromThat) { return ProjectOnPlane(fromThat, excludeThis); }
	}
}
