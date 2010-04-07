
#ifndef AMOD_IMPL_H
#define AMOD_IMPL_H

namespace AMod {
	class ModuleConfig {
        public:
		// add fun stuff here
		ModuleConfig(const char * n, int id);

		char name[40];
                int  id;
	};

        ModuleConfig * init(const char *n,int id);
}

#endif // AMOD_IMPL_H
