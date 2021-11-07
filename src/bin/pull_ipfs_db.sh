#!/bin/bash

CLONE_DIR=
REMOTE_DIR=/ipfs # ipfs mount point (defaults to /ipfs)
ROOT=$REMOTE_DIR/QmZbSqT3mybJcUdkwsA1knNyrZwFcgfY9n8ftPyrk5kQzo 

if [[ -z $1 ]]; then
{
  echo "Usage: $0 --db-dir=database_dir"
  exit 0
}; fi

process_param()
{
  if [[ "${1:0:9}" == "--db-dir=" ]]; then
  {
    CLONE_DIR="${1:9}"
  };
  else
  {
    echo "Unknown argument: $1"
    exit 0
  };
  fi
};

if [[ -n $1  ]]; then process_param $1; fi

mkdir $CLONE_DIR && cd $CLONE_DIR

ln -s $REMOTE_DIR/QmUcYDcbEZTdYPntdWcTTvtpQjQjLahxzxdNccYSYTcddt base-url
ln -s $REMOTE_DIR/QmQATyEb87YYQqKkn1htnLL7Bhs9AU1PisxP13D572Y4cg node_keys.bin
ln -s $REMOTE_DIR/QmdhAz1qcKp5mTyXNsxo8Kq9kf31y1d589eKHaUd3EUXFM node_keys.bin.idx
ln -s $REMOTE_DIR/QmR3tWHHB2RhPdvCLpNJFZFadeyAsFiyJjwtpKp4mEePFN node_tags_global.bin
ln -s $REMOTE_DIR/QmNepf4xxysUr1U37BmFnzggshbyUqrXFQrM7DGB3PsCRk node_tags_global.bin.idx
ln -s $REMOTE_DIR/QmRhhPeFhStedHJrs85emk81TuV1zquToDgDnMXKe8pHbv node_tags_local.bin
ln -s $REMOTE_DIR/QmZJwW3rKLR2SZ8AtCUPTp8qEhrFPCmujp5X4KVxbnu2kz node_tags_local.bin.idx
ln -s $REMOTE_DIR/QmR95JfiRSaPRDyZ2o76vejMToZfkR4jjpsWB5SLCR1ABz nodes.bin
ln -s $REMOTE_DIR/QmRcQG1g75Z3UXd6Me9woUmkFqnqn7MCFp1uPdnDwLpgez nodes.bin.idx
ln -s $REMOTE_DIR/QmY1umkdnQdYHp2xcSTBobStrSAMzAZi5dtDhcLf8yCNBN nodes.map
ln -s $REMOTE_DIR/QmQRdvQMmegjcuMub4R49twAMccExfb8mneCCvof9rqVZX nodes.map.idx
ln -s $REMOTE_DIR/QmY3AkWDX6nmtKKK9oRWYDZRyX7q718ikP6TnyuLhq4RzW relation_keys.bin
ln -s $REMOTE_DIR/QmXZwpLseeGtaVkR6K1objSAemabBGVCSjXSgDddmo8CsL relation_keys.bin.idx
ln -s $REMOTE_DIR/QmfGXUtkmtGt34XkkRRnagWGaq49qZgFdKZiPjmdnVwCHv relation_roles.bin
ln -s $REMOTE_DIR/QmVkzh5ap44T7RE4G47oc5642auFPNXLTxb2Xt6rR1vKR1 relation_roles.bin.idx
ln -s $REMOTE_DIR/QmS24ECkiBWCUaDistECUXXJPwU11djk22kcDKfaew54xd relation_tags_global.bin
ln -s $REMOTE_DIR/QmcApoV6Yz5GdxQoNPx4R1qut2CxHvgmtf8BkvxunP79yd relation_tags_global.bin.idx
ln -s $REMOTE_DIR/QmauwtpFDB3k32XVroHTFgFMWYBEQ1CcrnSPFnAhYPnUaf relation_tags_local.bin
ln -s $REMOTE_DIR/Qmd1vLRRcja5JNqexFRZ6L2ZQxRA7tL1mJA3NtyBBTHhFD relation_tags_local.bin.idx
ln -s $REMOTE_DIR/QmZvMtmummcaotJijc1zy6KM6f7BtCRL1d4zAbWo9Hg4FD relations.bin
ln -s $REMOTE_DIR/QmaXWjDJhxBaA2M43MtVHjDuWdCJJBArMcScHfUHTDp98k relations.bin.idx
ln -s $REMOTE_DIR/QmcefSkDYQJd4swVzuXsaSr2874fQyLE4M543LEAHm1Ln8 relations.map
ln -s $REMOTE_DIR/QmdkUBvEYjo7n1iPhWDPHJsdxqwrc3mw9re3LEFnq99JNh relations.map.idx
ln -s $REMOTE_DIR/QmZLskyetLso3ZsYVrmhCtTmZcwPmSWkxzqMkEbiZEyt1e replicate_id
ln -s $REMOTE_DIR/QmZG4Ry7P5mXo6v4cR5mnWqcRLSxdWx8bMYz7KAcX1VBN1 transactions.log
ln -s $REMOTE_DIR/QmcBFpr2fdAXJmpgoj7uL5vVAbXbqFwj2cMxijkxo7g5bT way_keys.bin
ln -s $REMOTE_DIR/QmYYvM8xt6EDX2q2FpC59vRuSe9BxBWQMFZWjXVdoY3BA3 way_keys.bin.idx
ln -s $REMOTE_DIR/QmNu24Bth8TV7qUNScS1jM91rSkHV5Q1B6hj8PDqJXMpnD way_tags_global.bin
ln -s $REMOTE_DIR/QmQbuutByhcToz2yayeYaQRUSpC3HycPe4n5RYZBPCrFjY way_tags_global.bin.idx
ln -s $REMOTE_DIR/QmU8paQF59Yjxj83Ft1katmn4DvFPZrbLEWoVe2Dxk9zQi way_tags_local.bin
ln -s $REMOTE_DIR/QmRCZDv5j74MELwX3QEx1pg97KT5xrKNKz7zfudsTTDTeY way_tags_local.bin.idx
ln -s $REMOTE_DIR/QmTPCGrNBqRYZCwvhjNuQcGXJLoJuKsdjZV8mFuE1NE2AF ways.bin
ln -s $REMOTE_DIR/QmXFPKyLxcrZTR4TYgV7C2oszZV2yZw7E7CLWC9Q8cku9J ways.bin.idx
ln -s $REMOTE_DIR/QmXR7Ty8a9ydwopVSuLAz78DDrapLKWby8GuFXKqEVzaBm ways.map
ln -s $REMOTE_DIR/QmUWPTjT6VN7PFTDWfJtqn41zSbVxqxGCzFG2xSQfdWxY5 ways.map.idx
