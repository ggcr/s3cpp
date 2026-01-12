FROM ghcr.io/almalinux/almalinux:10-kitten-20260104

RUN dnf install -y --setopt=install_weak_deps=False \
    gcc \
    gcc-c++ \
    make \
    cmake \
    openssl-devel \
    libcurl-devel \
    git \
    wget \
    && wget https://dl.min.io/server/minio/release/linux-amd64/minio -O /usr/local/bin/minio \
    && chmod +x /usr/local/bin/minio \
    && dnf clean all \
    && rm -rf /var/cache/dnf

RUN useradd -r minio-user && \
    mkdir -p /data && \
    chown minio-user:minio-user /data

EXPOSE 9000 9001

USER minio-user

ENTRYPOINT ["/usr/local/bin/minio"]
CMD ["server", "/data", "--console-address", ":9001"]

