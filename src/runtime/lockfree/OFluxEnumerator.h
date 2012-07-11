#ifndef OFLUX_ENUMERATOR_H
#define OFLUX_ENUMERATOR_H
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

/**
 * @file OFluxEnumerator.h
 * @author Mark Pichora
 * Enumerator (Iterator) pattern templates for easy datastructure walking.
 * This interface is used to help abstract the internal details of a data
 * structure when writing code that implements a walker.
 */

#include <exception>
#include <string>

namespace oflux {

class EnumeratorException : public std::exception {
public:
	EnumeratorException(const char * msg) throw() 
		: _message("EnumeratorException: ")
		{ _message += msg; }
	virtual ~EnumeratorException() throw() {}
	virtual const char * what() throw ()
	{ return _message.c_str(); }
private:
	std::string _message;
};

template< typename K >
class Enumerator {
public:
	virtual bool 
	next(const K * & k) = 0;
};


template< typename K
	, typename V >
class KeyValueEnumerator {
public:
	virtual bool 
	next(const K * & kptr, const V * & vptr) = 0;
};

template< typename E >
class RefCountable : public E {
public:
	RefCountable()
		: _count(0)
	{}

	size_t increment() { return ++_count; }
	size_t decrement() { return --_count; }
private:
	size_t _count;
};

template< typename E >
class Measurable : public E {
public:
	virtual size_t length_left() const = 0;
};


template< typename E >
class EnumeratorRefBase {
public:
	typedef E UnderEnumerator;
	EnumeratorRefBase(E * e)
		: _enumerator(e)
	{ _enumerator->increment(); }
	~EnumeratorRefBase()
	{ close(); }
protected:
	void
	close()
	{
		if(_enumerator && _enumerator->decrement() == 0) {
			delete _enumerator;
		}
		_enumerator = NULL;
	}
	void clone(const EnumeratorRefBase<E> & erb)
	{
		close();
		_enumerator = erb._enumerator;
		erb._enumerator->increment();
	}
protected:
	E * _enumerator;
};

template<typename K>
class EnumeratorRef : public EnumeratorRefBase< RefCountable< Enumerator<K> > > {
public:
	EnumeratorRef(RefCountable< Enumerator<K> > * e)
		: EnumeratorRefBase< RefCountable< Enumerator<K> > >(e)
	{}
	EnumeratorRef(const EnumeratorRef<K> & er)
	{
		*this = er;
	}
	inline EnumeratorRef<K> & 
	operator=(const EnumeratorRef<K> & er)
	{
		EnumeratorRefBase< RefCountable< Enumerator<K> > >::clone(er);
		return *this;
	}
	inline bool
	next(const K * & kp) {
		if(EnumeratorRefBase< RefCountable< Enumerator<K> > >::_enumerator) {
			return EnumeratorRefBase< RefCountable< Enumerator<K> > >::_enumerator->next(kp);
		}
		return false;
	}
};

template< typename K >
class EnumeratorRefMeasurable : public EnumeratorRef<K> {
public:
	EnumeratorRefMeasurable(Measurable< RefCountable< Enumerator<K> > > *e)
		: EnumeratorRef<K>(e)
	{}
	inline size_t
	length_left() const
	{
		if(EnumeratorRefBase< RefCountable< Enumerator<K> > >::_enumerator) {
			return reinterpret_cast<Measurable< RefCountable< Enumerator<K> > > * >(EnumeratorRefBase< RefCountable< Enumerator<K> > >::_enumerator)->length_left();
		}
	}
	inline EnumeratorRefMeasurable<K> & 
	operator=(const EnumeratorRefMeasurable<K> & er)
	{
		EnumeratorRefBase<K>::clone(er);
		return *this;
	}
};

template< typename K
	, typename V >
class KVEnumeratorRef : public EnumeratorRefBase< RefCountable< KeyValueEnumerator<K,V> > > {
public:
	KVEnumeratorRef(RefCountable< KeyValueEnumerator<K,V> > * e)
		: EnumeratorRefBase< RefCountable< KeyValueEnumerator<K,V> > >(e)
	{}
	KVEnumeratorRef(const KVEnumeratorRef<K,V> & er)
		: EnumeratorRefBase< RefCountable< KeyValueEnumerator<K,V> > >(er._enumerator)
	{}
	inline KVEnumeratorRef<K,V> & 
	operator=(const KVEnumeratorRef<K,V> & er)
	{
		EnumeratorRefBase< RefCountable< KeyValueEnumerator<K,V> > >::clone(er);
		return *this;
	}
	inline bool
	next(const K * & kp, const V * & vp) {
		if(EnumeratorRefBase< RefCountable< KeyValueEnumerator<K,V> > >::_enumerator) {
			return EnumeratorRefBase< RefCountable< KeyValueEnumerator<K,V> > >::_enumerator->next(kp, vp);
		}
		return false;
	}
};

} // namespace oflux

#endif // ENUMERATOR_H
