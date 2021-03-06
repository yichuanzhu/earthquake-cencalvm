# Installation instructions

We have prebuilt packages for the following systems:

* Linux system x86_64 (GLIBC 2.12 or later)
* Darwin Intel Mac OS X 10.10 or later
* Windows 10

If you are using another system or what to use the API, then you will
need to install from source.

## Installing from source

See [Installation
instructions](https://github.com/usgs/earthquake-cencalvm/blob/master/INSTALL.md)
in the top-level directory for how to install from source.

## Installing the binary package

### Linux x86_64 (GLIBC 2.12 or later) and Darwin Mac OS X (10.10 or later)

1. Open a terminal window and change to the directory where you want
to place the distribution.

```
$ cd  $HOME
$ mkdir cencalvm
$ cd cencalvm
```

2. Download the Linux or Mac OS X (Darwin) tarball from
[GitHub](https://github.com/usgs/earthquake-cencalvm/releases) and save
it to the desired location, e.g., `$HOME/cencalvm`.

3. Unpack the tarball.
```
# Linux
$ tar -xzf cencalvm-1.1.0-Linux-x86_64.tgz
# Mac OS X
$ tar -xzf cencalvm-1.1.0-Darwin-x86_64.tgz
```
4. Set environment variables. The provided `setup.sh`
script only works if you are using bash shell. If you are using a
different shell, you will need to alter how the environment
variables are set in `setup.sh`. 
```
$ cd cencalvm-1.1.0
$ source setup.sh
```

**IMPORTANT**: You will need to either source the `setup.sh` script
each time you open a new bash shell (terminal) window or add the
environment variables to your shell setup script (for example,
`.bashrc`).

**WARNING**: The binary distribution contains cencalvm and all of its
dependencies (Proj4 and Euclid). If you have any of this software
already installed on your system, you need to be careful in setting up
your environment so that preexisting software does not conflict with
the cencalvm binary. By default the `setup.sh` script will prepend to
the `PATH` (for Darwin and Linux) and `LD_LIBRARY_PATH` (for Linux)
environment variables. This will prevent most conflicts.

See the Applications section of the [Query Interface](query.md) for
how to use the applications provided in the cencalvm binary package.

### Windows 10

We do not provide a Windows 10 native binary package. Instead, you can
use the Linux binary with the Windows Subsystem for Linux.

To enable the Linux subsystem on Windows 10:

1. Go to `Settings` -> `Security`.

2. Under `For developers` select `Developer mode`. This step should
not be required for Windows build 16215 and later.
  
3. Go to `Control Panel` -> `Programs` -> `Turn Windows Features On or
Off`.

4. Enable `Windows Subsystem for Linux` and click `OK`.

5. Restart the computer.

6. Go to `Start` -> `bash`. You will be prompted to download `Bash on
  Ubuntu on Windows` from the Windows Store. Create a user account and
  password for the bash environment.

7. Install the cencalvm Linux binary within the bash environment
  following the instructions for installing the cencalvm binary for
  Linux. You will run the cencalvm software within the bash
  environment just like you would for a Linux operating system.

## Download the velocity model(s)

Download the velocity model database file(s) from [ftp://ehzftp.wr.usgs.gov/baagaard/cencalvm/database](ftp://ehzftp.wr.usgs.gov/baagaard/cencalvm/database).

* [USGSBayAreaVM-08.3.0.etree.gz](ftp://ehzftp.wr.usgs.gov/baagaard/cencalvm/database/USGSBayAreaVM-08.3.0.etree.gz) Database file for the San Francisco Bay area seismic velocity (**REQUIRED**)
* [USGSBayAreaVMExt-08.3.0.etree.gz](ftp://ehzftp.wr.usgs.gov/baagaard/cencalvm/database/USGSBayAreaVMExt-08.3.0.etree.gz) Database file for the extension of the San Francisco Bay area seismic velocity model to the surrounding region (**OPTIONAL**)
* [MD5SUMS_GZIPPED](ftp://ehzftp.wr.usgs.gov/baagaard/cencalvm/database/MD5SUMS_GZIPPED) Checksums to verify the integrity of the compressed files
* [MD5SUMS](ftp://ehzftp.wr.usgs.gov/baagaard/cencalvm/database/MD5SUMS) Checksums to verify the integrity of the uncompressed files

### Check the integrity of the database files:

```
# Before uncompressing the files
$ md5sum -c MD5SUMS_GZIPPED

# After uncompressing the files
$ md5sum -c MD5SUMS
```