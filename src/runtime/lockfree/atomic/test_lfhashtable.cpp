#include "OFluxLFHashTable.h"
#include "DistributedCounter.h"
#include <pthread.h>
#include <cstdlib>
#include <cstdio>

using namespace oflux::lockfree;
using namespace oflux;

struct Key {
	int a;
	int b;

	bool operator==(const Key & k) const {
		return (a==k.a) && (b == k.b);
	}
	size_t hash() const { return (size_t)(a^b); }
};

struct Value {
	double f;
};

enum { num_threads = 4 };

typedef HashTable<Key,Value,num_threads> HT;

Counter<size_t> get_hit_count;
Counter<size_t> get_miss_count;
Counter<size_t> insert_hit_count;
Counter<size_t> insert_miss_count;
Counter<size_t> remove_hit_count;
Counter<size_t> remove_miss_count;

const Value * Does_Not_Exist =
	HT::HTC::Does_Not_Exist;

void * run_thread(void *htvoid)
{
	HT * ht = reinterpret_cast<HT *>(htvoid);
	init_ThreadNumber(_tn);
	for(size_t i = 0; i < 50000; ++i) {
		Key k = { _tn.index%2, i};
		if((i+_tn.index)%2) {
			Value * v = new Value();
			v->f =  i / ((double) _tn.index);
			const Value * ov = ht->compareAndSwap(
					  k
					, Does_Not_Exist
					, v);
			if(ov != Does_Not_Exist) {
				++insert_hit_count;
			} else {
				++insert_miss_count;
			}
		} else {
			const Value * v = ht->get(k);
			if(v != Does_Not_Exist) {
				++get_hit_count;
			} else {
				++get_miss_count;
			}
		}
	}
	for(size_t i = 0; i < 50000; ++i) {
		Key k = { _tn.index%2, i};
		if((i+_tn.index)%2) {
			const Value * v = ht->remove(k);
			if(v != Does_Not_Exist) {
				delete v;
				++remove_hit_count;
			} else {
				++remove_miss_count;
			}
		} else {
			const Value * v = ht->get(k);
			if(v != Does_Not_Exist) {
				++get_hit_count;
			} else {
				++get_miss_count;
			}
		}
	}
	return NULL;
}

int main()
{
	HT htable;
	pthread_t tids[num_threads];
	for(size_t i = 0; i < num_threads; ++i) {
		int err = pthread_create(&tids[i],NULL,run_thread,&htable);
		if(err != 0) {
			exit(11);
		}
	}
	time_t startt = time(NULL);
	void * tret = NULL;
	printf("statistics (hit/miss counts):\n");
	for(size_t i=0; i < num_threads; ++i) {
		int err = pthread_join(tids[i],&tret);
		if(err != 0) {
			exit(12);
		}
		/*printf("thread %u malloced %ld freed %ld collected %ld\n"
			, i
			, malloc_counter[i]
			, free_counter[i]
			, collect_counter[i]);*/
		printf("thread %u get %u/%u insert %u/%u remove %u/%u\n"
			, i
			, get_hit_count[i] , get_miss_count[i]
			, insert_hit_count[i], insert_miss_count[i]
			, remove_hit_count[i], remove_miss_count[i] );
	}
	time_t endt = time(NULL);
	printf("total time the test ran %ld sec\n", endt - startt);
	return 0;
}
