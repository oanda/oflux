
static
module CMod
begin
  exclusive anotherguard () => void *;
  node anode (guard anotherguard() as ang) => (int a);
  node abstract aerrorhandler () => ...;
  node anodeNext (int a) => ...;
  node aflow () => ...;
  aflow = anode -> anodeNext;
  handle error anode -> aerrorhandler;
end
