module testm_self
begin

   exclusive someGuard() => void*; 

   node In
        (std::string txt)
        =>
        ...;

   node Out
        (std::string txt)
        =>
        ...;


   node Process
        (std::string txt, guard someGuard() if self->useSomeGuard)
        =>
        (std::string txt);

   In = Process -> Out;

end

