#ifndef OFLUX_MACHINE_SPECIFIC_H
#define OFLUX_MACHINE_SPECIFIC_H
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
 * @file OFluxMachineSpecific.h
 * @author Mark Pichora
 *  Specific values and macros for getting things done on different
 * machine architectures.
 */


namespace oflux {
namespace lockfree {

class MachineSpecific {
public:
	enum    //                          presumed log-size of a cache line
		{ Cache_Line_Scale          = 
#if sparc_HOST_ARCH
						7 // most sparc
#else
						6 // most intel
#endif
		//                          presumed size of a cache line
		, Cache_Line_Size           = (1 << Cache_Line_Scale) 
		
		, Max_Threads_Conserve      = 32
		, Max_Threads_Liberal       = 1024
		};
};

#define DEFAULT_MEMPOOL_MAX_THREADS  (oflux::lockfree::MachineSpecific::Max_Threads_Conserve)

# define INLINE_HEADER static inline
# define STATIC_INLINE INLINE_HEADER

#if defined(__GNUC_GNU_INLINE__) && !defined(__APPLE__)
#  if defined(KEEP_INLINES)
#    define EXTERN_INLINE inline
#  else
#    define EXTERN_INLINE extern inline __attribute__((gnu_inline))
#  endif
#else
#  if defined(KEEP_INLINES)
#    define EXTERN_INLINE
#  else
#    define EXTERN_INLINE INLINE_HEADER
#  endif
#endif


EXTERN_INLINE void
store_load_barrier(void) {
#if i386_HOST_ARCH
    __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
#elif x86_64_HOST_ARCH
    __asm__ __volatile__ ("lock; addq $0,0(%%rsp)" : : : "memory");
#elif sparc_HOST_ARCH
    __asm__ __volatile__ ("membar #StoreLoad" : : : "memory");
#else
#error memory barriers unimplemented on this architecture
#endif
}

EXTERN_INLINE void
mfence_or_equiv_barrier(void) {
#if i386_HOST_ARCH
    __asm__ __volatile__("mfence");
#elif x86_64_HOST_ARCH
    __asm__ __volatile__("mfence");
#elif sparc_HOST_ARCH
    __sync_synchronize();
#else
#error memory barriers unimplemented on this architecture
#endif
}

EXTERN_INLINE void
load_load_barrier(void) {
#if i386_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif x86_64_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif sparc_HOST_ARCH
    /* Sparc in TSO mode does not require load/load barriers. */
    __asm__ __volatile__ ("" : : : "memory");
#else
#error memory barriers unimplemented on this architecture
#endif
}

EXTERN_INLINE void
write_barrier(void) {
#if i386_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif x86_64_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#elif sparc_HOST_ARCH
    __asm__ __volatile__ ("" : : : "memory");
#else
#error memory barriers unimplemented on this architecture
#endif
}


} // namespace lockfree
} // namespace oflux


#endif // OFLUX_MACHINE_SPECIFIC_H
