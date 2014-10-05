###
#   Dockerfile for Fossil
###
FROM fedora:21

### Now install some additional parts we will need for the build
# RUN yum update -y && yum clean all
RUN yum install -y gcc make zlib-devel openssl-devel tcl-devel && yum clean all
RUN groupadd -r fossil -g 433 && useradd -u 431 -r -g fossil -d /opt/fossil -s /sbin/nologin -c "Fossil user" fossil

### If you want to build "release", change the next line accordingly.
ENV FOSSIL_INSTALL_VERSION trunk

RUN curl "http://www.fossil-scm.org/index.html/tarball/fossil-src.tar.gz?name=fossil-src&uuid=${FOSSIL_INSTALL_VERSION}" | tar zx
RUN cd fossil-src && ./configure --lineedit=0 --json --with-tcl --with-tcl-stubs --with-tcl-private-stubs && make;
RUN cp fossil-src/fossil /usr/bin
RUN rm -rf fossil-src
RUN chmod a+rx /usr/bin/fossil
RUN mkdir -p /opt/fossil
RUN chown fossil:fossil /opt/fossil

### Build is done, remove modules no longer needed
RUN yum remove -y gcc make zlib-devel openssl-devel tcl-devel && yum clean all

USER fossil

ENV HOME /opt/fossil

RUN fossil new --empty -A admin /opt/fossil/repository.fossil
RUN fossil user password -R /opt/fossil/repository.fossil admin admin
RUN fossil cache init -R /opt/fossil/repository.fossil

EXPOSE 8080

CMD ["/usr/bin/fossil", "server", "/opt/fossil/repository.fossil"]
