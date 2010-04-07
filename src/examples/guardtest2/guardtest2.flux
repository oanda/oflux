readwrite Ga () => int *;
readwrite Gb (int v) => int *;

node S (guard Ga/write()) => (int b);
node NS (int b, guard Gb/read(b)) => ();

node R (guard Ga/read()) => ();

source S -> NS;
source R;
