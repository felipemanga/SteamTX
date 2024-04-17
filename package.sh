#!/bin/sh

add()
{
  echo "adding $1"

  if [ -e "/usr/lib/x86_64-linux-gnu/$1" ]; then
      src="/usr/lib/x86_64-linux-gnu"
  elif [ -e "/usr/lib/x86_64-linux-gnu/pulseaudio/$1" ]; then
      src="/usr/lib/x86_64-linux-gnu/pulseaudio"
  elif [ -e "/usr/lib64/$1" ]; then
      src="/usr/lib64"
  fi

  if [ $src != $out ]; then
      cp "$src/$1" "$out/$1"
  fi

  dependencies=$(readelf -d "$src/$1" | grep NEEDED | sed -En "s/[^\[]*\[([^]]*)\S*/\1/gp")

  for dependency in $dependencies
  do
    if [ ! -f "$out/$dependency" ]; then
        add $dependency
    fi
  done
}

out=$(pwd)
src=$(pwd)

add "./SteamTX"

cp /usr/lib/libpthread.so* ./
cp /usr/lib/librt.so* ./
cp /usr/lib/libstdc++.so* ./

chmod +x SteamTX

mkdir -p dist/SteamTX/usr/bin
mkdir -p dist/SteamTX/usr/lib

cat > dist/SteamTX/SteamTX <<EOF
#!/bin/sh
SCRIPT=\$(readlink -f "\$0")
SCRIPTPATH=\$(dirname "\$SCRIPT")
cd "\$SCRIPTPATH/usr/bin"
export LD_LIBRARY_PATH=\$SCRIPTPATH/usr/lib
./SteamTX
EOF

chmod +x dist/SteamTX/SteamTX

cp SteamTX dist/SteamTX/usr/bin/
cp config.ini dist/SteamTX/usr/bin/
cp DejaVuSans.ttf dist/SteamTX/usr/bin/
cp SteamTX.desktop dist/SteamTX/
cp SteamTX.png dist/SteamTX/steamtx.png
mv *.so* dist/SteamTX/usr/lib
