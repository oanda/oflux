node Select () => ();
node Read () => ();
node Route () => ();
node Send () => ();
node HandleMessage () => ();

node detached One () => ();
node detached Two () => ();
node detached Three () => ();
node detached Four () => ();
node detached Five () => ();
node detached HandleOne () => ();

source Select -> HandleMessage;
source One -> HandleOne;

HandleMessage = Read -> Route -> Send;
HandleOne = Two -> Three -> Four -> Five;
