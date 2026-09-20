# Building aMule (search-feature branch)

## Build dependencies (Debian / Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
  libwxgtk3.2-dev \
  libboost-dev libboost-system-dev \
  libcrypto++-dev \
  zlib1g-dev \
  gettext flex bison \
  libupnp-dev
```

### Notes on the required packages

- **wxWidgets** — provides the base/core/adv/net/gui components the build
  selects automatically (see `cmake/options.cmake` `wx_NEED_*` flags).
  On wxWidgets **3.2** the package is `libwxgtk3.2-dev` (the `-gtk3`
  suffix from the old 3.0 packaging is gone). On distros that only ship
  3.0, use `libwxgtk3.0-gtk3-dev` instead. `wx-common` (for `wx-config`)
  is pulled in as a dependency.

  If you are unsure of the exact name, run:
  ```bash
  apt-cache search '^libwxgtk'        # list available wx GTK dev packages
  # or, to confirm the 3.2 dev metapackage exists:
  apt-cache policy libwxgtk3.2-dev
  ```
  and run `sudo apt-get update` first if the package index is stale.
- **Boost** — `libboost-system-dev` is required because the Boost.ASIO
  sockets use `boost::system::error_code` (`cmake/boost.cmake` checks for
  `boost/asio.hpp` + `boost/system/error_code.hpp`).
- **Crypto++** — `libcrypto++-dev` is effectively mandatory: the monolithic
  build enables `NEED_LIB_EC`, which forces `NEED_LIB_CRYPTO`.
- **flex / bison** — required for `Parser.y` / `Scanner.l` in `muleappcore`.
- **gettext** — required since `ENABLE_NLS` defaults ON (provides `msgfmt` /
  `msgmerge` used by `cmake/nls.cmake`).

### Optional dependencies (only if you enable the feature)

- `libgeoip-dev` — only with `-DENABLE_IP2COUNTRY=ON` (defaults OFF).
- `libpng-dev libgd-dev` — only for `-DBUILD_WEBSERVER=ON` (defaults OFF).
- `libreadline-dev` — CLI readline support (optional, auto-disabled if absent).
- `binutils-dev` — `bfd` backtrace support (optional, auto-disabled if absent).

## Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

> The default build type is `Debug` here because a `.git` directory exists.
> `Debug` is fine for a link test; use `Release` for a faster build.

The `amule` (monolithic) target pulls in `CORE_SOURCES` + `GUI_SOURCES`,
and `amuled` (daemon) pulls in `CORE_SOURCES`. If the new search wiring is
incomplete, expect link errors around `SearchUIAdapter` in the `amuled`
target.
