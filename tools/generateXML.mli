(** a generator for the OFlux xml flow descriptions *)
exception XMLConversion of string * ParserTypes.position
  (** when things go wrong *)

val emit_program_xml : string -> (** filename *)
		Flow.built_flow -> 
			Xml.xml

val emit_plugin_xml : string -> (** filename *)
		Flow.built_flow -> (** before *)
		Flow.built_flow -> (** after *)
			Xml.xml

val write_xml : Xml.xml -> string -> unit

