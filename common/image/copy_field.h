/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "../hardware/cpuid.h"

namespace caspar{ namespace common{ namespace image{
	
void copy_field_SSE2	 (unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
void copy_field_REF	 (unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);

void copy_fieldParallel_SSE2	 (unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
void copy_fieldParallel_REF	 (unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);

typedef void(*copy_field_fun)(unsigned char* pDest, const unsigned char* pSrc, size_t fieldIndex, size_t width, size_t height);
copy_field_fun get_copy_field_fun(SIMD simd = REF);

}}} 

