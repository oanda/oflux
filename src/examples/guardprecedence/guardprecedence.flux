exclusive Ga () => int *;
exclusive Gb (int v) => int *;

precedence Gb < Ga;

node S (guard Ga()) => (int b);
node NS (int b, guard Gb(b), guard Ga()) => ();

source S -> NS;
