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
(* used to make a list of things unique *)

let uniq comp resolv ll =
	let rec uniq' ll =
		match ll with
			(h1::h2::t) ->
				if (comp h1 h2) = 0 then
					uniq' ((resolv h1 h2)::t)
				else h1::(uniq' (h2::t))
			| _ -> ll in
	let lls = List.sort comp ll 
	in  uniq' lls

let uniq_keep_order comp resolv ll =
	let rec within wl x =
		match wl with 
			(h::t) -> 
				if (compare h x) = 0 then (resolv h x)::t
				else h::(within t x)
			| _ -> [x] in
	let rec uniq' sofar ll =
		match ll with
			(h::t) -> uniq' (within sofar h) t
			| _ -> ll
	in  List.rev (uniq' [] ll)

let uniq_discard comp ll = 
	let discard a _ = a
	in  uniq comp discard ll

let uniq_append comp ll =
	let append a b = a @ b
	in  uniq comp append ll
	
