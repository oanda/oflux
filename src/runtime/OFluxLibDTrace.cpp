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
#include "OFluxLibDTrace.h"

// These thunks are needed since dtrace and -O do not get along in gcc

#ifdef HAS_DTRACE
# include "ofluxprobe.h"
namespace oflux {


int access_dtrace()
{
	static int ok_keep_dtrace_symbols __attribute__((unused)) = access_dtrace();

	char * empty = const_cast<char *>("");
	PUBLIC_NODE_START(NULL,empty,0,0);
	PUBLIC_NODE_DONE(NULL,empty);
	PUBLIC_NODE_HAVEALLGUARDS(NULL,empty);
	PUBLIC_NODE_ACQUIREGUARDS(NULL,empty);
	OFLUX_EVENT_BORN(NULL,empty);
	OFLUX_EVENT_DEATH(NULL,empty);
	//OFLUX_SHIM_CALL(empty);
	//OFLUX_SHIM_WAIT(empty);
	//OFLUX_SHIM_RETURN(empty);
	return empty == NULL;
}

void PUBLIC_GUARD_ACQUIRE(const char *A,const char *B,int C)
{
	OFLUX_GUARD_ACQUIRE(const_cast<char *>(A), const_cast<char *>(B),C);
}

void PUBLIC_GUARD_WAIT(const char *A,const char *B,int C)
{
	OFLUX_GUARD_WAIT(const_cast<char *>(A), const_cast<char *>(B),C);
}

void PUBLIC_GUARD_RELEASE(const char *A,const char *B,int C)
{
	OFLUX_GUARD_RELEASE(const_cast<char *>(A), const_cast<char *>(B),C);
}


void PUBLIC_FIFO_PUSH(const void *A,const char *B)
{
	OFLUX_FIFO_PUSH(const_cast<void *>(A), const_cast<char *>(B));
}

void PUBLIC_FIFO_POP(const void *A,const char *B)
{
	OFLUX_FIFO_POP(const_cast<void *>(A), const_cast<char *>(B));
}


void PUBLIC_NODE_START(const void *A,const char * B,int C,int D)
{
	OFLUX_NODE_START(const_cast<void *>(A), const_cast<char *>(B),C,D);
}

void PUBLIC_NODE_DONE(const void *A,const char *B)
{
	OFLUX_NODE_DONE(const_cast<void *>(A), const_cast<char *>(B));
}

void PUBLIC_NODE_HAVEALLGUARDS(const void *A, const char *B)
{
	OFLUX_NODE_HAVEALLGUARDS(const_cast<void *>(A), const_cast<char *>(B));
}

void PUBLIC_NODE_ACQUIREGUARDS(const void *A, const char *B)
{
	OFLUX_NODE_ACQUIREGUARDS(const_cast<void *>(A), const_cast<char *>(B));
}


void PUBLIC_EVENT_BORN(const void *A,const char * B)
{
	OFLUX_EVENT_BORN(const_cast<void *>(A), const_cast<char *>(B));
}

void PUBLIC_EVENT_DEATH(const void *A,const char * B)
{
	OFLUX_EVENT_DEATH(const_cast<void *>(A), const_cast<char *>(B));
}


void PUBLIC_AHA_EXCEPTION_BEGIN_THROW(int A)
{
	OFLUX_AHA_EXCEPTION_BEGIN_THROW(A);
}

} // namespace oflux

#endif // HAS_DTRACE
