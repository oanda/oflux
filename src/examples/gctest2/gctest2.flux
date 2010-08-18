
readwrite/gc Ga () => void *;
exclusive/gc Gb (int v) => int *; 

precedence Ga < Gb;

node S_splay () => (int a);

node NS (int a, guard Ga/read(), guard Gb(a) as ab) => (int a); 
		/* populates the rval guard */

node NSS (int a, guard Gb(a) as ab) => (); /* depopulates the rval guard */

node abstract N(int a) => ...;

source S_splay -> N;
N = NS -> NSS;
