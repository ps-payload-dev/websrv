# websrv

This is a simple web server that can be executed on a PS4 or PS5 that has been
jailbroken and is running an ELF loader. It can be used to launch homebrew via
the browser embedded with either console, or remotely from your PC, phone, etc.
It serves on port 8080.

This project is based on [John Törnblom](https://github.com/john-tornblom)'s
[websrv project][upstream]. It builds with the unified [payload SDK][sdk] and
uses [elfldr][elfldr] and [lib-ptrace][ptrace].

## Quick-start

To deploy websrv, first make sure [elfldr][elfldr] is running, then load the
payload as follows (substitute `ps4` for `ps5` where appropriate):

```console
PS5_HOST=ps5
PS5_PORT=9021
nc -q0 "$PS5_HOST" "$PS5_PORT" < build/ps5/websrv-ps5.elf
```

On the PS4 and PS5, websrv adds a "Homebrew Loader" tile to the home screen the
first time it runs. On the PS4, the tile starts websrv if it is not running and
opens the page in the browser. On the PS5, install the [launcher PKG][launcher]
or point your browser to one of the available services exemplified below.

## Available services

Examples:

- http://ps5:8080/index.html - Launch Homebrew
- http://ps5:8080/elfldr - Launch ELF Payloads
- http://ps5:8080/fs/ - Browse the local filesystem (html)
- http://ps5:8080/fs/?fmt=json - Browse the local filesystem (json)
- http://ps5:8080/fs/system_ex/app/NPXS40028/redis.conf - Download a local file
- http://ps5:8080/mdns - List mDNS services discovered by websrv (json)
- http://ps5:8080/smb?addr=192.168.1.1 - List shares on a remote SMB host (json)
- http://ps5:8080/smb/share?addr=192.168.1.1 - List files and folders shared by a remote SMB host (json)
- http://ps5:8080/smb/share/file?addr=192.168.1.1 - Download a remote SMB file via websrv

## Installing Homebrew

The web server will search for homebrew in `/data/homebrew`,
`/mnt/usb%d/homebrew` and `/mnt/ext%d/homebrew`, and makes a couple of
assumptions on the file structure. More specifically, suppose you have a
homebrew called MyHomebrew, the loader assumes that the following files exist:

- `/data/homebrew/MyHomebrew/eboot.elf` - Payload to run, compiled with the [payload SDK][sdk].
- `/data/homebrew/MyHomebrew/sce_sys/icon0.png` - An icon to be rendered at `/index.html`.

You can also add your own custom UI extensions by specifying a javascript file
named `/data/homebrew/MyHomebrew/homebrew.js`. This is useful when the payload
accepts different command line options, e.g., an emulator that expects
arguments for loading ROMs. For an example of the available capabilities, see
the [demo][demo]. For real-world homebrew, check out the
[upstream releases][releases] and [ps5-ezremote-client][ezremote].

## Building

Assuming you have the [payload SDK][sdk] installed on a Debian-flavored
operating system, the payload can be compiled using the following commands. The
libraries that websrv links against (libmicrohttpd, libmicrodns and libsmb2)
come from the [pacbrew repository][pacbrew]:

```console
export PS5_PAYLOAD_SDK=/opt/ps-payload-sdk/ps5
make PLATFORM=ps5
```

For the PS4, set `PS4_PAYLOAD_SDK=/opt/ps-payload-sdk/ps4` and use
`make PLATFORM=ps4`. Build outputs are under `build/<platform>/`: PS4 produces
`build/ps4/websrv-ps4.elf` and PS5 produces `build/ps5/websrv-ps5.elf`. Object
files are under `build/<platform>/objects/`. Clean with
`make PLATFORM=ps5 clean` and format the sources with
`make PLATFORM=ps5 format`. One GitHub Actions workflow builds the PS5 target
with the [payload SDK][sdk] and the pacbrew libraries, and uploads the
`websrv-ps5` artifact.

The Makefile clones [elfldr][elfldr] and [lib-ptrace][ptrace] into `deps/` as
needed and builds them for the selected console.

The homebrew samples live under `homebrew/`: `homebrew/ps4/` and `homebrew/ps5/`
hold the samples for each console, and `homebrew/shared/` holds what both consoles
use (the build helper, the demo, and samples built for either console, such as PKGInstall).
Build the samples of a console with the same platform selector:

```console
make PLATFORM=ps4 homebrew
make PLATFORM=ps5 homebrew
```

Samples that need SDL, ffmpeg or other third-party packages (everything except
PKGInstall) build with their own workflows. To build and check only the samples
that need nothing but the [payload SDK][sdk], run:

```console
make PLATFORM=ps4 homebrew-test
make PLATFORM=ps5 homebrew-test
```

Sample files are written under `build/<platform>/homebrew/<name>/`. The
`scripts/console-websrv-elfldr.sh` helper can send an ELF to a running websrv.

If you are compiling for Ubuntu 26.04:

```console
sudo apt install libmicrohttpd-dev libsmb2-dev libmicrodns-dev
make -f Makefile.linux
```

## Known Issues

- Homebrew sometimes crashes when there is already a previous homebrew running.

## Reporting Bugs

If you encounter problems with websrv, please [file a github issue][issues].
If you plan on sending pull requests which affect more than a few lines of
code, please file an issue before you start to work on your changes. This will
allow us to discuss the solution properly before you commit time and effort.

## License

websrv uses [xterm.js][xterm.js] in its web interface, which is licensed under
the MIT license. Unless otherwise explicitly specified inside individual files,
the rest of the source code included with websrv is licensed under the GPLv3+.

[sdk]: https://github.com/ps-payload-dev/sdk
[elfldr]: https://github.com/ps-payload-dev/elfldr
[ptrace]: https://github.com/ps-payload-dev/lib-ptrace
[pacbrew]: https://github.com/ps5-payload-dev/pacbrew-repo
[issues]: https://github.com/ps-payload-dev/websrv/issues/new
[upstream]: https://github.com/ps5-payload-dev/websrv
[xterm.js]: https://github.com/xtermjs/xterm.js
[launcher]: https://github.com/ps5-payload-dev/websrv/raw/refs/heads/master/homebrew/IV9999-FAKE00000_00-HOMEBREWLOADER01.pkg
[demo]: https://github.com/ps5-payload-dev/websrv/tree/master/homebrew/demo
[releases]: https://github.com/ps5-payload-dev/websrv/releases
[ezremote]: https://github.com/cy33hc/ps5-ezremote-client
