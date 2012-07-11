#ifndef IO_CONVERSION_H
#define IO_CONVERSION_H
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
