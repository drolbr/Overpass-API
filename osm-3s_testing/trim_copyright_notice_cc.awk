{
  if (show > 2)
    print;
  else if (show == 2 && NF > 0)
  {
    print;
    show = 3;
  }
  if (show == 1 && $1 == "*/")
    show = 2;
  if (!show)
  {
    if ($1 == "/**")
      show = 1;
    else if (NF > 0)
    {
      show = 3;
      print;
    }
  }
}
