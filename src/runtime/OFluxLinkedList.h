#ifndef _OFLUX_LINKEDLIST
#define _OFLUX_LINKEDLIST
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

#include <cassert>
#include <stdlib.h>
#include "OFlux.h"
#include "OFluxSharedPtr.h"

namespace oflux {

template<class C>
class Node {
public:
	enum { create_does_new = 1, is_removable = 0, can_share_ptr = 0 };
	Node(C * content, Node<C> * next = NULL)
		: _next(next)
		, _content(content)
		{}
	inline Node<C> * next() { return _next; }
	inline void next(Node<C> *n) { _next = n; }
	inline C * content() { return _content; }
	static Node<C> * create(C * c) { return new Node<C>(c); }
	static void destroy(Node<C> * n) { delete n; }

	inline Node<C> ** _next_location() { return &_next; }
protected:
	Node<C> * _next;
	C *       _content;
};

template<class C>
class InheritedNode {
public:
	enum { create_does_new = 0, is_removable = 0, can_share_ptr = 0 };
	InheritedNode(InheritedNode<C> * next = NULL)
		: _next(next)
		{}
	InheritedNode(C *,InheritedNode<C> * next = NULL)
		: _next(next)
		{}
	inline InheritedNode<C> * next() { return _next; }
	inline void next(InheritedNode<C> *n) { _next = n; }
	inline C * content() { return static_cast<C *>(this); }
	static InheritedNode<C> * create(C * c) { return dynamic_cast<InheritedNode<C> *>(c); }
	static void destroy(InheritedNode<C> * n) {} // not my beer
	inline InheritedNode<C> ** _next_location() { return &_next; }
protected:
	InheritedNode<C> * _next;
};

template<class C>
class SharedPtrNode {
public:
	enum { create_does_new = 1, is_removable = 0, can_share_ptr = 1 };
	SharedPtrNode(C * content, SharedPtrNode<C> * next = NULL)
		: _next(next)
		, _content(content)
		{}
	SharedPtrNode(shared_ptr<C> & content, SharedPtrNode<C> * next = NULL)
		: _next(next)
		, _content(content)
		{}
	inline SharedPtrNode<C> * next() { return _next; }
	inline void next(SharedPtrNode<C> *n) { _next = n; }
	inline shared_ptr<C> & shared_content() { return _content; }
	inline C * content() { return _content.get(); }
	static SharedPtrNode<C> * create(C * c) 
		{ assert(0); return NULL; }
	static SharedPtrNode<C> * create(shared_ptr<C> & c) 
		{ return new SharedPtrNode<C>(c); }
	static void destroy(SharedPtrNode<C> * n) { delete n; }

	inline SharedPtrNode<C> ** _next_location() { return &_next; }
protected:
	SharedPtrNode<C> *   _next;
	shared_ptr<C> _content;
};

template<class C, class N>
class Removable;

template<class C, class N >
class LinkedListRemovable;

template<class C, class N=Node<C> >
class Removable : public N {
public:
	friend class LinkedListRemovable<C, Removable<C,N> >;
	enum { is_removable = 1 };
	Removable(Removable<C,N> ** prev=NULL,N * next = NULL)
		: N(next)
		, _previous(prev)
		{}
	Removable(C * c,Removable<C,N> ** prev=NULL,N * next = NULL)
		: N(c,next)
		, _previous(prev)
		{}
	Removable(shared_ptr<C> & c,Removable<C,N> ** prev=NULL,N * next = NULL)
		: N(c,next)
		, _previous(prev)
		{}
	inline Removable<C,N> * next() 
		{ return static_cast<Removable<C,N> *>(static_cast<N *>(this)->next()); }
	inline void next(Removable<C,N> *n) 
		{ static_cast<N *>(this)->next(n); }
protected:
	inline bool remove() 
		{ 
                        bool res = false;
			if(_previous) {
                                res = true;
				*_previous = static_cast<Removable<C,N> *>(this->next()); 
				if(*_previous) {
					(*_previous)->previous(_previous);
				}
			}
			_previous = NULL; 
			this->next(NULL); 
                        return res;
		}
	inline Removable<C,N> ** previous() { return _previous; }
	inline void previous(Removable<C,N> ** p) { _previous = p; }
	inline Removable<C,N> ** next_location() 
		{ 
			return reinterpret_cast<Removable<C,N> **>(this->_next_location()); 
		}
	static Removable<C,N> * create(C * c)
		{
			if(N::create_does_new) {
				return new Removable<C,N>(c,NULL);
			} else {
				return static_cast<Removable<C,N> *>(N::create(c));
			}
		}
	static Removable<C,N> * create(shared_ptr<C> & c)
		{
			CompileTimeAssert<N::create_does_new && N::can_share_ptr> __attribute__((unused)) cta;
			return new Removable<C,N>(c,NULL);
		}
private:
	Removable<C,N> ** _previous;
};

template<class C, class N=Node<C> >
class LinkedList {
public:
        typedef C content_type;
	typedef void (*iter_fun)(C *);
	typedef void * (*fold_fun)(void *, C *);

	LinkedList()
		: _head(NULL)
		{}
	~LinkedList()
		{
			N * n = _head;
			while(n) {
				N::destroy(n);
				n = static_cast<N *> (n->next());
			}
		}
	inline void insert_front(N * n)
		{
			assert(n->next() == NULL);
			n->next(_head);
			_head = n;
		}
	inline N * insert_front(C * c)
		{
			N * n = N::create(c);
			insert_front(n);
			return n;
		}
	inline N * insert_front(shared_ptr<C> & csp)
		{
			CompileTimeAssert<N::can_share_ptr> __attribute__((unused)) cta;
			N * n = N::create(csp);
			insert_front(n);
			return n;
		}
	inline void iter(iter_fun i_f)
		{
			N * n = _head;
			while(n) {
				(*i_f)(n->content());
				n = static_cast<N *> (n->next());
			}
		}
	inline void * fold(void * fixed, fold_fun f_f)
		{
			N * n = _head;
			while(n) {
				fixed = (*f_f)(fixed,n->content());
				n = n->next();
			}
			return fixed;
		}
	inline N * first() { return _head; }
protected:
	N * _head;
};

template<class C, class F, void (*Ff)(F &,C &) >
inline void * _linkedlist_fold(void * fp, C * c)
{
	F * fptr = static_cast<F *>(fp);
	(*Ff)(*fptr,*c);
	return fptr;
}

template<class C, class F, void (*Ff)(F &,C &), class N >
inline void linkedlist_fold(F & f, LinkedList<C,N> & list)
{
	list.fold(&f,_linkedlist_fold<C,F,Ff>);
}

template<class C, class N=Removable<C, Node<C> > >
class LinkedListRemovable : public LinkedList<C,N> {
public:
	LinkedListRemovable()
		: LinkedList<C,N>()
		, _tail(NULL)
		{}
	inline void insert_front(N * n)
		{
			// insist you are not in another list
			assert(n->next() == NULL && n->previous() == NULL);
			if(_tail == NULL) {
				_tail = n->next_location();
			}
			N ** head_loc = &(this->_head);
			n->previous(head_loc);
			if(*head_loc) {
				(*head_loc)->previous(n->next_location());
			}
			n->next(*head_loc);
			*head_loc = n;
		}
	inline N * insert_front(C * c)
		{
			N * n = N::create(c);
			LinkedListRemovable<C,N>::insert_front(n);
			return n;
		}
	inline N * insert_front(shared_ptr<C> & csp)
		{
			CompileTimeAssert<N::can_share_ptr> __attribute__((unused)) cta;
			N * n = N::create(csp);
			LinkedListRemovable<C,N>::insert_front(n);
			return n;
		}
	inline void insert_back(N * n)
		{
			assert(n->next() == NULL && n->previous() == NULL);
			if(_tail == NULL) { // if empty
				insert_front(n);
			} else {
				n->previous(_tail);
				*_tail = n;
				_tail = n->next_location();
			}
		}
	inline N * insert_back(C * c)
		{
			N* n = N::create(c);
			LinkedListRemovable<C,N>::insert_back(n);
			return n;
		}
	inline N * insert_back(shared_ptr<C> & csp)
		{
			CompileTimeAssert<N::can_share_ptr> __attribute__((unused)) cta;
			N * n = N::create(csp);
			LinkedListRemovable<C,N>::insert_back(n);
			return n;
		}
	inline bool remove(N * n) 
		{
			if(_tail == n->next_location()) {
				_tail = n->previous();
			}
			bool res = n->remove();
			if(_tail == &(this->_head)) {
				// empty again
				_tail = NULL;
				this->_head = NULL;
			}
			N::destroy(n);
                        return res;
		}
private:
	CompileTimeAssert<N::is_removable> _ct;
	N **                               _tail;
}; 

} // namespace

#endif // _OFLUX_LINKEDLIST
