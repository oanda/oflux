
#include "OFluxGenerate_single_source.h"
#include "OFluxRunTime.h"
#include "OFluxProfiling.h"

extern boost::shared_ptr<oflux::RunTimeAbstract> theRT;

int Start(const Start_in *, Start_out *, Start_atoms *)
{
	sleep(1); 
	theRT->log_snapshot();
	return 0;
}
