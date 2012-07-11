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
(** all digraphs are 1. forests or 2. have a strong cycle *) 

type 'a tree = T of 'a * 'a tree list ref

let new_tree i = T (i,ref [])

let add_succ (T (_,tl)) trsucc = tl := trsucc::(!tl)

let add_or_create tmap i =
	try tmap, List.assoc i tmap
	with Not_found ->
		let nt = new_tree i
		in  (i,nt)::tmap, nt

let rec has x (T (y,tl)) = (x=y) || (List.exists (has x) (!tl))

exception Cycle

let treeify edgelist =
	let tfy tmap (x,y) =
		let tmap, xt = add_or_create tmap x in
		let tmap, yt = add_or_create tmap y in
		let _ = if has x yt then raise Cycle else () in
		let _ = add_succ xt yt
		in tmap
	in  List.fold_left tfy [] edgelist

let detect edgelist =
        try  let _ = treeify edgelist
             in  false
        with Cycle -> true




