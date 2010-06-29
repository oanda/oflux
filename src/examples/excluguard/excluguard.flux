exclusive G () => int *;

node detached S (guard G()) => (int b);
node detached NS (int b, guard G()) => ();

source S -> NS;
