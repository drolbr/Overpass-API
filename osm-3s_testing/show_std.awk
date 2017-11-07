{
  buf = $0;
  match(buf, "std::[a-z_]*");
  while (RSTART > 0)
  {
    symbol = substr(buf, RSTART + 5, RLENGTH - 5);
    if (symbol == "map" || symbol == "string" || symbol == "vector")
      symbol = "#include <" symbol ">";
    else if (symbol == "binary_search" || symbol == "lower_bound" || symbol == "min" || symbol == "max" || symbol == "sort" || symbol == "unique")
      symbol = "#include <algorithm>";
    else if (symbol == "make_pair" || symbol == "pair")
      symbol = "#include <utility>";
    print symbol;
    buf = substr(buf, RSTART + RLENGTH);
    match(buf, "std::[a-z_]*");
  }
}
