# Omniscope Mutliviewer for the Browser

##### Install:
in your terminal, write 

```
npm install
```

for the Omniscope APP 

```
cd 

```

## Showcase

![](./IMAGES/REACTAPP.gif)

## For the raspberry pi

```bash
sudo apt update
sudo apt upgrade

sudo apt install -y ca-certificates curl GnuPG
curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | sudo gpg --dearmor -o /usr/share/keyrings/nodesource.gpg
NODE_MAJOR=18
echo "deb [signed-by=/usr/share/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | sudo tee /etc/apt/sources.list.d/nodesource.list
sudo apt update
sudo apt install nodejs
node -v
```


## For the Jetson Nano (Jetpack)

https://github.com/nodesource/distributions/issues/1392#issuecomment-1749131791
```bash
# Start by installing Node 20:

sudo  curl -L https://raw.githubusercontent.com/tj/n/master/bin/n -o n
bash n 20

# Node 20 is now at /usr/local/bin/node, but glibc 2.28 is missing:
# node: /lib/aarch64-linux-gnu/libc.so.6: version `GLIBC_2.28' not found (required by node)
# /usr/local/bin/node: /lib/aarch64-linux-gnu/libc.so.6: version `GLIBC_2.28' not found (required by /usr/local/bin/node)

# Build and install glibc 2.28:
sudo apt install -y gawk
cd ~
wget -c https://ftp.gnu.org/gnu/glibc/glibc-2.28.tar.gz
tar -zxf glibc-2.28.tar.gz
cd glibc-2.28
mkdir build-glibc
cd build-glibc
sudo ../configure --prefix=/opt/glibc-2.28

../configure --prefix=/opt/glibc-2.28
sudo make -j 4 # Use all 4 Jetson Nano cores for much faster building
sudo make install
cd ..
rm -fr glibc-2.28 glibc-2.28.tar.gz
 
# Patch the installed Node 20 to work with /opt/glibc-2.28 instead: 
apt install -y patchelf
patchelf --set-interpreter /opt/glibc-2.28/lib/ld-linux-aarch64.so.1 --set-rpath /opt/glibc-2.28/lib/:/lib/aarch64-linux-gnu/:/usr/lib/aarch64-linux-gnu/ /usr/local/bin/node

# Et voilà:
node --version
v20.8.0
```




```bash
git clone https://github.com/Matchboxscope/Omniscope-MultiCamViewer
cd Omniscope-MultiCamViewer/server
npm install
npm run
```

```bash
cd Omniscope-MultiCamViewer/client
npm install
npm run
```

## Electron

```bash
cd electron
npm run electron-start
````
``
```bash
npm install --save-dev electron-builder
npm run dist -- -w
npm run dist -- -m
```
