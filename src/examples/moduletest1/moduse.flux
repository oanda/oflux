
include AMod.flux
include BMod.flux
include CMod.flux


node consumer_even (int k) => (int k, int r);
node consumer_odd (int k) => (int k, int r);

node consumer_ac (int a) => ();

instance BMod aB;
instance CMod aC where anotherguard = aB.aA.aguard;

aB.aA.aEvenConsumer = consumer_even -> aB.finish;
aB.aA.aOddConsumer = consumer_odd -> aB.finish;

/*aC.anodeNext = consumer_ac;*/
terminate aC.anodeNext;
terminate aC.aerrorhandler;

