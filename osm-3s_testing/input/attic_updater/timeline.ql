timeline(node,1101);out;
timeline(node,1102);out;
timeline(node,1201);out;
timeline(node,1202);out;
timeline(node,1203);out;
timeline(node,1301);out;
timeline(node,1302);out;
timeline(node,1303);out;
timeline(node,1304);out;
timeline(node,1401);out;
timeline(node,1402);out;
timeline(node,1403);out;
timeline(node,1404);out;
timeline(node,1501);out;
timeline(node,1502);out;
timeline(node,1503);out;
timeline(node,1504);out;
timeline(node,1505);out;
timeline(way,1101);out;
timeline(way,1102);out;
timeline(way,1103);out;
timeline(way,1201);out;
timeline(way,1202);out;
timeline(way,1203);out;
timeline(way,1301);out;
timeline(way,1302);out;
timeline(way,1303);out;
timeline(way,1304);out;
timeline(way,1401);out;
timeline(way,1402);out;
timeline(way,1403);out;
timeline(way,1404);out;
timeline(way,1501);out;
timeline(way,1502);out;
timeline(way,1503);out;
timeline(way,1504);out;
timeline(way,1505);out;
timeline(way,1601);out;
timeline(way,1602);out;
timeline(way,1603);out;
timeline(way,1701);out;
timeline(way,1702);out;
timeline(way,1703);out;
timeline(way,1801);out;
timeline(way,1802);out;
timeline(way,1803);out;
timeline(rel,1111);out;
timeline(rel,1112);out;
timeline(rel,1113);out;
timeline(rel,1121);out;
timeline(rel,1122);out;
timeline(rel,1123);out;
timeline(rel,1131);out;
timeline(rel,1132);out;
timeline(rel,1133);out;
timeline(rel,1141);out;
timeline(rel,1142);out;
timeline(rel,1143);out;
timeline(rel,1171);out;
timeline(rel,1172);out;
timeline(rel,1173);out;
timeline(rel,1201);out;
timeline(rel,1202);out;
timeline(rel,1203);out;
timeline(rel,1311);out;
timeline(rel,1312);out;
timeline(rel,1313);out;
timeline(rel,1314);out;
timeline(rel,1321);out;
timeline(rel,1322);out;
timeline(rel,1323);out;
timeline(rel,1324);out;
timeline(rel,1325);out;
timeline(rel,1326);out;
timeline(rel,1327);out;
timeline(rel,1331);out;
timeline(rel,1332);out;
timeline(rel,1333);out;
timeline(rel,1334);out;
timeline(rel,1411);out;
timeline(rel,1412);out;
timeline(rel,1413);out;
timeline(rel,1414);out;
timeline(rel,1421);out;
timeline(rel,1422);out;
timeline(rel,1423);out;
timeline(rel,1424);out;
timeline(rel,1425);out;
timeline(rel,1426);out;
timeline(rel,1427);out;
timeline(rel,1431);out;
timeline(rel,1432);out;
timeline(rel,1433);out;
timeline(rel,1434);out;
timeline(rel,1501);out;
timeline(rel,1502);out;
timeline(rel,1503);out;
timeline(rel,1504);out;
timeline(rel,1505);out;
timeline(rel,1611);out;
timeline(rel,1612);out;
timeline(rel,1613);out;
timeline(rel,1621);out;
timeline(rel,1622);out;
timeline(rel,1623);out;
timeline(rel,1631);out;
timeline(rel,1632);out;
timeline(rel,1633);out;
timeline(rel,1641);out;
timeline(rel,1642);out;
timeline(rel,1643);out;
timeline(rel,1671);out;
timeline(rel,1672);out;
timeline(rel,1673);out;
timeline(rel,1711);out;
timeline(rel,1712);out;
timeline(rel,1713);out;
timeline(rel,1721);out;
timeline(rel,1722);out;
timeline(rel,1723);out;
timeline(rel,1731);out;
timeline(rel,1732);out;
timeline(rel,1733);out;
timeline(rel,1741);out;
timeline(rel,1742);out;
timeline(rel,1743);out;
timeline(rel,1771);out;
timeline(rel,1772);out;
timeline(rel,1773);out;
timeline(rel,1811);out;
timeline(rel,1812);out;
timeline(rel,1813);out;
timeline(rel,1821);out;
timeline(rel,1822);out;
timeline(rel,1823);out;
timeline(rel,1831);out;
timeline(rel,1832);out;
timeline(rel,1833);out;
timeline(rel,1841);out;
timeline(rel,1842);out;
timeline(rel,1843);out;
timeline(rel,1871);out;
timeline(rel,1872);out;
timeline(rel,1873);out;
timeline(rel,1901);out;
timeline(rel,1911);out;
timeline(rel,1921);out;
timeline(rel,1922);out;
timeline(rel,1941);out;

make marker retro="2013-07-01T09:02:00Z"; out;
retro("2013-07-01T09:02:00Z")
(
  node(10,4,10.1,4.1); out meta geom;
  way(10,6,10.1,6.1); out meta geom;
  rel(10,6,10.8,6.1); out meta geom;
);

make marker retro="2013-07-01T09:03:00Z"; out;
retro("2013-07-01T09:03:00Z")
(
  node(10,4,10.1,4.1); out meta geom;
  way(10,6,10.1,6.1); out meta geom;
  rel(10,6,10.8,6.1); out meta geom;
);

make marker retro="2013-07-01T09:04:00Z"; out;
retro("2013-07-01T09:04:00Z")
(
  node(10,4,10.1,4.1); out meta geom;
  way(10,6,10.1,6.1); out meta geom;
  rel(10,6,10.8,6.1); out meta geom;
);

make marker retro="2013-07-01T09:05:00Z"; out;
retro("2013-07-01T09:05:00Z")
(
  node(10,4,10.1,4.1); out meta geom;
  way(10,6,10.1,6.1); out meta geom;
  rel(10,6,10.8,6.1); out meta geom;
);

make marker full_history="node 1402"; out;
timeline(node,1402);
foreach(
  out;
  retro(u(t["created"]))
  (
    node(1402); out meta;
  );
);

make marker full_history="way 1505"; out;
timeline(way,1505);
foreach(
  out;
  retro(u(t["created"]))
  (
    way(1505); out geom meta;
    >; out meta;
  );
);

make marker full_history="way 1602"; out;
timeline(way,1602);
foreach(
  out;
  retro(u(t["created"]))
  (
    way(1602); out geom meta;
    >; out meta;
  );
);

make marker full_history="rel 1632"; out;
timeline(rel,1632);
foreach(
  out;
  retro(u(t["created"]))
  (
    rel(1632); out geom meta;
    >; out center meta;
  );
);
