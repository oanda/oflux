(** simple xml code for generation *)

type xml = Element of (string * (string * string) list * xml list)
        (* PCData section is not needed *)

val to_string_fmt : xml -> string
