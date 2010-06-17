readwrite Ga () => int *;

/* try S guard acquisition as write mode or upgradeable mode 
   write: R and S alternate every sec
   upgradeable: R and S steady state in going each second
*/
node S (guard Ga/upgradeable()) => (); 
node R (guard Ga/read()) => ();

source S;
source R;
