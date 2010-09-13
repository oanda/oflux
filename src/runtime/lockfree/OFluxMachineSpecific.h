#ifndef OFLUX_MACHINE_SPECIFIC_H
#define OFLUX_MACHINE_SPECIFIC_H

/**
 * @file OFluxMachineSpecific.h
 * @author Mark Pichora
 *  Specific values and macros for getting things done on different
 * machine architectures.
 */


namespace oflux {
namespace lockfree {

class MachineSpecific {
	enum    //                          presumed log-size of a cache line
		{ Cache_Line_Scale          = 
#if sparc_HOST_ARCH
						7 // most sparc
#else
						6 // most intel
#endif
		//                          presumed size of a cache line
		, Cache_Line_Size           = (1 << Cache_Line_Scale) 
		//                          number of entries in each bucket
		};
};

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


} // namespace lockfree
} // namespace oflux


#endif // OFLUX_MACHINE_SPECIFIC_H
