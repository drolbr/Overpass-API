{
  match($0,"id=\"[^ ]*\"");
  anchor = substr($0,RSTART+4,RLENGTH-5);
  print "<a href=\"#"anchor"\">"substr($0,index($0,">")+1,length($0)-index($0,">")-5)"</a><br/>";
}