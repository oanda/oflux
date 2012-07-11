(*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *)
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

