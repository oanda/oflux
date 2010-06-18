readwrite Ga (int a) => int *;

readwrite Gb () => int *;

condition isNotNull(void *) => bool;
condition isNull(void *) => bool;

node S () => (int a);

node detached N
	( int a
	, guard Ga/write(a) as ga
	, guard Gb/write/nullok() as gb if isNull(ga)
	)
	=>
	();

source S -> N;
