
include kernalex.flux

plugin plugin2
begin
  external node abstract zHandleRequest ( int id ) => ...;
  condition IsPlugin2 ( int id ) => bool;
  node ExecutePlugin2 ( int id ) => ( int id );
  node abstract LocalSink ( int id ) => ();
  zHandleRequest : [ IsPlugin2 ] = ExecutePlugin2 -> LocalSink ;
  terminate LocalSink;
end
