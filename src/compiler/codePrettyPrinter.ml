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
(* pretty printing C++ish code *)

exception CodeOutput of string

type code = string list * int (* list of lines, indent level *)

let empty_code = ([],0)

let add_code (cdl,ind) str =
	let rec indent n = 
		if n > 0 then "    "^(indent (n-1)) else "" in
	let should_find ch str =
		try let rind = String.rindex str ch
		    in  rind + 2 >= (String.length str)
		with Not_found -> false in
	let should_indent str = should_find '{' str in
	let should_undent str = should_find '}' str in
	let max a b = if a > b then a else b in
	let ind = if should_undent str then max 0 (ind-1) else ind
	in  (((indent ind)^str)::cdl,if should_indent str then ind+1 else ind)

let add_cr (cdl,ind) = (("")::cdl,ind)

let to_string (cdl,_) =
	let t_s r s = (s^"\n"^r)
	in  List.fold_left t_s "" cdl

let output filename (cdl,_) =
	let chan = open_out filename in
	let _ =
		begin
		List.iter (fun x -> output_string chan (x^"\n")) (List.rev cdl);
		close_out chan;
		end 
	in ()
	(*let st = Unix.stat filename
	in  if st.Unix.st_size = 0 
	    then raise (CodeOutput 
		("output file: "^filename^" wrote 0 bytes\n"))
	    else ()*)


