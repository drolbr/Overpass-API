(
  make vertex ::geom=pt(51.51,7.47);
  make link ::geom=lstr(pt(51.51,7.47),pt(51.51,7.49)),calcspeed=60;
  make link ::geom=lstr(pt(51.51,7.47),pt(51.52,7.48)),calcspeed=60;
  make link ::geom=lstr(pt(51.51,7.49),pt(51.52,7.48)),calcspeed=60;
);
dijkstra;
out geom;
