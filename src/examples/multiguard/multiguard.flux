exclusive Gb () => int *;
exclusive Ga () => int *;

node S (guard Ga() as g) => (int b);
node R (guard Gb() as g) => (int b);
node NS (int b) => ();

source S -> NS;
source R -> NS;
