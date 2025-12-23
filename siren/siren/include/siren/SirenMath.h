#pragma once
#include <cmath>

namespace siren {

	struct Vector3 {
		float x;
		float y;
		float z;

		Vector3() 
			: x(0.0f), y(0.0f), z(0.0f) {
		}

		Vector3(float x, float y, float z) 
			: x(x), y(y), z(z) {
		}

		Vector3 operator-(const Vector3& other) const {
			return Vector3(x - other.x, y - other.y, z - other.z);
		}

		float operator*(const Vector3& other) const {
			return (x * other.x) + (y * other.y) + (z * other.z);
		}

		Vector3 operator*(float scalar) const {
			return Vector3(x * scalar, y * scalar, z * scalar);
		}

		float length() const {
			return sqrt((x * x) + (y * y) + (z * z));
		}

		void normalize() {
			float length = this->length();
			if (length > 0.0000001f) {
				x = x / length;
				y = y / length;
				z = z / length;
			}
		}
	};

	inline Vector3 crossMultiply(const Vector3& v1, const Vector3& v2) {
		Vector3 result(
			(v1.y * v2.z) - (v1.z * v2.y),
			(v1.z * v2.x) - (v1.x * v2.z),
			(v1.x * v2.y) - (v1.y * v2.x)
		);
		return result;
	}
}
