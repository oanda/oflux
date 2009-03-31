(* documentation tags for OFlux *)

type 'a doc = 
	{ name : string
	; mutable content : 'a
	}

type node_content =
        { mutable description : string
        ; mutable inputs : string doc list
        ; mutable outputs : string doc list
        ; mutable guards : string doc list
        }

type node_doc = node_content doc

type node_doc_list = node_doc list

val dump : node_doc_list -> unit

val empty_node_content : unit-> node_content

val empty : node_doc_list

val add_input : node_doc -> string doc -> unit
val add_output : node_doc -> string doc -> unit
val add_guard : node_doc -> string doc -> unit

val add_content_string : string doc -> string -> unit

val add_description : node_doc -> string -> unit

val add_node : node_doc_list -> node_doc -> node_doc_list

val find : 'a doc list -> string -> 'a doc

val new_node : string -> node_doc

val new_item : string -> string doc


