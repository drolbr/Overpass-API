#!/bin/bash

if [[ -z $1  ]]; then
{
  echo -e "\n\
Usage: $0\n\
    --run=REPLICATE_ID --db-dir=DB_DIR --replicate-dir=REPLICATE_DIR\n\
    --minute-url=MINUTE_URL\n\
where\n\
\n\
--run=REPLICATE_ID\n\
    The \$REPLICATE_ID to start the minute diffs from.\n\
\n\
--db-dir=DB_DIR\n\
    The directory where the database is stored. There need to be about 60 GB\n\
    space for the whole planet.\n\
\n\
--replicate-dir=REPLICATE_DIR\n\
    The directory where the minute diffs are stored. The minute diffs can be\n\
    deleted after they are applied.\n\
\n\
--minute-url=MINUTE_URL\n\
    Specify the URL to download the minute diffs from. The usual URL is\n\
    http://planet.openstreetmap.org/minute-replicate/\n\
\n\
"
  exit 0
};
fi

RUN=
DB_DIR=
REPLICATE_DIR=
DOWNLOAD_DIFFS=

process_param()
{
  if [[ "${1:0:6}" == "--run=" ]]; then
  {
    RUN="${1:6}"
  };
  elif [[ "$1" == "--db-dir=" ]]; then
  {
    DB_DIR="${1:8}"
  };
  elif [[ "$1" == "--replicate-dir=" ]]; then
  {
    REPLICATE_DIR="${1:16}"
  };
  elif [[ "$1" == "--minute-url=" ]]; then
  {
    DOWNLOAD_DIFFS="${1:13}"
  };
  fi
};

if [[ -n $1  ]]; then process_param $1; fi
if [[ -n $2  ]]; then process_param $2; fi
if [[ -n $3  ]]; then process_param $3; fi
if [[ -n $4  ]]; then process_param $4; fi

if [[ -z $RUN ]]; then
{
  echo "Error: first_diff must be nonempty."
  exit 0
};
fi

if [[ -z $DB_DIR ]]; then
{
  echo "Error: database_dir must be nonempty."
  exit 0
};
fi

if [[ -z $REPLICATE_DIR ]]; then
{
  echo "Error: replicate_dir must be nonempty."
  exit 0
};
fi

if [[ -z $REPLICATE_DIR ]]; then
{
  echo "Error: You need to provide an URL to download the minute diffs from."
  exit 0
};
fi

./fetch_osc.sh $RUN $DOWNLOAD_DIFFS $REPLICATE_DIR &
./dispatcher --osm-base --db-dir=$DB_DIR/ &
sleep 5
./apply_osc_to_db.sh $DB_DIR/ $REPLICATE_DIR $RUN &
