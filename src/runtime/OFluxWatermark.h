#ifndef _OFLUX_WATERMARK
#define _OFLUX_WATERMARK

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
