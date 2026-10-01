FROM golang:1.24.8-bookworm AS minio-builder

ARG MINIO_VERSION=RELEASE.2025-10-15T17-29-55Z
RUN CGO_ENABLED=0 GOBIN=/out go install github.com/minio/minio@${MINIO_VERSION}

FROM ghcr.io/almalinux/almalinux:10-kitten-20260104

RUN dnf install -y --setopt=install_weak_deps=False \
    ca-certificates \
    tzdata \
    && dnf clean all \
    && rm -rf /var/cache/dnf

COPY --from=minio-builder /out/minio /usr/local/bin/minio

RUN useradd -r minio-user && \
    mkdir -p /data && \
    chown minio-user:minio-user /data

EXPOSE 9000 9001

USER minio-user

ENTRYPOINT ["/usr/local/bin/minio"]
CMD ["server", "/data", "--console-address", ":9001"]
