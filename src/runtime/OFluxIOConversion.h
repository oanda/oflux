#ifndef IO_CONVERSION_H
#define IO_CONVERSION_H

#include "OFlux.h"

/**
 * @file OFluxIOConversion.h
 * @author Mark Pichora
 * These structures can hold the converter logic to get you from
 * a particular output type into another (compatible via subset)
 * (input) type
 */

namespace oflux {

//forward declaration
template<typename ToClass,typename FromClass /*,int todepth,int fromdepth*/ > 
void copy_to(ToClass *, const FromClass *);

template<typename ResultClass>
class IOConversionBase {
public:
        IOConversionBase() {}
        virtual ~IOConversionBase() {}
        virtual const ResultClass * value() const = 0;
};

template<typename GivenClass>
class TrivialIOConversion : public IOConversionBase<GivenClass> {
public:
        TrivialIOConversion(const GivenClass * gclass)
                : _gclass(gclass)
                {}
        virtual const GivenClass * value() const { return _gclass; }
private:
        const GivenClass * _gclass;
};

template<typename TargetClass,typename GivenClass>
class RealIOConversion : public IOConversionBase<TargetClass> {
public:
        RealIOConversion(const GivenClass * given)
                { copy_to<TargetClass,GivenClass/*,0,0*/ >(&_target,given); }
        virtual ~RealIOConversion() {}
        virtual const TargetClass * value() const { return &_target; }
private:
        TargetClass _target;
};

template<typename GivenClass>
void * create_trivial_io_conversion(const void *out)
{
        return new TrivialIOConversion<GivenClass>(reinterpret_cast<const GivenClass *> (out));
}

template<typename TargetClass, typename GivenClass>
void * create_real_io_conversion(const void *out) 
{
        return new RealIOConversion<TargetClass,GivenClass>(reinterpret_cast<const GivenClass *>(out));
}

};




#endif // IO_CONVERSION_H
