//-----------------------------------------------------------------------------
// Copyright (c) 2015 The Platinum Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

/// These functions let us do big math in TorqueScript without
/// losing precision :D

#include "stdafx.h"

#include <cstdio>
#include <TorqueLib/math/mMath.h>
#include <TorqueLib/math/mathUtils.h>
#include <TorqueLib/TGE.h>

// _declspec(deprecated)
#pragma warning(disable : 4996)

/**
 * Used for defining a new floating point math64 function.
 * @arg name The name of the function.
 * @arg expr The expression to evaluate for the function.
 */
#define math64(name, expr) \
ConsoleFunction(name, const char*, 3, 3, #name "(a, b)") { \
	F64 a;\
	F64 b;\
	sscanf(argv[1], "%lf", &a);\
	sscanf(argv[2], "%lf", &b);\
	expr;\
	char* ret = TGE::Con::getReturnBuffer(64);\
	sprintf(ret, "%.7g", a);\
	return ret;\
}

/**
 * Used for defining a new rounding math64 function (only takes one argument).
 * @arg name The name of the function.
 * @arg expr The expression to evaluate for the function.
 */
#define round64(name, expr) \
ConsoleFunction(name, const char*, 2, 2, #name "(a)") { \
	F64 a; \
	sscanf(argv[1], "%lf", &a);\
	expr; \
	char* ret = TGE::Con::getReturnBuffer(64); \
	sprintf(ret, "%.7g", a); \
	return ret; \
}

/**
 * Used for defining a new integer math64 function.
 * @arg name The name of the function.
 * @arg expr The expression to evaluate for the function.
 */
#define math64_int(name, expr) \
ConsoleFunction(name, const char*, 3, 3, #name "(a, b)") { \
	S64 a = (S64)atoll(argv[1]); \
	S64 b = (S64)atoll(argv[2]); \
	expr; \
	char* ret = TGE::Con::getReturnBuffer(64); \
	sprintf(ret, "%lld", a); \
	return ret; \
}

/**
 * Basic floating point math functions:
 *  - Addition
 *  - Subtraction
 *  - Multiplication
 *  - Division
 *  - Exponentiation
 *  - Modulus
 */
math64(add64, a += b);
math64(sub64, a -= b);
math64(mult64, a *= b);
math64(div64, a /= b);
math64(pow64, a = pow(a, b));
math64(mod64, a = fmod(a, b));

/**
 * Rounding functions:
 *  - Floor (round down)
 *  - Ceiling (round up)
 */
round64(floor64, a = floor(a));
round64(ceil64, a = ceil(a));

/**
 * Basic integer math functions:
 *  - Addition
 *  - Subtraction
 *  - Multiplication
 *  - Division
 *  - Exponentiation
 *  - Modulus
 */
math64_int(add64_int, a += b);
math64_int(sub64_int, a -= b);
math64_int(mult64_int, a *= b);
math64_int(div64_int, a /= b);
math64_int(pow64_int, a = (S64)pow(a, b));
math64_int(mod64_int, a = (S64)fmod(a, b));