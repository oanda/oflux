#ifndef OFLUX_ENUMERATOR_H
#define OFLUX_ENUMERATOR_H

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
	inline EnumeratorRef<K> & 
	operator=(const EnumeratorRef<K> & er)
	{
		EnumeratorRefBase<K>::clone(er);
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
