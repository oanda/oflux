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
(** simple xml code for generation *)

type xml = Element of (string * (string * string) list * xml list)
        (* PCData section is not needed *)

let to_string_fmt xmlinp = (* returns a string *)
        let buff = Buffer.create 1024 in
        let add_to_buffer b s = Buffer.add_string b s; b in
        let add_strlist b sl = 
                let _ = List.fold_left add_to_buffer b sl
                in  () in
        let write_attrib (n,v) =
                let _ = add_strlist buff
                        [ " "
                        ; n
                        ; "=\""
                        ; v
                        ; "\"" ]
                in  () in
        let rec write tab xmlinp =
                match xmlinp with
                        (Element (tag,alist,ll)) ->
                                begin
                                add_strlist buff
                                        [ tab
                                        ; "<"
                                        ; tag ];
                                List.iter write_attrib alist;
                                (match ll with 
                                        [] -> Buffer.add_string buff "/>\n"
                                        | _ ->
                                        begin
                                        Buffer.add_string buff ">\n";
                                        List.iter (write (tab^" ")) ll;
                                        add_strlist buff
                                                [ tab
                                                ; "</"
                                                ; tag
                                                ; ">\n" ];
                                        ()
                                        end
                                )
                                end
        in  begin
            write "" xmlinp; 
            Buffer.contents buff
            end

let diff ordered_tags diff_handler same_handler xml1 xml2 =
        let sort_attrib = false in (** could expose as an argument *)
        let attrib_compare (x,_) (y,_) = compare x y in
        let node_compare (Element(n1,a1,_)) (Element (n2,a2,_)) =
                let opt_sort_attrib a = if sort_attrib then List.sort attrib_compare a else a in
                let as1 = opt_sort_attrib a1 in
                let as2 = opt_sort_attrib a2 
                in  compare (n1,as1) (n2,as2) in
        let rec diff' (((Element (n1,a1,xl1)) as xml1),((Element (n2,a2,xl2)) as xml2)) =
                let is_ordered = List.mem n1 ordered_tags in
                let somesubdiff,res = 
                        if (node_compare xml1 xml2) = 0 && (List.length xl1)=(List.length xl2) then
                                let sort_opt xl = if is_ordered then xl else List.sort node_compare xl in
                                let xls1,xls2 = (sort_opt xl1),(sort_opt xl2) in
                                let comb = List.combine xls1 xls2 in
                                let resl = List.map diff' comb in
                                let on_each (tf,xmlsame) = if tf then xmlsame else same_handler xmlsame in
                                let somesubdiff = List.exists (fun (tf,_) -> tf) resl
                                in  if somesubdiff then
                                        true, Element (n1,a1, List.map on_each resl)
                                    else false, xml1 (* same, so return first one *)
                        else true, diff_handler xml1 xml2
                in  somesubdiff,res in
        let notsame,res = diff' (xml1,xml2)
        in  if notsame then res else same_handler res
                                
                    
let get_tag (Element (n,_,_)) = n

let get_attributes (Element (_,a,_)) = a

let get_contents (Element (_,_,c)) = c       

let rec opt_map ff ll =
        match ll with
                (h::t) -> 
                        (match ff h with
                                None -> opt_map ff t
                                | (Some r) -> r::(opt_map ff t))
                | _ -> []

let rec filter filtfun xml =
        if filtfun xml then
                Some (Element (get_tag xml, get_attributes xml, opt_map (filter filtfun) (get_contents xml)))
        else None

let filter_list filtfun xmll = opt_map (filter filtfun) xmll
