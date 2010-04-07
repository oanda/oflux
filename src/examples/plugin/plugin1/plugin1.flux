
include kernalex.flux

plugin plugin1
begin
  /* external nodes and guards */
  external node abstract zHandleRequest ( int id ) => ... ;
  external node Sink ( int id ) => ();
  external readwrite og1 ( int id ) => int *;

  /* internal conditions and guards */
  condition pcond1 ( int id ) => bool;
  condition icond1 ( int id ) => bool;
  readwrite ig1 ( int id ) => int *;

  /* nodes */
  node abstract Execute1 ( int id ) => ...;
  node abstract next ( int id ) => ...;
  node exec1 ( int id, guard og1/read(id) ) => ( int aid );
  node n1 (int id, guard ig1/read(id) ) => ( int id );
  node n2 ( int id ) => ( int id );
  node abstract exec1_abstract (int id) => ();
  
  /* flow */
  zHandleRequest : [ pcond1 ] = Execute1;
  Execute1 = exec1_abstract;
  exec1_abstract = exec1 -> next;
  next : [icond1] = n1 -> Sink;
  next : [*] = n2 -> Sink;
end
