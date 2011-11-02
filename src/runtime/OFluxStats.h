#ifndef _OFLUX_STATS_H_
#define _OFLUX_STATS_H_

#include <stdlib.h>

namespace oflux {

  class OFluxStats
  {
  public:
    static size_t UnusedThreadCount;
    static size_t IdleThreadCount;
    static size_t DetachedThreadCount;
  };
}

#endif
