(** a generator for the OFlux xml flow descriptions *)
exception XMLConversion of string * ParserTypes.position
  (** when things go wrong *)

val emit_xml : string ->
		Flow.built_flow ->
		TypeCheck.consequence_result ->
			Xml.xml

val write_xml : Xml.xml -> string -> unit

