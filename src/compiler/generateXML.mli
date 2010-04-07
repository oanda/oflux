(** a generator for the OFlux xml flow descriptions *)
exception XMLConversion of string * ParserTypes.position
  (** when things go wrong *)

val emit_program_xml : string -> (** filename *)
		Flow.built_flow -> 
			Xml.xml

val emit_program_xml' : string -> (** filename *)
		Flow.built_flow -> 
		(string * string) list -> (** full uses model *)
			Xml.xml

val emit_plugin_xml : string -> (** filename *)
                string list -> (** list of dependencies (plugins *)
		Flow.built_flow -> (** before *)
		Flow.built_flow -> (** after *)
		(string * string) list -> (** full used model *)
			Xml.xml

val write_xml : Xml.xml -> string -> unit

