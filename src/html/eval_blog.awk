{
  if (filename == "")
    filename = $0;
  
  if (title == "" && substr($1,1,4) == "<h1>")
    title = substr($0,5,length($0)-9);
  
  if (published == "" && $1 == "<p>Published:")
    published = substr($2,1,10);
}
END {
  if (filename != "index.html")
    print "<p>"published": <a href=\""filename"\">"title"</a></p>";
}
