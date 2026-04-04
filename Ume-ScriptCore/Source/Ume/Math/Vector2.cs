using System;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Ume;

namespace Ume
{
	static class MethodImplOptionsEx
	{
		public const short AggressiveInlining = 256;
	}
	[StructLayout(LayoutKind.Sequential)]
	[NativeClass("Vector2f")]
	public struct Vector2 : IEquatable<Vector2>, IFormattable
	{
		// X component of the vector.
		public float X;
		// Y component of the vector.
		public float Y;

		// Access the /x/ or /y/ component using [0] or [1] respectively.
		public float this[int index]
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				switch (index)
				{
					case 0: return X;
					case 1: return Y;
					default:
						throw new IndexOutOfRangeException("Invalid Vector2 index!");
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
						throw new IndexOutOfRangeException("Invalid Vector2 index!");
				}
			}
		}

		// Constructs a new vector with given x, y components.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector2(float x, float y) { this.X = x; this.Y = y; }
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public Vector2(float x) { this.X = x; this.Y = 0F; }

		// Set x and y components of an existing Vector2.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Set(float newX, float newY) { X = newX; Y = newY; }

		// Linearly interpolates between two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 Lerp(Vector2 a, Vector2 b, float t)
		{
			t = Mathf.Clamp01(t);
			return new Vector2(
				a.X + (b.X - a.X) * t,
				a.Y + (b.Y - a.Y) * t
			);
		}

		// Linearly interpolates between two vectors without clamping the interpolant
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 LerpUnclamped(Vector2 a, Vector2 b, float t)
		{
			return new Vector2(
				a.X + (b.X - a.X) * t,
				a.Y + (b.Y - a.Y) * t
			);
		}

		// Moves a point /current/ towards /target/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta)
		{
			// avoid vector ops because current scripting backends are terrible at inlining
			float toVector_x = target.X - current.X;
			float toVector_y = target.Y - current.Y;

			float sqDist = toVector_x * toVector_x + toVector_y * toVector_y;

			if (sqDist == 0 || (maxDistanceDelta >= 0 && sqDist <= maxDistanceDelta * maxDistanceDelta))
				return target;

			float dist = (float)Math.Sqrt(sqDist);

			return new Vector2(current.X + toVector_x / dist * maxDistanceDelta,
				current.Y + toVector_y / dist * maxDistanceDelta);
		}

		// Multiplies two vectors component-wise.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 Scale(Vector2 a, Vector2 b) { return new Vector2(a.X * b.X, a.Y * b.Y); }

		// Multiplies every component of this vector by the same component of /scale/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Scale(Vector2 scale) { X *= scale.X; Y *= scale.Y; }

		// Makes this vector have a ::ref::magnitude of 1.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public void Normalize()
		{
			float mag = magnitude;
			if (mag > kEpsilon)
				this = this / mag;
			else
				this = zero;
		}

		// Returns this vector with a ::ref::magnitude of 1 (RO).
		public Vector2 normalized
		{
			[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
			get
			{
				Vector2 v = new Vector2(X, Y);
				v.Normalize();
				return v;
			}
		}

		/// *listonly*
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override string ToString()
		{
			return ToString(null, null);
		}

		// Returns a nicely formatted string for this vector.
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
			return UmeString.Format("({0}, {1})", X.ToString(format, formatProvider), Y.ToString(format, formatProvider));
		}

		// used to allow Vector2s to be used as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override int GetHashCode()
		{
			return X.GetHashCode() ^ (Y.GetHashCode() << 2);
		}

		// also required for being able to use Vector2s as keys in hash tables
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public override bool Equals(object other)
		{
			if (!(other is Vector2)) return false;

			return Equals((Vector2)other);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public bool Equals(Vector2 other)
		{
			return X == other.X && Y == other.Y;
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 Reflect(Vector2 inDirection, Vector2 inNormal)
		{
			float factor = -2F * Dot(inNormal, inDirection);
			return new Vector2(factor * inNormal.X + inDirection.X, factor * inNormal.Y + inDirection.Y);
		}

		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 Perpendicular(Vector2 inDirection)
		{
			return new Vector2(-inDirection.Y, inDirection.X);
		}

		// Dot Product of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Dot(Vector2 lhs, Vector2 rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y; }

		// Returns the length of this vector (RO).
		public float magnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return (float)Math.Sqrt(X * X + Y * Y); } }
		// Returns the squared length of this vector (RO).
		public float sqrMagnitude { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return X * X + Y * Y; } }

		// Returns the angle in degrees between /from/ and /to/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Angle(Vector2 from, Vector2 to)
		{
			// sqrt(a) * sqrt(b) = sqrt(a * b) -- valid for real numbers
			float denominator = (float)Math.Sqrt(from.sqrMagnitude * to.sqrMagnitude);
			if (denominator < kEpsilonNormalSqrt)
				return 0F;

			float dot = Mathf.Clamp(Dot(from, to) / denominator, -1F, 1F);
			return (float)Math.Acos(dot) * Mathf.Rad2Deg;
		}

		// Returns the signed angle in degrees between /from/ and /to/. Always returns the smallest possible angle
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float SignedAngle(Vector2 from, Vector2 to)
		{
			float unsigned_angle = Angle(from, to);
			float sign = Mathf.Sign(from.X * to.Y - from.Y * to.X);
			return unsigned_angle * sign;
		}

		// Returns the distance between /a/ and /b/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float Distance(Vector2 a, Vector2 b)
		{
			float diff_x = a.X - b.X;
			float diff_y = a.Y - b.Y;
			return (float)Math.Sqrt(diff_x * diff_x + diff_y * diff_y);
		}

		// Returns a copy of /vector/ with its magnitude clamped to /maxLength/.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 ClampMagnitude(Vector2 vector, float maxLength)
		{
			float sqrMagnitude = vector.sqrMagnitude;
			if (sqrMagnitude > maxLength * maxLength)
			{
				float mag = (float)Math.Sqrt(sqrMagnitude);

				//these intermediate variables force the intermediate result to be
				//of float precision. without this, the intermediate result can be of higher
				//precision, which changes behavior.
				float normalized_x = vector.X / mag;
				float normalized_y = vector.Y / mag;
				return new Vector2(normalized_x * maxLength,
					normalized_y * maxLength);
			}
			return vector;
		}

		// [Obsolete("Use Vector2.sqrMagnitude")]
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static float SqrMagnitude(Vector2 a) { return a.X * a.X + a.Y * a.Y; }
		// [Obsolete("Use Vector2.sqrMagnitude")]
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public float SqrMagnitude() { return X * X + Y * Y; }

		// Returns a vector that is made from the smallest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 Min(Vector2 lhs, Vector2 rhs) { return new Vector2(Mathf.Min(lhs.X, rhs.X), Mathf.Min(lhs.Y, rhs.Y)); }

		// Returns a vector that is made from the largest components of two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 Max(Vector2 lhs, Vector2 rhs) { return new Vector2(Mathf.Max(lhs.X, rhs.X), Mathf.Max(lhs.Y, rhs.Y)); }

		// TODO: Make the Time class

		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Vector2 SmoothDamp(Vector2 current, Vector2 target, ref Vector2 currentVelocity, float smoothTime, float maxSpeed)
		//{
		//	float deltaTime = Time.deltaTime;
		//	return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
		//}
		//
		//[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		//public static Vector2 SmoothDamp(Vector2 current, Vector2 target, ref Vector2 currentVelocity, float smoothTime)
		//{
		//	float deltaTime = Time.deltaTime;
		//	float maxSpeed = Mathf.Infinity;
		//	return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
		//}

		public static Vector2 SmoothDamp(Vector2 current, Vector2 target, ref Vector2 currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
		{
			// Based on Game Programming Gems 4 Chapter 1.10
			smoothTime = Mathf.Max(0.0001F, smoothTime);
			float omega = 2F / smoothTime;

			float x = omega * deltaTime;
			float exp = 1F / (1F + x + 0.48F * x * x + 0.235F * x * x * x);

			float change_x = current.X - target.X;
			float change_y = current.Y - target.Y;
			Vector2 originalTo = target;

			// Clamp maximum speed
			float maxChange = maxSpeed * smoothTime;

			float maxChangeSq = maxChange * maxChange;
			float sqDist = change_x * change_x + change_y * change_y;
			if (sqDist > maxChangeSq)
			{
				var mag = (float)Math.Sqrt(sqDist);
				change_x = change_x / mag * maxChange;
				change_y = change_y / mag * maxChange;
			}

			target.X = current.X - change_x;
			target.Y = current.Y - change_y;

			float temp_x = (currentVelocity.X + omega * change_x) * deltaTime;
			float temp_y = (currentVelocity.Y + omega * change_y) * deltaTime;

			currentVelocity.X = (currentVelocity.X - omega * temp_x) * exp;
			currentVelocity.Y = (currentVelocity.Y - omega * temp_y) * exp;

			float output_x = target.X + (change_x + temp_x) * exp;
			float output_y = target.Y + (change_y + temp_y) * exp;

			// Prevent overshooting
			float origMinusCurrent_x = originalTo.X - current.X;
			float origMinusCurrent_y = originalTo.Y - current.Y;
			float outMinusOrig_x = output_x - originalTo.X;
			float outMinusOrig_y = output_y - originalTo.Y;

			if (origMinusCurrent_x * outMinusOrig_x + origMinusCurrent_y * outMinusOrig_y > 0)
			{
				output_x = originalTo.X;
				output_y = originalTo.Y;

				currentVelocity.X = (output_x - originalTo.X) / deltaTime;
				currentVelocity.Y = (output_y - originalTo.Y) / deltaTime;
			}
			return new Vector2(output_x, output_y);
		}

		// Adds two vectors.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator +(Vector2 a, Vector2 b) { return new Vector2(a.X + b.X, a.Y + b.Y); }
		// Subtracts one vector from another.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator -(Vector2 a, Vector2 b) { return new Vector2(a.X - b.X, a.Y - b.Y); }
		// Multiplies one vector by another.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator *(Vector2 a, Vector2 b) { return new Vector2(a.X * b.X, a.Y * b.Y); }
		// Divides one vector over another.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator /(Vector2 a, Vector2 b) { return new Vector2(a.X / b.X, a.Y / b.Y); }
		// Negates a vector.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator -(Vector2 a) { return new Vector2(-a.X, -a.Y); }
		// Multiplies a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator *(Vector2 a, float d) { return new Vector2(a.X * d, a.Y * d); }
		// Multiplies a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator *(float d, Vector2 a) { return new Vector2(a.X * d, a.Y * d); }
		// Divides a vector by a number.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static Vector2 operator /(Vector2 a, float d) { return new Vector2(a.X / d, a.Y / d); }
		// Returns true if the vectors are equal.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator ==(Vector2 lhs, Vector2 rhs)
		{
			// Returns false in the presence of NaN values.
			float diff_x = lhs.X - rhs.X;
			float diff_y = lhs.Y - rhs.Y;
			return (diff_x * diff_x + diff_y * diff_y) < kEpsilon * kEpsilon;
		}

		// Returns true if vectors are different.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static bool operator !=(Vector2 lhs, Vector2 rhs)
		{
			// Returns true in the presence of NaN values.
			return !(lhs == rhs);
		}

		// Converts a [[Vector3]] to a Vector2.
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector2(Vector3 v)
		{
			return new Vector2(v.X, v.Y);
		}

		// Converts a Vector2 to a [[Vector3]].
		[MethodImpl(MethodImplOptionsEx.AggressiveInlining)]
		public static implicit operator Vector3(Vector2 v)
		{
			return new Vector3(v.X, v.Y, 0);
		}

		static readonly Vector2 zeroVector = new Vector2(0F, 0F);
		static readonly Vector2 oneVector = new Vector2(1F, 1F);
		static readonly Vector2 upVector = new Vector2(0F, 1F);
		static readonly Vector2 downVector = new Vector2(0F, -1F);
		static readonly Vector2 leftVector = new Vector2(-1F, 0F);
		static readonly Vector2 rightVector = new Vector2(1F, 0F);
		static readonly Vector2 positiveInfinityVector = new Vector2(float.PositiveInfinity, float.PositiveInfinity);
		static readonly Vector2 negativeInfinityVector = new Vector2(float.NegativeInfinity, float.NegativeInfinity);


		// Shorthand for writing @@Vector2(0, 0)@@
		public static Vector2 zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return zeroVector; } }
		// Backward-compatible alias used by older scripts.
		public static Vector2 Zero { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return zero; } }
		// Shorthand for writing @@Vector2(1, 1)@@
		public static Vector2 one { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return oneVector; } }
		// Shorthand for writing @@Vector2(0, 1)@@
		public static Vector2 up { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return upVector; } }
		// Shorthand for writing @@Vector2(0, -1)@@
		public static Vector2 down { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return downVector; } }
		// Shorthand for writing @@Vector2(-1, 0)@@
		public static Vector2 left { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return leftVector; } }
		// Shorthand for writing @@Vector2(1, 0)@@
		public static Vector2 right { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return rightVector; } }
		// Shorthand for writing @@Vector2(float.PositiveInfinity, float.PositiveInfinity)@@
		public static Vector2 positiveInfinity { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return positiveInfinityVector; } }
		// Shorthand for writing @@Vector2(float.NegativeInfinity, float.NegativeInfinity)@@
		public static Vector2 negativeInfinity { [MethodImpl(MethodImplOptionsEx.AggressiveInlining)] get { return negativeInfinityVector; } }

		// *Undocumented*
		public const float kEpsilon = 0.00001F;
		// *Undocumented*
		public const float kEpsilonNormalSqrt = 1e-15f;
	}
}
