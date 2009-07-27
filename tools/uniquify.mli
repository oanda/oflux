(* functions for common uniquification functions (discard/merge duplicates) in lists*)

val uniq : ('a -> 'a -> int) -> (** comparator function *)
	('a -> 'a -> 'a) -> (** resolution function *)
	'a list -> (** list to operate on *)
	'a list

val uniq_discard : ('a -> 'a -> int) -> (** comparator function *)
	'a list -> (** list to operate on *)
	'a list

val uniq_append : ('a list -> 'a list -> int) -> (** comparator function *)
	'a list list -> (** list to operate on *)
	'a list list
