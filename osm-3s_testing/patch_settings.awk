{
  if (commit == "")
    commit = $0;
  else if (substr($1,1,8) == "version(")
  {
    version = substr($1,10,length($1)-12);
    split(version,a,"\.");
    if (a[4] != "")
      print "  version(\""a[1]"."a[2]"."a[3]"."++a[4]"\"),";
    else
      print "  version(\""a[1]"."a[2]"."a[3]"\"),";
  }
  else if (substr($1,1,12) == "source_hash(")
    print "  source_hash(\""commit"\"),";
  else
    print $0;
}

