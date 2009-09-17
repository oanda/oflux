#!/bin/bash

name=$1
mainfile=$2
directory=$3

if [ -e $directory/README ];
then

echo "<tr>"
echo " <td>"
echo "  <b>"$name"</b><br/><a href=\""$mainfile".svg\">flow </a><br/>"
echo "  <a href=\""$mainfile".html\">flux </a><br/>"
echo "  <br/><a href=\""$mainfile"-flat.svg\">"flattened flow"</a>"
echo " </td>"
echo " <td>"
sed \
  -e 's/< /\&lt; /g' \
  -e 's/> /\&gt; /g' \
  -e 's/^$/<br\/>/g' \
   $directory/README
echo " </td>"
echo "</tr>"

fi
