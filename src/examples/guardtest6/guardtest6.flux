readwrite Ga () => void*;

node detached S1 () => ();
node detached NS1 (guard Ga/write()) => ();

node detached S2 () => (int k);
node detached NS2 (int k, guard Ga/read()) => ();

source S1 -> NS1;
source S2 -> NS2;

