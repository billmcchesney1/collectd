./build.sh
./configure
make dist
mv collectd-*tar.gz fk-ops-collectd-version.tar.gz
tar xf fk-ops-collectd-version.tar.gz
mv collectd-* fk-ops-collectd-version
cd fk-ops-collectd-version
dh_make -e ops-tickets@flipkart.com  -f ../fk-ops-collectd-version.tar.gz
<i>
<enter>
cp fk-ops-collectd.upstart src/collectd.conf
cp fk-ops-collectd.upstart src/collectd.conf
cp fk-ops-collectd.init debian/
cp fk-ops-collectd.default debian/
DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc
