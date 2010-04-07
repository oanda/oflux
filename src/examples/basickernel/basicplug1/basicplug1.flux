include basickernel.flux

plugin basicplug1
 begin
 external node abstract FooSucc (int a) => ...;
 node abstract BarEtc (int a) => ...;
 node Bar (int a) => (B1 op1, int * op2, bool dc);
 node Consume (B1 op1, int * op2) => ();
 FooSucc &= BarEtc;
 BarEtc = Bar -> Consume;
 end
