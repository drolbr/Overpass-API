FROM nginx:stable

RUN addgroup overpass && adduser --home /db --disabled-password --ingroup overpass overpass

RUN apt-get update \
    && apt-get install --no-install-recommends --no-install-suggests -y \
        autoconf \
        automake \
        expat \
        libexpat1-dev \
        g++ \
        liblz4-1 \
        liblz4-dev \
        libtool \
        m4 \
        make \
        zlib1g \
        zlib1g-dev

COPY . /app/

RUN cd /app/src \
    && autoscan \
    && aclocal \
    && autoheader \
    && libtoolize \
    && automake --add-missing  \
    && autoconf \
    && CXXFLAGS='-O2' CFLAGS='-O2' ./configure --prefix=/app \
    && make -j $(grep -c ^processor /proc/cpuinfo) install clean \
    && apt-get remove -y \
        autoconf \
        automake \
        libexpat1-dev \
        g++ \
        libtool \
        liblz4-dev \
        m4 \
        make \
        zlib1g-dev \
    && apt-get autoremove -y 

RUN apt-get install --no-install-recommends --no-install-suggests -y \
        supervisor \
        bash \
        lftp \
        wget \
        fcgiwrap \
        bzip2


RUN mkdir /nginx && chown nginx:nginx /nginx && mkdir -p /db/db /db/diffs && chown -R overpass:overpass /db
COPY src/rules /db/db/rules
COPY etc/supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY etc/nginx-overpass.conf /etc/nginx/nginx.conf
VOLUME /db

EXPOSE 80
# CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
CMD ["/app/docker-entrypoint.sh"]
