#include <servant/servant_engine.h>

#define KEY 365
#define VAL 7460

struct ubpf_map_def test_map = {
	.type = UBPF_MAP_TYPE_HASHMAP,
	.key_size = sizeof(int),
	.value_size = sizeof(long long int),
	.max_entries = 100,
	.nb_hash_functions = 0,
};

/* static inline __attribute__((always_inline)) void __initialize(void); */

int bpf_prog(void *pkt) {
	// TODO: enable runtime to have a way of initializing the maps
	// probably using a RPC framework like Katran.
	/* static int __flag = 0; */
	/* if (!__flag) */
	/* { */
	/* 	/1* __flag = 1; *1/ */
	/* 	__initialize(); */
	/* } */

	DUMP("A packet received\n");
	const int key = KEY;
	int * val = userspace_lookup(&test_map, &key);
	if (val != NULL && *val == VAL) {
		DUMP("Test passed\n");
		return DROP;
	}
	DUMP("Test failed\n");
	return DROP;
}

/* static inline __attribute__((always_inline)) void __initialize(void) */
/* { */
/* 	DUMP("Initializing\n"); */
/* 	const int key = KEY; */
/* 	long long int value = VAL; */
/* 	userspace_update(&test_map, &key, &value); */
/* } */
