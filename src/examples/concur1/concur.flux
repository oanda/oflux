/*
 * This example demonstrates oflux compile time concurrency capabilities
 */

node Src () => (int a);
node abstract Splitter1 (int a) => ();
node abstract Decider1 (int a) => ();
node abstract Decider2 (int a) => ();
node abstract Splitter2 (int a) => ();
node abstract Splitter3 (int a) => ();
node detached Exec1 (int a) => ();
node detached Exec2 (int a) => ();
node detached Exec3 (int a) => ();
node detached Exec4 (int a) => ();

source Src -> Splitter1;

Splitter1 = Splitter2 & Decider2;

Decider1 : [isEven] = Splitter2;
Decider1 : [*] = Splitter3; 

Decider2 : [isEven] = Exec1;
Decider2 : [*] = Decider1;

Splitter2 = Exec1 & Exec3;

Splitter3 = Exec2 & Exec4;

