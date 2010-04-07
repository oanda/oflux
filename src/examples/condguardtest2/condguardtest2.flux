exclusive Ga () => int *;
exclusive Gb (int v) => int *;

node S (guard Ga()) => (int b);
node abstract GO (int b) => ...;
node NS (int b, guard Gb(b+1) if b <= 8 ) => (int b);
node NSS (int b, guard Gb(b)) => ();

source S -> GO;
GO = NS -> NSS;

