
include plugin1.flux
include mod1.flux
include mod2.flux

plugin plugin3
begin
  external node abstract plugin1.Execute1 ( int id ) => ...;
  external node abstract plugin1.next ( int id ) => ...;
  external node Sink (int id) => ();
  node n3 (int id) => (int id);
  condition icond2 ( int id ) => bool;
  condition icond3 ( int id ) => bool;
  plugin1.next : [icond2] = n3 -> Sink;

  instance mod1 a1;
  instance mod2 a2;

  plugin1.next : [icond3] = a1.HandleRequest;
  a1.HandleResult = a2.HandleRequest;
  a2.HandleResult = Sink;

  plugin1.Execute1 &= a1.HandleRequest;

end
