
module A
begin
  node f () => (int i, long l);
  node g (int i, long l) => ...;
  source f -> g;
end

