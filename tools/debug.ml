(** for holding a debug print function -- which can be turned off *)


let debug = ref false

let dprint_string s = if !debug then print_string s else ()

