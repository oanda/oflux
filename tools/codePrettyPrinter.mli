
(** Code pretty printing in a C++ style (auto-indents based on incoming strings)
*)

type code (** opaque type *)

val empty_code : code (** you begin with an empty code value *)

val add_code : code -> string -> code 
(** you add a string (line) to an existing code value to append it to the end
*)
 
val add_cr : code -> code
(** add a new line to the end of the code *)

val to_string : code -> string
(** convert the code value to a big string *)


