exclusive Ga () => int *;
exclusive Gb (int v) => int *;

node S (guard Ga()) => (int b);
node NS (int b, guard Gb(b+1) if b <= 8 ) => ();

source S -> NS;
