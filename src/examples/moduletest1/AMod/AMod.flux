
module AMod
begin
  condition even (int a) => bool;
  exclusive aguard () => void *;
  node src (guard aguard() as ag) => (int a);
  node aChoice (int a) => ...;
  node aEvenConsumer (int a) => ...;
  node aOddConsumer (int a) => ...;
  source src -> aChoice;
  aChoice: [even] = aEvenConsumer;
  aChoice: [*] = aOddConsumer;
end
