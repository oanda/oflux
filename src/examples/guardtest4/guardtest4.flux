exclusive Ga () => int *;
readwrite Gb () => int *;

node S (guard Ga()) => ();
node detached NS (guard Ga(), guard Gb/write()) => ();

node Log () => ();

node R () => ();
node detached NR (guard Gb/read()) => ();

source S -> NS;
source R -> NR;
source Log;

precedence Gb < Ga;
