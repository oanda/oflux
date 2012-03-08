include testm_self.flux

node Start () => (std::string txt);
node End (std::string txt) => ();

instance testm_self ggg;

source Start -> ggg.In;
ggg.Out = End;
