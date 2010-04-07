exclusive G (int a, int b, int c, int d, int e) => int *;

node S () => ();
node NS (guard G(1, 2, 3, 4, 5) as ga, guard G(1, 2, 3, 4, 6) as gb) => ();

source S -> NS;
