readwrite Ga (int a) => Aob *;

readwrite Gb (int b) => Bob *;

readwrite Gc (int c) => Cob *;

condition isNotNull(void *) => bool;

node S () => (int a);

node detached N
	( int a
	, guard Ga/write(a) as ga
	, guard Gb/write(ga->get_a()) as gb if isNotNull(ga)
	, guard Gc/write(gb->get_b()) as gc_ if isNotNull(gb)
	)
	=>
	();

source S -> N;
