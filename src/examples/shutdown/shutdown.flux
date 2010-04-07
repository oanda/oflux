/* test start up and shut down */

exclusive G (int x) => int *;

node S () => (int n);
node N (int n, guard G(n) as g) => ();

source S -> N;
