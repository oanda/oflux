
(** generate a DOT file with the flow graph in it *)

type dot

val generate_flow : Flow.flowmap -> dot

val to_string : dot -> string

val generate_program : string -> ParserTypes.program -> dot

