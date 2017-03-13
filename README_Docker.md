# How to use this image

This image on startup initializes with data on first start.
To do it properly it needs a bit of guidance in form of enviroment variables.
* `OVERPASS_MODE` - takes the value of either `init` or `clone`
* `OVERPASS_META` - `yes`, `no` or `attic` - passed to Overpass as `--meta` or `--keep-attic`
* `OVERPASS_DIFF_URL` - url to diff's directory for updating the instance (eg. https://planet.openstreetmap.org/replication/minute/)
* `OVERPASS_PLANET_URL` - url to "planet" file in `init` mode
* `OVERPASS_PLANET_SEQUENCE_ID` - sequence identifier corresponding to planet file above. All files after this one will be applied
* `OVERPASS_COMPRESSION` - takes values of `no`, `gz` or `lz4`. Specifies compression mode of the Overpass database. 
Ony useful in `init` mode. Defaults to `gz`

Image works in two modes `clone` or `init`. This affects how the instance gets initialized. If the mode is set to `clone`
then data is copied from http://dev.overpass-api.de/api_drolbr/ and then updated from diffs. This will result in Overpass instance
covering whole world. This mode works only with minute diffs.

In `init` mode you need to point `OVERPASS_PLANET_URL` to address with planet (partial) dump. This file will be downloaded,
indexed by Overpass and later - updated using `OVERPASS_PLANET_SEQUENCE_ID` and `OVERPASS_DIFF_URL`. You need to check which
sequence number is for your planet file. Take it from desctiption or just take a sequence number a day before your planet
file is dated.

Start initalization mode with `-i` and `-t` options to `docker run` so you will have a chance to monitor the progress of
file downloads etc.

After initialization is finished Docker container will stop. Once you start it again (with `docker start` command) it will start
downloading minute diffs, applying them to database and serving API requests.

Container exposes port 80.

All data resides within /db directory in container.

# Examples
## Overpass instance covering part of the world
In this example Overpass instance will be initialized with planet file for Poland downloaded from Geofabrik. Data will be stored in folder
`/big/docker/overpass_db/` on the host machine. Overpass will be available on port 12345 on host machine.
```
docker run \
  -e OVERPASS_META=yes \
  -e OVERPASS_MODE=init \
  -e OVERPASS_PLANET_URL=http://download.geofabrik.de/europe/poland-latest.osm.bz2 \
  -e OVERPASS_DIFF_URL=http://download.openstreetmap.fr/replication/europe/poland/minute/ \
  -e OVERPASS_PLANET_SEQUENCE_ID=2346486 \
  -v /big/docker/overpass_db/:/db \
  -p 12345:80 \
  -i -t \
  --name overpass_poland overpass
```

## Overpass clone covering whole world
In this example Overpass instance will be initialized with data from main Overpass instance and updated with master planet diffs.
Data will be stored in /big/docker/overpass_clone_db/ directory on the host machine and API will be exposed on port 12346 on host machine.
```
docker run \
  -e OVERPASS_META=yes
  -e OVERPASS_MODE=clone
  -e OVERPASS_DIFF_URL=https://planet.openstreetmap.org/replication/minute/
  -v /big/docker/overpass_clone_db/:/db
  -p 12346:80
  -i -t
  --name overpass_world
  overpass
```
