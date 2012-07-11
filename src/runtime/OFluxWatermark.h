#ifndef _OFLUX_WATERMARK
#define _OFLUX_WATERMARK
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

namespace oflux {

#define OFLUX_WATERMARKED_SAMPLES 10

// replaces IntegerCounter when statistics are needed
class WatermarkedCounter : public IntegerCounter {
public:
	struct Sample {
		Sample() 
			: min(0)
			, max(0)
			{}
		void reset(int c) { min = c; max = c; }

		int min;
		int max;
	};
	WatermarkedCounter()
		: _index(0)
		{}
	inline WatermarkedCounter & operator++() // ++c
		{
			++_c;
			_samples[_index].max = std::max(_samples[_index].max, _c);
			return *this;
		}
	inline WatermarkedCounter & operator--() // --c
		{
			--_c;
			_samples[_index].min = std::min(_samples[_index].min, _c);
			return *this;
		}
	void next_sample() 
		{ 
			_index = (_index+1)%OFLUX_WATERMARKED_SAMPLES; 
			_samples[_index].reset(_c);
		}
	int min() const
		{
			int amin = _samples[0].min;
			for(int i = 1; i < OFLUX_WATERMARKED_SAMPLES; i++) {
				amin = std::min(amin, _samples[i].min);
			}
			return amin;
		}
	int max() const
		{
			int amax = _samples[0].max;
			for(int i = 1; i < OFLUX_WATERMARKED_SAMPLES; i++) {
				amax = std::max(amax, _samples[i].max);
			}
			return amax;
		}
	void log_snapshot(char * str, int sz)
		{
			snprintf(str,sz,"[summary] + last 3 samples (min/max) [%d,%d] %d/%d %d/%d %d/%d",
				min(), max(),
				_samples[_index].min, _samples[_index].max,
				_samples[(_index-1+OFLUX_WATERMARKED_SAMPLES)%OFLUX_WATERMARKED_SAMPLES].min, 
					_samples[(_index-1+OFLUX_WATERMARKED_SAMPLES)%OFLUX_WATERMARKED_SAMPLES].max,
				_samples[(_index-2+OFLUX_WATERMARKED_SAMPLES)%OFLUX_WATERMARKED_SAMPLES].min, 
					_samples[(_index-2+OFLUX_WATERMARKED_SAMPLES)%OFLUX_WATERMARKED_SAMPLES].max);
		}
private:
	int    _index;
	Sample _samples[OFLUX_WATERMARKED_SAMPLES];
};

} // namespace

#endif // _OFLUX_WATERMARK
