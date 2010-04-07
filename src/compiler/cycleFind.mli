(** detect if there is a cycle in the digraph described by the 
    pair list *)

val detect : ('a * 'a) list -> bool (** true if a cycle is found *)

