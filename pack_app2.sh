#!/bin/bash

#./pack_app2.sh polyvr && ./appimagetool-x86_64.AppImage -n packages/polyvr
#./pack_app2.sh PoscarViewer poscarImport.pvr /c/Users/Victor/Projects/surfacechemistry

appName=$1
appProject=$2
appFolder=$3

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pckFolder="packages/"$appName

if [ ! -e $pckFolder ]; then
	mkdir -p $pckFolder 
fi

rm -rf $pckFolder/*
	
if [ -n "$appFolder" ]; then # check is appFolder given
	echo " copy app data"
	cp -r $appFolder/* $pckFolder/
fi

engFolder=$pckFolder"/engine"
engFolder=$pckFolder

if [ ! -e $engFolder ]; then
	mkdir -p $engFolder 
fi

echo " copy polyvr"
mkdir $engFolder/src
cp -r bin/Debug/VRFramework $engFolder/
cp -r bin/Debug/*.bin $engFolder/
cp -r bin/Debug/*.dat $engFolder/
cp -r src/cluster $engFolder/src/cluster
cp -r ressources $engFolder/ressources
cp -r setup $engFolder/setup
cp -r shader $engFolder/shader
cp -r examples $engFolder/examples
mkdir -p $engFolder/bin/Debug
cp bin/Debug/*.so $engFolder/bin/Debug/

echo " copy libs"
mkdir -p $engFolder/libs
cp -r /usr/lib/opensg/* $engFolder/libs/
cp -r /usr/lib/CEF/* $engFolder/libs/
cp -r /usr/lib/1.4/* $engFolder/libs/
cp -r /usr/lib/virtuose/* $engFolder/libs/
cp -r /usr/lib/STEPcode/* $engFolder/libs/
cp -r /usr/lib/OCE/* $engFolder/libs/
cp -r /usr/lib/OPCUA/* $engFolder/libs/
cp -r /usr/lib/DWG/* $engFolder/libs/

echo " copy system libs"
cp /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/nss/libnssckbi.so $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libpython2.7.so.1.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libboost_program_options.so.1.74.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libboost_filesystem.so.1.74.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libstb.so.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/liblapacke.so.3 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libgcc_s.so.1 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libc.so.6 $engFolder/libs/
#cp /lib/x86_64-linux-gnu/libz.so.1 $engFolder/libs/
#cp /lib/x86_64-linux-gnu/libm.so.6 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libX11.so.6 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libGL.so.1 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libxcb.so.1 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libgmp.so.10 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libGLU.so.1 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/liblapacke.so.3 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libfftw3.so.3 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libjsoncpp.so.25 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libcurl-gnutls.so.4 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libssh2.so.1 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libBulletSoftBody.so.3.05 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libBulletDynamics.so.3.05 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libBulletCollision.so.3.05 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libLinearMath.so.3.05 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libavformat.so.58 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libavcodec.so.58 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libswscale.so.5 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libavutil.so.56 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libHACD.so.3.05 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libicuuc.so.70 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libvtkCommonCore-7.1.so.7.1p $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libvtkIOLegacy-7.1.so.7.1p $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libvtkCommonDataModel-7.1.so.7.1p $engFolder/libs/
cp /lib/libgdal.so.30 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libboost_regex.so.1.74.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libopenal.so.1 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libswresample.so.3 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libmtdev.so.1 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libopenvr_api.so.1 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libfreetype.so.6 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libqrencode.so.4 $engFolder/libs/
#cp /usr/lib/x86_64-linux-gnu/libglib-2.0.so.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libcairo.so.2 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libcrypto++.so.8 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libxml2.so.2 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libmdb.so.3 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libmpfr.so.6 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libraptor2.so.0 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libgomp.so.1 $engFolder/libs/
#cp /lib/x86_64-linux-gnu/libGLdispatch.so.0 $engFolder/libs/
#cp /lib/x86_64-linux-gnu/libGLX.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libXau.so.6 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libXdmcp.so.6 $engFolder/libs/
#cp /lib/x86_64-linux-gnu/libOpenGL.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libblas.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/liblapack.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtmglib.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnghttp2.so.14 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libidn2.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/librtmp.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libssh.so.4 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpsl.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnettle.so.8 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgnutls.so.30 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgssapi_krb5.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libldap-2.5.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/liblber-2.5.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libzstd.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libbrotlidec.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcrypto.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libbz2.so.1.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgme.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libopenmpt.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libchromaprint.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libbluray.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/librabbitmq.so.4 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsrt-gnutls.so.1.4 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libssh-gcrypt.so.4 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libzmq.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvpx.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libwebpmux.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libwebp.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/liblzma.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libdav1d.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/librsvg-2.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgobject-2.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libzvbi.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsnappy.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libaom.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcodec2.so.1.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgsm.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libmp3lame.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libopenjp2.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libopus.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libshine.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libspeex.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtheoraenc.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtheoradec.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtwolame.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvorbis.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvorbisenc.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libx264.so.163 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libx265.so.199 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libxvidcore.so.4 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libva.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libmfx.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libva-drm.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libva-x11.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvdpau.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libdrm.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libOpenCL.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libicudata.so.70 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtksys-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtkIOCore-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtkCommonExecutionModel-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtkCommonTransforms-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtkCommonMisc-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtkCommonSystem-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvtkCommonMath-7.1.so.7.1p $engFolder/libs/
cp /lib/x86_64-linux-gnu/libheif.so.1 $engFolder/libs/
cp /lib/libarmadillo.so.10 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpoppler.so.118 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libjson-c.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libfreexl.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libqhull_r.so.8.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgeos_c.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libodbc.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libodbcinst.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libkmlbase.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libkmldom.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libkmlengine.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libexpat.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libxerces-c-3.2.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnetcdf.so.19 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libhdf5_serial.so.103 $engFolder/libs/
cp /lib/libmfhdfalt.so.0 $engFolder/libs/
cp /lib/libdfalt.so.0 $engFolder/libs/
cp /lib/libogdi.so.4.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgif.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcharls.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgeotiff.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpng16.so.16 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcfitsio.so.9 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpq.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/liblz4.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libblosc.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libproj.so.22 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsqlite3.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtiff.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libjpeg.so.8 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libdeflate.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libspatialite.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpcre2-8.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcurl.so.4 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libfyba.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libmysqlclient.so.21 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libicui18n.so.70 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsndio.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsoxr.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpthread.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpcre.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpixman-1.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libfontconfig.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libxcb-shm.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libxcb-render.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libXrender.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libXext.so.6 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libxslt.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libyajl.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libbsd.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgfortran.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libunistring.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libhogweed.so.6 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libp11-kit.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtasn1.so.6 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libkrb5.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libk5crypto.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcom_err.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libkrb5support.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsasl2.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libbrotlicommon.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libmpg123.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libvorbisfile.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libudfread.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libssl.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgcrypt.so.20 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgpg-error.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsodium.so.23 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpgm-5.3.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnorm.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libcairo-gobject.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgdk_pixbuf-2.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgio-2.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpangocairo-1.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpango-1.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libffi.so.8 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libogg.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnuma.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libXfixes.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libde265.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libarpack.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsuperlu.so.5 $engFolder/libs/
cp /lib/x86_64-linux-gnu/liblcms2.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnss3.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsmime3.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libplc4.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnspr4.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgeos.so.3.10.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libltdl.so.7 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libminizip.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/liburiparser.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libhdf5_serial_hl.so.100 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libsz.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libtirpc.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libdl.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libjbig.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/librttopo.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libfyut.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libfygm.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libresolv.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libasound.so.2 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libuuid.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libmd.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libquadmath.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libkeyutils.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgmodule-2.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libmount.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libselinux.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libpangoft2-1.0.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libharfbuzz.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libfribidi.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libthai.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libnssutil3.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libplds4.so $engFolder/libs/
cp /lib/x86_64-linux-gnu/libaec.so.0 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libblkid.so.1 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libgraphite2.so.3 $engFolder/libs/
cp /lib/x86_64-linux-gnu/libdatrie.so.1 $engFolder/libs/
cp /usr/lib/x86_64-linux-gnu/libglut.so.3 $engFolder/libs/

rm -rf $engFolder/libs/CMakeFiles



if [ -e $pckFolder/cleanupDeploy.sh ]; then
	/bin/bash $pckFolder/cleanupDeploy.sh 
fi



if [ -n "$appProject" ]; then # check is appProject given
cat <<EOT >> $pckFolder/AppRun
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\${0}")")"
echo "AppRun PolyVR"
echo "work dir: '\$HERE'"
cd \$HERE
ls
export LD_LIBRARY_PATH="\${HERE}/libs:\${LD_LIBRARY_PATH}"
exec ./VRFramework --maximized=1 --application $appProject "\$@"
EOT
else
cat <<EOT >> $pckFolder/AppRun
#!/bin/sh
HERE="\$(dirname "\$(readlink -f "\${0}")")"
echo "AppRun PolyVR"
echo "work dir: '\$HERE'"
cd \$HERE
ls
export LD_LIBRARY_PATH="\${HERE}/libs:\${LD_LIBRARY_PATH}"
exec ./VRFramework "\$@"
EOT
fi



chmod +x $pckFolder/AppRun

cat <<EOT >> $pckFolder/appimage.yml
app:
  name: $appName
  version: 1.0
  exec: PolyVR.sh
  icon: logo_icon.png
EOT

cat <<EOT >> $pckFolder/PolyVR.desktop
[Desktop Entry]
Type=Application
Name=$appName
Terminal=true
MimeType=application/x-polyvr
Categories=Development
Path=$DIR/$engFolder
Icon=logo_icon
EOT
#Icon=$DIR/$engFolder/ressources/gui/logo_icon

cp $engFolder/ressources/gui/logo_icon.png $pckFolder/logo_icon.png


echo " done"

