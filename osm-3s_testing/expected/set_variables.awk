{
  if (testdir == "")
    testdir = $0;
  else if (last_cr == "")
    last_cr = $0;
  else
  {
    if (inner_line)
      printf "\n";
    else
      inner_line = 1;

    gsub("\\${testdir}",testdir);
    printf $0;
  }
}
END {
  if (last_cr != "no")
    printf "\n";
}
