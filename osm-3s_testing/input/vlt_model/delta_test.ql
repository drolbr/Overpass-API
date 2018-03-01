[diff:"2018-02-01T00:01:00Z","2018-02-01T01:01:00Z"];

(
  make point ::geom=pt(48.1,11.004);
  make point ::geom=pt(48.1,11.005);
  make point ::geom=pt(48.1,11.006);
)->.point;

node(2001)->.n;
.n convert equal_geom ::=::;
out geom;
.n convert equal_geom ::=::,::geom=pt(48,11);
out geom;
.n convert equal_geom ::=::,::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001));
out geom;
.n convert equal_geom ::=::,::geom=poly(lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001)));
out geom;
.n convert equal_geom ::=::,::geom=point.gcat(geom());
out geom;
compare()(
  make marker after=n.set(id());out;
);

node(2003)->.n;
.n convert equal_geom ::=::;
out geom;
.n convert equal_geom ::=::,::geom=pt(48,11);
out geom;
.n convert equal_geom ::=::,::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001));
out geom;
.n convert equal_geom ::=::,::geom=poly(lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001)));
out geom;
.n convert equal_geom ::=::,::geom=point.gcat(geom());
out geom;
compare()(
  make marker after=n.set(id());out;
);

node(2004)->.n;
.n convert equal_geom ::=::;
out geom;
.n convert equal_geom ::=::,::geom=pt(48,11);
out geom;
.n convert equal_geom ::=::,::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001));
out geom;
.n convert equal_geom ::=::,::geom=poly(lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001)));
out geom;
.n convert equal_geom ::=::,::geom=point.gcat(geom());
out geom;
compare()(
  make marker after=n.set(id());out;
);

node(2005)->.n;
.n convert equal_geom ::=::;
out geom;
.n convert equal_geom ::=::,::geom=pt(48,11);
out geom;
.n convert equal_geom ::=::,::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001));
out geom;
.n convert equal_geom ::=::,::geom=poly(lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001)));
out geom;
.n convert equal_geom ::=::,::geom=point.gcat(geom());
out geom;
compare()(
  make marker after=n.set(id());out;
);

(
  make point ::geom=pt(48.1,11.004);
  make point ::geom=pt(48.1,11.0055);
  make point ::geom=pt(48.1,11.006);
)->.point_bis;

node(2500)->.n;
if (n.count(nodes) == 1)
(
  make equal_tags tag="no_geom"->.no_geom;
  make equal_tags tag="center",::geom=pt(48,12.004)->.center;
  make equal_tags tag="comp_vs_vertex",::geom=point.gcat(geom())->.comp_vs_vertex;
  make equal_tags tag="comp_equal",::geom=point.gcat(geom())->.comp_equal;
  make equal_tags tag="comp_vertex_diff",::geom=point.gcat(geom())->.comp_vertex_diff;
  make equal_tags tag="link_equal",::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001))->.link_equal;
  make equal_tags tag="link_last_diff",::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48.0005,10.9999))->.link_last_diff;
  make equal_tags tag="link_first_diff",::geom=lstr(pt(48.0005,11.0001),pt(48.001,11),pt(48,11.001))->.link_first_diff;
  make equal_tags tag="link_middle_diff",::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001))->.link_middle_diff;
  make equal_tags tag="poly_equal_1",::geom=poly(lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)))->.poly_equal_1;
  make equal_tags tag="poly_equal_2",::geom=poly(
      lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)),
      lstr(pt(48,13),pt(48,13.001),pt(48.001,13.001)))->.poly_equal_2;
  make equal_tags tag="poly_diff_1",::geom=poly(
      lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)),
      lstr(pt(48,13),pt(48,13.001),pt(48.001,13.001)))->.poly_diff_1;
  make equal_tags tag="poly_diff_2",::geom=poly(
      lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)),
      lstr(pt(48,13),pt(48,13.001),pt(48.001,13.001)))->.poly_diff_2;
);
if (n.count(nodes) == 0)
(
  make equal_tags tag="no_geom",::geom=pt(48,12)->.no_geom;
  make equal_tags tag="center",::geom=pt(48,12.006)->.center;
  make equal_tags tag="comp_vs_vertex",::geom=center(point.gcat(geom()))->.comp_vs_vertex;
  make equal_tags tag="comp_equal",::geom=point.gcat(geom())->.comp_equal;
  make equal_tags tag="comp_vertex_diff",::geom=point_bis.gcat(geom())->.comp_vertex_diff;
  make equal_tags tag="link_equal",::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48,11.001))->.link_equal;
  make equal_tags tag="link_last_diff",::geom=lstr(pt(48,10.999),pt(48.001,11),pt(48.0006,10.9999))->.link_last_diff;
  make equal_tags tag="link_first_diff",::geom=lstr(pt(48.0006,11.0001),pt(48.001,11),pt(48,11.001))->.link_first_diff;
  make equal_tags tag="link_middle_diff",::geom=lstr(pt(48,10.999),pt(48.001,11.0001),pt(48,11.001))->.link_middle_diff;
  make equal_tags tag="poly_equal_1",::geom=poly(lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)))->.poly_equal_1;
  make equal_tags tag="poly_equal_2",::geom=poly(
      lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)),
      lstr(pt(48,13),pt(48,13.001),pt(48.001,13.001)))->.poly_equal_2;
  make equal_tags tag="poly_diff_1",::geom=poly(
      lstr(pt(48,12),pt(48.0001,12.001),pt(48.001,12.001)),
      lstr(pt(48,13),pt(48,13.001),pt(48.001,13.001)))->.poly_diff_1;
  make equal_tags tag="poly_diff_2",::geom=poly(
      lstr(pt(48,12),pt(48,12.001),pt(48.001,12.001)),
      lstr(pt(48,13),pt(48.0001,13.001),pt(48.001,13.001)))->.poly_diff_2;
);
.no_geom out geom;
.center out geom;
.comp_vs_vertex out geom;
.comp_equal out geom;
.comp_vertex_diff out geom;
.link_equal out geom;
.link_last_diff out geom;
.link_first_diff out geom;
.link_middle_diff out geom;
.poly_equal_1 out geom;
.poly_equal_2 out geom;
.poly_diff_1 out geom;
.poly_diff_2 out geom;
