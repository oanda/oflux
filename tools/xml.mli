(** simple xml code for generation *)

type xml = Element of (string * (string * string) list * xml list)
        (* PCData section is not needed *)

val to_string_fmt : xml -> string

val get_tag : xml -> string

val get_attributes : xml -> (string * string) list

val get_contents : xml -> xml list

val diff : string list -> (** list of tags whose contents is ordered *)
        (xml -> xml -> xml) -> (** different handler *)
        (xml -> xml) -> (** same handler *)
        xml -> (** XML left input *)
        xml -> (** XML right input *)
        xml  
  (** find the earliest differences in two xml structures and handle them *)

val filter : (xml -> bool) -> xml -> xml option

val filter_list : (xml -> bool) -> xml list -> xml list
