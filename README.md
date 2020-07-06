# ODS2

## Building on Linux
```
make -f makefile.unix
```
## Usage
### Mount CD/DVD/Disk image
```
$> mount sr0
$> mount /dev/sr0
$> mount <image_file>
```
### Navigate contents
```
$> dir
$> set default [XYZ]
$> set default [.ABC]
$> set default [XYZ.ABC]
```
### Copy everything (flat hierarchy)
```
$> copy [...]*.* *.*
```
### Copy everything (preserve hierarchy)
```
$> copy [...]*.* [*]*.*
```

## Credits

Paul Nankervis / Hunter Goatley

Original README(s) in [aaareadme.txt](aaareadme.txt) and [aaareadme.too](aaareadme.too)

