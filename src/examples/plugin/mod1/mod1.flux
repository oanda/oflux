
static
module mod1
begin

  /* nodes */
  node abstract HandleRequest ( int id ) => ...;
  node abstract HandleResult ( int id ) => ...;
  node pass ( int id ) => ( int id );
  
  /* flow */
  HandleRequest = pass -> HandleResult;

end

