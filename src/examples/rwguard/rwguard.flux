readwrite G () => int *;

node detached S (guard G/write()) => (int b);
node detached NS (int b, guard G/read()) => ();

source S -> NS;
