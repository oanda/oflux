include basickernel.flux

plugin basicplug2
 begin
 /*depends basicplug1;*/ /* uncomment to make load-time dependency*/
 external node abstract FooSucc (int a) => ...;
 node abstract BarEtc (int a) => ...;
 node Bar (int a) => (char * c1, B2 c2, bool dc);
 node Consume (char * c1, B2 c2) => ();
 node abstract ConsumeSucc () => ();
 FooSucc &= BarEtc;
 BarEtc = Bar -> Consume -> ConsumeSucc;
 terminate ConsumeSucc;
 end
