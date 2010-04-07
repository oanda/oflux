(** for holding a debug print function -- which can be turned off *)


let debug = ref false

let timing_on = ref false

let dprint_string s = if !debug then print_string s else ()

let timer_indent = ref 0

let timer thing_timed_str =
        let rec findent n =
                if n <= 0 then ""
                else " "^(findent (n-1))
        in
        if !timing_on then
                let starttime = ref (Sys.time ()) in
                let indent = findent (!timer_indent) in
                let _ = timer_indent := (!timer_indent) + 1
                in  fun () ->
                        let endtime = Sys.time () in
                        let _ = timer_indent := (!timer_indent) - 1
                        in  print_string (" "^indent
                                ^(string_of_float (endtime))
                                ^" TIMER : "^thing_timed_str^" "
                                ^(string_of_float (endtime -. (!starttime)))
                                ^" sec\n")
        else fun () -> ()

