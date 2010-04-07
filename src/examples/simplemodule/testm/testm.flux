module testm
begin

   node In
        (std::string txt)
        =>
        ...;

   node Out
        (std::string txt)
        =>
        ...;


   node Process
        (std::string txt)
        =>
        (std::string txt);

   In = Process -> Out;

end

