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
(** not exactly union find's best implementation, but it is correct :-) *)

(** given a list of associated pairs, produce a list list of items so that 
they are in their equivalence classes *)


let union_find_n item_list =
        let basis = 
                let al,bl = List.split item_list
                in   Uniquify.uniq_discard compare (al @ bl)
                in
        let n = List.length basis in
        let htable = Hashtbl.create n in
        let revmap = Array.make n None in
        let ht_ffun i b = 
                Hashtbl.add htable b i ; 
                Array.set revmap i (Some b) ;
                i+1 in
        let _ = List.fold_left ht_ffun 0 basis in
        let lookup2 (a,b) = 
                let ar = Hashtbl.find htable a in
                let br = Hashtbl.find htable b 
                in  if ar < br then (ar,br) else (br,ar)
                in
        let ilist_numbers = List.sort compare
                (List.map lookup2 item_list) in
        let iarr = Array.make n (n+2) in
        let rec get_min_sim (a,minsofar) ll =
                match ll with
                        ((aa,b)::tl) ->
                                if aa = a then 
                                        let minsofar = min (Array.get iarr b) minsofar
                                        in  get_min_sim (a,minsofar) tl
                                else minsofar
                        | _ -> minsofar
                in
        let rec assign_descend (atvalue,rvalue) ll =
                match ll with
                        ((a,b)::tl) ->
                                if atvalue != a then 
                                        let msf = min a (Array.get iarr b) in
                                        let msf = get_min_sim (a,msf) tl
                                        in  Array.set iarr a msf; 
                                            Array.set iarr b msf; 
                                            assign_descend (a,msf) tl
                                else Array.set iarr a rvalue; 
                                     assign_descend (atvalue,rvalue) tl
                        | _ -> ()
                in
        let _ = assign_descend (-1,-1) ilist_numbers in
        let resmap = Array.make n [] in
        let iter_fun i ni =
                let l = Array.get resmap ni in
                let it = match Array.get revmap i with
                                (Some x) -> x
                                | _ -> raise Not_found
                in  Array.set resmap ni (it::l)
                in
        let _ = Array.iteri iter_fun iarr in
        let final_ffun lll ll =
                if List.length ll > 0 then ll::lll else lll
        in  Array.fold_left final_ffun [] resmap
        
                                        


let union_find item_list =
	let has_either (a,b) ll =
		(List.mem a ll) or (List.mem b ll) in
	let rec add ll a = if List.mem a ll then ll else a::ll in
	let rec uf res (a,b) =
		let res1, res2 = List.partition (has_either (a,b)) res
		in  match res1 with
			[] -> [a;b]::res2
			| [ll] ->  (List.fold_left add ll [a;b])::res2
			| [ll1;ll2] -> (ll1@ll2)::res2
			| _ -> raise Not_found (*serious bug*)
	in  List.fold_left uf [] item_list

(*
let input1 = [ 1,2;3,4;5,6;7,8;8,3;6,1 ]

let _ = union_find input1	
*)
			
