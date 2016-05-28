//-----------------------------------------------------------------------------
// MathExtension: A C++ Implementation of a ton of TorqueScript math functions
// Most code by Whirligig
// Ported to C++ by Jeff/HiGuy
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Copyright(c) 2015 Whirligig231
// Copyright(c) 2015 Jeff Hutchinson
// Copyright(c) 2015 HiGuy Smith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include <PluginLoader/PluginInterface.h>
#include <TorqueLib/TGE.h>
#include <TorqueLib/math/mMath.h>

#include "MathExtension.h"
	
/**
 * Gets the 2D point in the view from (-1,-1) to (1,1) corresponding to a point in 2D space
 * @arg transform The current camera transformation
 * @arg worldSpace The point to project, in world space
 * @arg fov The field of view
 * @arg recurse Internally used for backwards angles.
 */
Point2F getGuiSpace(const MatrixF &transform, const Point3F &worldSpace, const F32 fov, bool recurse)
{
	//Default game attributes
	const Point2I extent = TGE::Canvas->getExtent();
	const F32 aspect = (F32)extent.x / (F32)extent.y;
	const Point3F forward = transform.getForwardVector();

	Point3F diff = worldSpace - transform.getPosition();
	Point3F projOut = VectorRej(diff, forward);

	MatrixF mat = transform;
	mat.inverse();
	mat.mulV(projOut);
	Point3F projRot = projOut; // keeping it consitent with whirligig code

	const F32 dist = VectorProjLen(diff, forward);

	if (dist < 0.0f && !recurse) {
		//Behind you, rotate the point and do some math
		Point3F pos = worldSpace + ((transform.getPosition() - worldSpace) * 2.0f);
		Point2F result = getGuiSpace(transform, pos, fov, true) * -1;
		//Since it's behind you, we need to scale to the edge of the screen
		//Sqrt 2 will get us out of bounds, even in the corners
		result.normalizeSafe();
		result *= mSqrt(2.0f);
		return result;
	}

	F32 slopeP = mTan(fov / 2.0f*M_PI_F / 180.0f);
	F32 slopeX = projRot.x / dist;
	F32 slopeY = projRot.z / dist;
	F32 projX = slopeX / slopeP;
	F32 projY = slopeY / slopeP;
	projY *= aspect;
	Point2F result(projX, -projY);
	return result;
}

/**
 * Gets the 2D point in the view from (-1,-1) to (1,1) corresponding to a point in 2D space
 * @arg transform The current camera transformation
 * @arg worldSpace The point to project, in world space
 * @arg fov The field of view
 * @return The 2D position for the point.
 */
ConsoleFunction(getGuiSpace, const char*, 4, 4, "getGuiSpace(MatrixF transform, Point3F worldSpace, F32 fov)") {
	MatrixF mat = StringMath::scan<MatrixF>(argv[1]);
	Point3F worldSpace = StringMath::scan<Point3F>(argv[2]);
	F32 fov = StringMath::scan<F32>(argv[3]);
	Point2F gui = getGuiSpace(mat, worldSpace, fov);
	return StringMath::print(gui);
}

/**
 * Projects vector u onto vector v.
 * @arg u The vector to project.
 * @arg v The vector on which to project.
 * @return The projected vector.
 */
ConsoleFunction(VectorProj, const char*, 3, 3, "VectorProj(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	Point3F vec = VectorProj(u, v);
	return StringMath::print(vec);
}

/**
 * Gets the length of the projection of vector u onto vector v.
 * @arg u The vector to project.
 * @arg v The vector on which to project.
 * @return The lenght of the projection vector.
 */
ConsoleFunction(VectorProjLen, F32, 3, 3, "VectorProjLen(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	return VectorProjLen(u, v);
}

/**
 * Rejects vector u onto vector v (component of u perpendicular to v).
 * @arg u The vector to reject.
 * @arg v The vector on which to reject.
 * @return The rejected vector.
 */
ConsoleFunction(VectorRej, const char*, 3, 3, "VectorRej(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	Point3F vec = VectorRej(u, v);
	return StringMath::print(vec);
}

/**
 * Calculates the inverse of a matrix.
 * @arg mat The matrix to inverse.
 * @return The inverted matrix.
 */
ConsoleFunction(MatrixInverse, const char *, 2, 2, "MatrixInverse(MatrixF mat)") {
	MatrixF mat = StringMath::scan<MatrixF>(argv[1]);
	MatrixF ret = MatrixInverse(mat);
	return StringMath::print(ret);
}

/**
 * Rotates one vector by an axis and angle.
 * @arg vec The vector to rotate.
 * @arg axis The axis about which to rotate the vector.
 * @arg angle The angle by which the vector is rotated.
 * @return The rotated vector.
 */
ConsoleFunction(VectorRotate, const char *, 4, 4, "VectorRotate(Point3F vec, Point3F axis, F32 angle)") {
	Point3F vec = StringMath::scan<Point3F>(argv[1]);
	Point3F axis = StringMath::scan<Point3F>(argv[2]);
	F32 angle = StringMath::scan<F32>(argv[3]);
	Point3F ret = VectorRotate(vec, axis, angle);
	return StringMath::print(ret);
}

/**
 * Crosses one vector by another, with regard to special edge-case angles (180 degrees).
 * @arg u The first vector to cross.
 * @arg v The second vector to cross.
 * @return The cross-product of the vectors.
 */
ConsoleFunction(VectorCrossSpecial, const char *, 3, 3, "VectorCrossSpecial(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	Point3F ret = VectorCrossSpecial(u, v);
	return StringMath::print(ret);
}

/**
 * Find the angle between two vectors.
 * @arg u The first vector.
 * @arg v The second vector.
 * @return The angle between u and v.
 */
ConsoleFunction(VectorAngle, F32 , 3, 3, "VectorAngle(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	F32 ret = VectorAngle(u, v);
	return ret;
}

/**
 * Find the axis of rotation between two vectors.
 * @arg u The first vector.
 * @arg v The second vector.
 * @return The axis of rotation between u and v.
 */
ConsoleFunction(VectorAxis, const char *, 3, 3, "VectorAxis(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	Point3F ret = VectorAxis(u, v);
	return StringMath::print(ret);
}

/**
 * Find the axis-angle rotation between two vectors.
 * @arg u The first vector.
 * @arg v The second vector.
 * @return The axis-angle rotation between u and v.
 */
ConsoleFunction(VectorRot, const char *, 3, 3, "VectorRot(Point3F u, Point3F v)") {
	Point3F u = StringMath::scan<Point3F>(argv[1]);
	Point3F v = StringMath::scan<Point3F>(argv[2]);
	AngAxisF ret = VectorRot(u, v);
	return StringMath::print(ret);
}

/**
 * Divide one matrix by another.
 * @arg mat1 The dividend matrix.
 * @arg mat2 The divisor matrix.
 * @return The quotient of the division mat1 / mat2.
 */
ConsoleFunction(MatrixDivide, const char *, 3, 3, "MatrixDivide(MatrixF mat1, MatrixF mat2)") {
	MatrixF mat1 = StringMath::scan<MatrixF>(argv[1]);
	MatrixF mat2 = StringMath::scan<MatrixF>(argv[2]);
	MatrixF ret = MatrixDivide(mat1, mat2);
	return StringMath::print(ret);
}

/**
 * Interpolate between one axis-angle rotation and another.
 * @arg rot1 The first rotation.
 * @arg rot2 The second rotation.
 * @arg t The current state of interpolation (0.0 - 1.0).
 * @return The partial rotation from rot1 to rot2.
 */
ConsoleFunction(RotInterpolate, const char *, 4, 4, "RotInterpolate(AngAxisF rot1, AngAxisF rot2, F32 t)") {
	AngAxisF rot1 = StringMath::scan<AngAxisF>(argv[1]);
	AngAxisF rot2 = StringMath::scan<AngAxisF>(argv[2]);
	F32 t = StringMath::scan<F32>(argv[3]);
	AngAxisF ret = RotInterpolate(rot1, rot2, t);
	return StringMath::print(ret);
}
/**
 * Interpolate between one matrix and another.
 * @arg mat1 The first matrix.
 * @arg mat2 The second matrix.
 * @arg t The current state of interpolation (0.0 - 1.0).
 * @return The partial interpolation from rot1 to rot2.
 */
ConsoleFunction(MatInterpolate, const char *, 4, 4, "MatInterpolate(MatrixF mat1, MatrixF mat2, F32 t)") {
	MatrixF mat1 = StringMath::scan<MatrixF>(argv[1]);
	MatrixF mat2 = StringMath::scan<MatrixF>(argv[2]);
	F32 t = StringMath::scan<F32>(argv[3]);
	MatrixF ret = MatInterpolate(mat1, mat2, t);
	return StringMath::print(ret);
}

/**
 * Remove Torque's nasty scientific notation from a number. This just casts it to a string.
 * @arg val The number to process.
 * @return A string containing that number, without scientific notation.
 */
ConsoleFunction(removeScientificNotation, const char *, 2, 2, "removeScientificNotation(F32 val)") {
	F64 val = StringMath::scan<F64>(argv[1]);
	return StringMath::print(val);
}

/**
 * Calculates the factorial of a number.
 * @arg val The number whose factorial to get.
 * @return The factorial of val.
 */
ConsoleFunction(mFact, const char *, 2, 2, "mFact(U32 val)") {
	U64 val = StringMath::scan<U64>(argv[1]);
	U64 ret = mFact(val);
	return StringMath::print(ret);
}

/**
 * Constrain a number within the bounds of a minimum and maximum.
 * @arg n The number to constrain.
 * @arg min The minimum possible value for n.
 * @arg max The maximum possible value for n.
 * @return The constrained value for n from min to max.
 */
ConsoleFunction(mClamp, F32, 4, 4, "mClamp(F32 n, F32 min, F32 max)") {
	F32 n = StringMath::scan<F32>(argv[1]);
	F32 min = StringMath::scan<F32>(argv[2]);
	F32 max = StringMath::scan<F32>(argv[3]);
	F32 ret = mClampF(n, min, max);
	return ret;
}