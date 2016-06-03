isoft-package-installer
-----------------------
iSOFT RPM (deb converted to RPM) package installer daemon and Qt frontend.

## Build && Install

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr    \
    -DENABLE_DEBUG=ON
make
sudo make install
```

## Daemon

```
sudo systemctl enable isoft-install-daemon.service
sudo systemctl restart isoft-install-daemon.service

sudo systemctl stop isoft-install-daemon.service
cd build
sudo ./daemon/isoft-install-daemon --debug
```
