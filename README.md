# librepp

Woodley Packard's C implementation of REPP (the Regular Expression Pre-Processor). It ships as a shared/static library, `librepp`, plus a small `repp` command-line tool. [ACE](https://sweaglesw.org/linguistics/ace/) links against this library to run a grammar's REPP preprocessor, e.g. the ERG's `rpp/*.rpp`.

This is **not** [delph-in/repp](https://github.com/delph-in/repp). That repository holds a separate C++ implementation of the same formalism by Rebecca Dridan and Stephan Oepen, with different dependencies (ICU, Boost Program_options/Filesystem).

For the REPP formalism itself, see [ReppTop](https://github.com/delph-in/docs/wiki/ReppTop) and [Dridan & Oepen (2012)](https://aclanthology.org/P12-2074/).

## Where this code comes from

The upstream source lives in a private CVS repository on Woodley Packard's server (`sweaglesw.org:/a/cvs/`, module `repp`). It was never published as a public repository. This repository reconstructs its history from the release tarballs, one commit per release, with the original author and dates:

| Tag / branch | Date | Source |
|---|---|---|
| `v0.1` | 2009-11-18 | `repp-0.1.tar.gz`, found only inside `repp-mask-trial.tgz` |
| `v0.2` | 2009-11-18 | https://sweaglesw.org/linguistics/repp-0.2.tar.gz |
| `v0.2.1` | 2009-11-18 | https://sweaglesw.org/linguistics/repp-0.2.1.tar.gz |
| `v0.2.2` | 2011-10-19 | https://sweaglesw.org/linguistics/repp-0.2.2.tar.gz |
| branch `mask-trial` | 2020-08-09 | source files of https://sweaglesw.org/linguistics/repp-mask-trial.tgz |

Each commit message records the tarball's SHA-256. The `CVS/` directories included in the tarballs were removed.

`main` is the 0.2.2 release, which is what everyone uses today: the [ACE install instructions](https://github.com/delph-in/docs/wiki/AceInstall), the [Homebrew tap](https://github.com/delph-in/homebrew-delphin) and Docker images. The `mask-trial` branch is a 2020 working copy that adds mask rules (`=` lines) and escaping of `{`/`}` in regular expressions. It may include changes that were never checked in to CVS (see that commit's message). It is kept off `main` until someone who knows the code has reviewed it.

## Building

```sh
autoreconf -fi   # the bundled 2009 libtool mislinks on macOS >= 11
./configure
make
make install
```

Dependencies: a C compiler and Boost.Regex. With Boost >= 1.8x, `<boost/regex.h>` pulls in `<boost/assert.hpp>`, which is C++ only, so C builds fail. See the shim in the Homebrew `repp` formula ([delph-in/homebrew-delphin#25](https://github.com/delph-in/homebrew-delphin/pull/25)) for a workaround.

## License

The original README says: "This software is free for all to use, under the terms of the LGPL (see: http://www.gnu.org/copyleft/lesser.html)." No specific LGPL version is given.

(C) 2009 Woodley Packard.
