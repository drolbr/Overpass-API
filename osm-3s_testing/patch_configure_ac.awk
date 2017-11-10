{
  if (version == "")
    version = $0;
  else if (substr($0,1,7) == "AC_INIT")
  {
    match($0,"\\[[0-9.]*\\]");
    print substr($0,1,RSTART) version substr($0,RSTART+RLENGTH-1);
  }
  else
    print;
}
