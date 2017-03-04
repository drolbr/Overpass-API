{
  if ($1 == "/*" && ($2 == "==" || $2 == "==="))
  {
    print "\n"substr($0,4);
    in_comment = 1;
  }
  else if ($1 == "*/" && in_comment == 1)
  {
    print "";
    in_comment = 0;
  }
  else if (in_comment)
    print;
}
