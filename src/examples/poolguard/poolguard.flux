pool IntPool () => int *;

node detached S (guard IntPool() as I) => (int b);
node detached NS (int b, guard IntPool() as I) => ();

source S -> NS;
