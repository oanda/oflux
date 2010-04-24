#ifndef OFLUX_MACHINE_SPECIFIC_H
#define OFLUX_MACHINE_SPECIFIC_H

namespace oflux {
namespace lockfree {

class MachineSpecific {
	enum    //                          presumed log-size of a cache line
		{ Cache_Line_Scale          = 6
		//                          presumed size of a cache line
		, Cache_Line_Size           = (1 << Cache_Line_Scale) 
		//                          number of entries in each bucket
		};
};

} // namespace lockfree
} // namespace oflux


#endif // OFLUX_MACHINE_SPECIFIC_H
