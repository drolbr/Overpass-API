{
  if (show > 1)
    print;
  else if (show == 1 && NF > 0 && $1 != "#")
  {
    print;
    show = 2;
  }
  if (!show)
  {
    if ($1 == "#!/usr/bin/env")
      show = 1;
    else if (NF > 0)
    {
      show = 3;
      print;
    }
  }
}
